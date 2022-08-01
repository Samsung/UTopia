import argparse
import json
import logging
import os
import shutil
import subprocess
from pathlib import Path
from typing import List

from helper import EXP_PATH, driverbuilder, astirbuilder

import yaml


class Builder:
    __basedir = Path(__file__).absolute().parent.parent

    def __init__(
        self,
        prj_name: str,
        prj_path: Path,
        buildpath: Path,
        version: str,
        framework: str,
    ):

        self.prj_name = prj_name
        self.prj_path = prj_path
        self.buildpath = (
            prj_path if buildpath is None else self.prj_path / buildpath
        )
        self.version = version
        self.framework = framework

        self.gen_path = (
            self.__basedir / "result" / self.version / self.prj_name
        )
        self.gen_path.mkdir(parents=True, exist_ok=True)

    @property
    def api_path(self) -> Path:
        return self.gen_path / "api.json"

    @property
    def target_path(self) -> Path:
        return self.gen_path / "target"

    def get_ent_path(self, name: str) -> Path:
        return (
            self.prj_path / "output" / "entry" / f"{name}_project_entry.json"
        )

    def get_ut_path(self, name: str) -> Path:
        return self.gen_path / f"{name}.json"

    def get_driver_path(self, name: str) -> Path:
        return self.gen_path / f"gen_{name}"

    def generate_api_report(self, libs: List[str]):
        logging.info("Generate API Name Report")

        report = {
            lib: collect_api(self.prj_path / "output" / lib)
            for lib in sorted(libs)
        }
        self.api_path.parent.mkdir(exist_ok=True, parents=True)
        with self.api_path.open("wt") as f:
            f.write(json.dumps(report, indent=4))

    def generate_ast_ir(
        self,
        src_path: str,
        build_path: str,
        lib_name: str,
        build_name: str,
    ):

        logging.info("Generate clang AST and LLVM IR")
        src_path = src_path if len(src_path) == 0 else self.prj_path / src_path

        # TODO: buildpath should be set absolute path in a very first stage.
        assert not os.path.isabs(build_path)
        build_path = os.path.normpath(self.prj_path / build_path)
        astirbuilder.build_ast_ir(
            Path(self.prj_path),
            Path(src_path),
            Path(build_path),
            lib_name,
            build_name,
        )

    def generate_target_report(self, libname: str):
        logging.info("Generate Target Report")

        tool = "target_analyzer"
        entpath = self.get_ent_path(libname)
        output = self.target_path / f"{libname}.json"

        output.parent.mkdir(parents=True, exist_ok=True)
        if output.exists():
            output.unlink()

        cmd = [
            tool,
            "--entry",
            entpath,
            "--extern",
            "external",
            "--name",
            libname,
            "--public",
            self.api_path,
            "--out",
            output,
        ]
        self.__execute(cmd)

        return True if output.exists() else False

    def generate_ut_report(self, ut_name: str) -> bool:
        logging.info("Generate UT Report")

        tool = "ut_analyzer"
        entpath = self.get_ent_path(ut_name)
        output = self.get_ut_path(ut_name)

        output.parent.mkdir(parents=True, exist_ok=True)
        if output.exists():
            output.unlink()

        cmd = [
            tool,
            "--entry",
            entpath,
            "--name",
            ut_name,
            "--extern",
            "external",
            "--ut",
            self.framework,
            "--target",
            self.target_path,
            "--public",
            self.api_path,
            "--out",
            output,
        ]
        self.__execute(cmd)
        return True if output.exists() else False

    def generate_fuzz_driver(self, ut_name: str) -> bool:
        logging.info("Generate Fuzz Driver Source Code")

        tool = "fuzz_generator"
        entpath = self.get_ent_path(ut_name)
        utpath = self.get_ut_path(ut_name)
        output = self.get_driver_path(ut_name)

        with open(entpath) as f:
            entry_data = json.load(f)
            src_dir = entry_data["project_dir"]

        os.makedirs(os.path.dirname(output), exist_ok=True)
        if os.path.exists(output):
            shutil.rmtree(output)

        cmd = [
            tool,
            "--src",
            src_dir,
            "--target",
            self.target_path,
            "--ut",
            utpath,
            "--public",
            self.api_path,
            "--out",
            output,
        ]
        if not self.__execute(cmd):
            return False
        return True if output.exists() else False

    def build_fuzz_driver(
        self,
        ut_name: str,
        ut_key: str,
        ut_src: str,
        libs: List[str],
        build_dir: str,
    ):
        logging.info("Build Fuzz Driver")
        code_path = self.get_driver_path(ut_name)
        driverbuilder.build_driver(
            code_path,
            self.prj_path,
            Path(ut_src),
            ut_key,
            Path(build_dir),
            libs,
        )

    def __execute(self, cmd: List[str], debug: bool = True) -> bool:
        if debug:
            logging.debug(f"cmd: {cmd}")
            p = subprocess.Popen(cmd)
            p.communicate()
        else:
            with open(os.devnull, "wb") as f:
                p = subprocess.Popen(cmd, stdout=f, stderr=f)
                p.communicate()

        return p.returncode == 0


