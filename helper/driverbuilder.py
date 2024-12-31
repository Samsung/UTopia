import copy
import os
import shutil
import logging
import subprocess
from pathlib import Path
from typing import List, Set

from helper.buildparser import BuildParser, CMD
from helper.driverparser import DriverParser
from helper.git import Git


class FuzzerBuilder:
    def __init__(
        self, name: str, prj_path: Path, src_path: Path, build_path: Path
    ):

        self.name = name
        self.prj_path = prj_path
        self.src_path = src_path
        self.build_path = build_path

        assert self.prj_path.is_absolute()
        assert self.src_path.is_absolute()
        assert self.build_path.is_absolute()

        self.bp = BuildParser(prj_path, build_path)
        (
            self.src_cmds,
            self.compile_cmds,
            self.link_cmds,
            self.assem_cmds,
        ) = self.bp.parse()
        self.link_cmds = {
            link_cmd.output(False): link_cmd
            for link_cmd in self.link_cmds.values()
        }

    def build(
        self,
        link_name: str,
        src_path: Path,
        extra_src_path: Set[Path],
        libs: List[str],
        profile: bool,
    ):
        for extra_src in extra_src_path:
            self.compile(self.get_compile_cmd_by_src(extra_src), profile)
        src_cmd = self.get_compile_cmd_by_src(src_path)
        autofuzz_cmds = self.get_compile_cmds_for_autofuzz(src_cmd)
        autofuzz_objs = [
            self.compile(autofuzz_cmd, profile)
            for autofuzz_cmd in autofuzz_cmds
        ]
        link_cmd = self.get_link_cmd(link_name, autofuzz_objs, libs, profile)
        logging.info(f"Link ({link_cmd.output()})")
        self.execute(link_cmd)
        assert os.path.exists(link_cmd.output())

    def compile(self, command: CMD, profile: bool = False) -> Path:

        avas_flag = "-DBOOST_TEST_NO_MAIN -DAUTOFUZZ"
        asan_flag = "-fsanitize=address,fuzzer-no-link"
        prof_flag = "-fprofile-instr-generate -fcoverage-mapping"

        options = [command.cmd, avas_flag, asan_flag]
        if profile:
            options.append(prof_flag)

        cmd = " ".join(options)

        # NOTE: Sometimes, -fPIE should be deleted to generate fuzz drivers.
        # cmd = cmd.replace('-fPIE', '')
        output = command.output()
        if str(output) not in cmd:
            cmd += f" -o {output}"

        logging.info(f"Compile ({output})")

        self.execute(CMD(cmd, command.path))
        assert output.exists()

        return output

    def execute(self, command: CMD):
        logging.info("Command (" + command.cmd + ")")
        subprocess.run(command.cmd, shell=True, cwd=command.path, check=True)

    def get_compile_cmd_by_src(self, src_path: Path) -> CMD:
        keys2 = {
            src_cmd.abspath(src): src
            for src, src_cmd in self.src_cmds.items()
            if not src.is_absolute()
        }

        if src_path in self.src_cmds:
            obj = self.src_cmds[src_path].output()
        else:
            assert src_path in keys2
            obj = self.src_cmds[keys2[src_path]].output()

        return self.get_compile_cmd_by_obj(obj)

    def get_compile_cmd_by_obj(self, obj: Path) -> CMD:

        assert obj in self.compile_cmds
        return self.compile_cmds[obj]

    def get_compile_cmds_for_autofuzz(self, base_cmd: CMD):

        output_dir = self.build_path / "avas_out"
        output_dir.mkdir(parents=True, exist_ok=True)

        src_path = base_cmd.sources()[0].parent

        avas_sources = ["FuzzArgsProfile.pb.cc", "fuzz_entry.cc"]
        avas_sources = [src_path / src for src in avas_sources]
        proto_path = avas_sources[0].parent
        subprocess.run(
            [
                "protoc",
                f"--cpp_out={proto_path}",
                f"--proto_path={proto_path}",
                proto_path / "FuzzArgsProfile.proto",
            ],
            check=True,
        )

        return [
            self.make_autofuzz_src_cmd(src, output_dir, base_cmd)
            for src in avas_sources
        ]

    def make_autofuzz_src_cmd(
        self, src: Path, output_dir: Path, base_cmd: CMD
    ) -> CMD:
        sources = base_cmd.sources(False)

        cmd = base_cmd.cmd
        cmd = cmd.replace(str(sources[0]), str(src))
        for source in sources[1:]:
            cmd = cmd.replace(str(source), "")

        obj_path = output_dir / f"{src.name}.o"

        tokens = cmd.split()
        tokens[0] = "clang++"

        output = base_cmd.output(False)
        for i in range(0, len(tokens)):
            if tokens[i] == str(output):
                tokens[i] = str(obj_path)
        cmd = " ".join(tokens)
        cmd += " -I/usr/local/include/libprotobuf-mutator"

        cmd = cmd.replace("std=gnu89", "")
        return CMD(cmd, base_cmd.path)

    def get_link_cmd(
        self, name: str, extra_objs: List[Path], libs: List[str], profile: bool
    ) -> CMD:

        cmd_protobuf = (
            " -L/usr/local/lib -l:libprotobuf-mutator-libfuzzer.a "
            "-l:libprotobuf-mutator.a -l:libprotobuf.a"
        )
        cmd_fuzzer = "-fsanitize=address,fuzzer -fno-omit-frame-pointer -g"
        cmd_profile = "-fprofile-instr-generate -fcoverage-mapping"

        name = Path(name)
        link_cmds = [x for x in self.link_cmds if str(x).endswith(str(name))]
        assert len(link_cmds) == 1
        link_cmd = copy.copy(self.link_cmds[link_cmds[0]])

        output_dir = self.bp.output_dir
        cmd = link_cmd.cmd
        cmd += " " + " ".join(str(obj) for obj in extra_objs)

        if profile:
            pf = "profile"
            output_dir = output_dir / "profiles"
        else:
            pf = "fuzzer"
            output_dir = output_dir / "fuzzers"

        output_dir.mkdir(parents=True, exist_ok=True)

        cmd = cmd.replace(
            f" {link_cmd.output(False)} ", f" {output_dir / self.name} "
        )

        for lib in libs:
            cmd = self.replace_link_lib(
                cmd, lib, f"-L{self.bp.output_dir} -l:{lib}_{pf}.a"
            )

        cmd = self.erase_link_lib(cmd)
        cmd += f" {cmd_protobuf} {cmd_fuzzer}"
        if profile:
            cmd += f" {cmd_profile}"

        # Heuristics 1: Do not include gtest-main object in link command.
        gtest_mains = ["gtest_main.cc.o", "gtest_main.o"]
        tokens = cmd.split(" ")
        tokens = [
            token for token in tokens if Path(token).name not in gtest_mains
        ]

        return CMD(" ".join(tokens), link_cmd.path)

    # Heuristics
    def erase_link_lib(self, cmd: str):

        opts = ["-lprotobuf"]

        tokens = cmd.split()
        for i in reversed(range(0, len(tokens))):
            if tokens[i] in opts:
                del tokens[i]
                continue

        return " ".join(tokens)

    # Heuristics
    def replace_link_lib(self, cmd: str, libname: str, opt: str) -> str:
        archive = f"{libname}.a"
        patterns = ["-l" + libname[3:], archive]
        tokens = cmd.split()
        place = set()

        for i, token in enumerate(tokens):
            if Path(token).name == archive or token in patterns:
                place.add(i)

        if not place:
            raise RuntimeError("Not Found Link Option [ " + cmd + " ]")

        place = sorted(place)
        tokens[place[0]] = opt

        for index in reversed(place[1:]):
            del tokens[index]

        return " ".join(tokens)


def prepare_git(git: Git, name: str, dst: Path, src: Path):
    # Remove all srcs and copy generated srcs from fuzz_generator
    logging.info(f"Prepare git [{name}]")

    git.checkout(git.branch)
    try:
        git.delete_branch(name)
    except RuntimeError:
        pass
    git.checkout(name, True)

    dst = dst.resolve()
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst)

    git.add(dst)
    git.commit("Fuzzer")

    git.checkout(git.branch)


def get_changeables(git: Git, name: str):
    logging.info(f"Get Changeables [{name}]")

    git.checkout(name)

    changed = set(git.changed())

    git.checkout(git.branch)

    return changed


def build_fuzzer(
    git: Git,
    base: Path,
    name: str,
    libs: List[str],
    ut_link_name: str,
    ut_src: Path,
    changeables: Set[Path],
    build_dir: Path,
):
    builder = FuzzerBuilder(
        name, git.root_dir, git.root_dir / base, git.root_dir / build_dir
    )

    git.checkout(name)
    builder.build(
        ut_link_name,
        git.root_dir / base / ut_src,
        changeables,
        libs,
        False,
    )
    builder.build(
        ut_link_name,
        git.root_dir / base / ut_src,
        changeables,
        libs,
        True,
    )
    git.checkout(git.branch)


def build_driver(
    generated_dir: Path,
    git_dir: Path,
    base_dir: Path,
    ut_link_name: str,
    build_dir: Path,
    libs: List[str],
):
    logging.info(f"UT: {ut_link_name}")

    dp = DriverParser(generated_dir)
    drivers = dp.parse()

    git = Git(git_dir)

    # changeables: fuzzer changed files
    changeables = set()
    for driver in drivers:
        logging.info(f"Build {driver}")
        prepare_git(git, driver.name, git_dir / base_dir, driver.path)
        changeables = changeables | get_changeables(git, driver.name)

    logging.info(f"changed filed: {changeables}")
    for driver in drivers:
        build_fuzzer(
            git,
            base_dir,
            driver.name,
            libs,
            ut_link_name,
            driver.source,
            changeables,
            build_dir,
        )
    logging.info(f"Built Fuzzers: {len(drivers)}")