def set_default_values(src: dict) -> dict:
    if "builddir" not in src:
        src["builddir"] = "."

    if "libs" in src and src["libs"] is not None:
        libs = src["libs"]
        for lib, conf in libs.items():
            if conf is None:
                conf = {}
            conf["builddir"] = str(
                Path(src["builddir"]) / conf.get("builddir", "")
            )
            if "buildkey" not in conf:
                conf["buildkey"] = lib
            if "libname" not in conf:
                conf["libname"] = lib
            if "srcpath" not in conf:
                conf["srcpath"] = "."
            libs[lib] = conf
        src["libs"] = libs

    if "uts" in src and src["uts"] is not None:
        uts = src["uts"]
        for ut, conf in uts.items():
            conf["builddir"] = str(
                Path(src["builddir"]) / conf.get("builddir", "")
            )
            if "srcpath" not in conf:
                conf["srcpath"] = "."
            if "buildkey" not in conf:
                conf["buildkey"] = ut
        src["uts"] = uts

    if "framework" not in src:
        src["framework"] = "gtest"

    return src


def get_targets(config: Path, name: str) -> dict:
    with open(config) as f:
        ctx = yaml.safe_load(f)

    if name == "all":
        return ctx

    if name not in ctx:
        raise RuntimeError(f"Not Found [{name}]")

    return {name: set_default_values(ctx[name])}


def generate_ast_ir(builder: Builder, libs: dict, uts: dict):
    for lib, data in libs.items():
        builder.generate_ast_ir(
            data["srcpath"], data["builddir"], lib, data["buildkey"]
        )

    for ut, data in uts.items():
        builder.generate_ast_ir(
            data["srcpath"], data["builddir"], ut, data["buildkey"]
        )


def generate_target_report(builder: Builder, libs: dict):
    for key in libs.keys():
        if not builder.generate_target_report(key):
            raise Exception(f"Failed to generate target report ({key})")
    # 2. Merge Public API List


def generate_ut_report(builder: Builder, uts: dict):
    for key in uts.keys():
        if not builder.generate_ut_report(key):
            raise Exception(f"Failed to generate UT report ({key})")


def generate_fuzz_driver(builder: Builder, uts: dict):
    for key in uts.keys():
        if not builder.generate_fuzz_driver(key):
            raise Exception(f"Failed to generate Fuzz Driver Code ({key})")


def build_fuzz_driver(builder: Builder, uts: dict):
    for key in sorted(uts.keys()):
        ut = uts[key]
        ut_buildkey = ut["buildkey"]
        ut_src = ut["srcpath"]

        libs = [
            lib.strip()[: lib.strip().rfind(".")]
            for lib in ut["libalias"].split()
            if lib.strip()
        ]

        builddir = ut.get("builddir", "")

        builder.build_fuzz_driver(key, ut_buildkey, ut_src, libs, builddir)


def build(target: dict, version: str):
    logging.info(f"Start build | {target} | {version}")
    builder = Builder(
        target["name"],
        EXP_PATH / target["prjdir"],
        Path(target["builddir"]),
        version,
        target["framework"],
    )
    libs = target["libs"]
    uts = target["uts"]

    builder.generate_api_report(list(libs.keys()))
    generate_ast_ir(builder, libs, uts)
    generate_target_report(builder, libs)
    generate_ut_report(builder, uts)
    generate_fuzz_driver(builder, uts)
    build_fuzz_driver(builder, uts)


def collect_api(path: Path) -> List[str]:
    res = subprocess.run(
        ["nm", "--no-demangle", "--defined-only", "-g", path],
        check=True,
        capture_output=True,
        text=True,
    )
    apis = set()
    for line in res.stdout.splitlines():
        tokens = line.strip().split()
        if len(tokens) != 3:
            continue
        if tokens[1] == "T":
            apis.add(tokens[2])

    return list(sorted(apis))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config",
        default=Path(__file__).absolute().parent / "build.yml",
        type=Path,
    )
    parser.add_argument("--version", default="test", type=str)
    parser.add_argument("target", type=str)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(
        format="%(asctime)s | %(levelname)s | %(module)s | %(message)s"
        if args.debug
        else "%(message)s",
        level=logging.DEBUG if args.debug else logging.INFO,
    )

    targets = get_targets(args.config, args.target)
    for target in sorted(targets.keys()):
        build(targets[target], args.version)


if __name__ == "__main__":
    main()
