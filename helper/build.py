import argparse
import json
import logging
import os
import shutil
import subprocess
import tempfile
from helper.common.setting import Setting
from pathlib import Path
from typing import List

from helper import EXP_PATH, driverbuilder, astirbuilder
from helper.common import util
from helper.git import Git

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
        self.buildpath = prj_path if buildpath is None else self.prj_path / buildpath
        self.version = version
        self.framework = framework

        self.gen_path = self.__basedir / "result" / self.version / self.prj_name
        self.gen_path.mkdir(parents=True, exist_ok=True)

    @property
    def api_path(self) -> Path:
        return self.gen_path / "api.json"

    @property
    def target_path(self) -> Path:
        return self.gen_path / "target"

    @property
    def extern_path(self) -> Path:
        return Path(__file__).absolute().parent / "external"

    def get_build_db_path(self, name: str) -> Path:
        return self.prj_path / "output" / "build_db" / f"{name}.json"

    def get_ut_path(self, name: str) -> Path:
        return self.gen_path / f"{name}.json"

    def get_driver_path(self, name: str) -> Path:
        return self.gen_path / f"gen_{name}"

    def generate_api_report(self, libs: List[str], lib_dir: str, output_path: str):
        logging.info(f'Generate API Report [libs: {", ".join(libs)}]')
        libs: list[str] = sorted(
            list(map(lambda lib: os.path.join(lib_dir, lib), libs))
        )
        report: dict[str, any] = {lib: collect_api(lib) for lib in libs}
        with open(output_path, "wt") as f:
            f.write(json.dumps(report, indent=4))

    def generate_ast_ir(
        self,
        src_path: str,
        build_path: str,
        lib_name: str,
        build_name: str,
        ast_dir: str,
        bc_dir: str,
        output_path: str,
    ):
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
            ast_dir,
            bc_dir,
            output_path,
        )

    def generate_target_report(
        self, build_db_path: str, api_path: str, output_path: str
    ) -> bool:
        logging.info(f"Generate Target Report [{output_path}]")

        tool = "target_analyzer"
        if shutil.which(tool) == None:
            raise RuntimeError(f"Tool Not Found [{tool}]")

        for necessity in [build_db_path, api_path, os.path.dirname(output_path)]:
            if not os.path.exists(necessity):
                raise RuntimeError(f"File Not Found [{necessity}]")

        if os.path.exists(output_path):
            if os.path.isdir(output_path):
                shutil.rmtree(output_path)
            else:
                os.unlink(output_path)

        cmd = [
            tool,
            "--db",
            build_db_path,
            "--extern",
            self.extern_path,
            "--public",
            api_path,
            "--out",
            output_path,
        ]
        self.__execute(cmd)

        return True if os.path.exists(output_path) else False

    def generate_ut_report(
        self, build_db_path: str, api_path: str, output_path: str
    ) -> bool:
        logging.info(f"Generate UT Report [{output_path}]")

        tool = "ut_analyzer"
        if shutil.which(tool) == None:
            raise RuntimeError(f"Tool Not Found [{tool}]")

        for necessity in [build_db_path, api_path, os.path.dirname(output_path)]:
            if not os.path.exists(necessity):
                raise RuntimeError(f"File Not Found [{necessity}]")

        if os.path.exists(output_path):
            if os.path.isdir(output_path):
                shutil.rmtree(output_path)
            else:
                os.unlink(output_path)

        cmd = [
            tool,
            "--db",
            build_db_path,
            "--extern",
            self.extern_path,
            "--ut",
            self.framework,
            "--target",
            self.target_path,
            "--public",
            self.api_path,
            "--out",
            output_path,
        ]
        self.__execute(cmd)
        return True if os.path.exists(output_path) else False

    def generate_fuzz_driver(
        self,
        build_db_path: str,
        api_path: str,
        target_report_dir_path: str,
        ut_report_path: str,
        base_dir_path: str,
        fuzz_generator_report_path: str,
        fuzzer_output_dir_path: str,
    ) -> bool:
        logging.info("Generate Fuzz Driver Source Code")

        tool = "fuzz_generator"
        if shutil.which(tool) == None:
            raise RuntimeError(f"Tool Not Found [{tool}]")

        for necessity in [
            build_db_path,
            api_path,
            target_report_dir_path,
            ut_report_path,
        ]:
            if not os.path.exists(necessity):
                raise RuntimeError(f"File Not Found [{necessity}]")

        with open(build_db_path) as f:
            build_db_data = json.load(f)
            src_dir = build_db_data["project_dir"]

        with tempfile.TemporaryDirectory() as temp_dir:
            cmd = [
                tool,
                "--src",
                src_dir,
                "--target",
                target_report_dir_path,
                "--ut",
                ut_report_path,
                "--public",
                api_path,
                "--out",
                temp_dir,
            ]
            if not self.__execute(cmd):
                return False

            report_name: str = "fuzzGen_Report.json"
            report_src_path: str = os.path.join(temp_dir, report_name)
            if not os.path.isfile(report_src_path):
                raise RuntimeError(f"Output File Not Found [{report_src_path}]")
            if os.path.exists(fuzz_generator_report_path):
                os.unlink(fuzz_generator_report_path)
            shutil.move(report_src_path, fuzz_generator_report_path)

            corpus_dir_path: str = os.path.join(temp_dir, "corpus")
            if not os.path.isdir(corpus_dir_path):
                raise RuntimeError(f"Output Directory Not Found [{corpus_dir_path}]")

            corpuses: list[str] = os.listdir(corpus_dir_path)
            for corpus in corpuses:
                fuzzer_name: str = corpus.split(".")[0]
                fuzzer_dir_path: str = os.path.join(fuzzer_output_dir_path, fuzzer_name)
                os.makedirs(fuzzer_dir_path, exist_ok=True)
                shutil.copy2(
                    os.path.join(corpus_dir_path, corpus),
                    os.path.join(fuzzer_dir_path, "seed"),
                )
            shutil.rmtree(corpus_dir_path)

            generated_dirs_path: list[str] = list(
                map(lambda x: os.path.join(temp_dir, x), os.listdir(temp_dir))
            )
            git = Git(self.prj_path)
            for generated_dir_path in generated_dirs_path:
                generated_fuzzer_name: str = os.path.basename(generated_dir_path)
                logging.info(
                    f"Create Branch for generated source code[{generated_fuzzer_name}]"
                )
                util.prepare_git(
                    git,
                    generated_fuzzer_name,
                    self.prj_path / base_dir_path,
                    generated_dir_path,
                )
        return True

    def build_fuzz_driver(
        self,
        ut_key: str,
        ut_src: str,
        lib_dir_path: str,
        libs: List[str],
        build_dir: str,
        fuzz_generator_report_path: str,
        output_dir: str,
    ):
        logging.info("Build Fuzz Driver")
        driverbuilder.build_driver(
            self.prj_path,
            Path(ut_src),
            ut_key,
            Path(build_dir),
            lib_dir_path,
            libs,
            fuzz_generator_report_path,
            output_dir,
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
            conf["builddir"] = str(Path(src["builddir"]) / conf.get("builddir", ""))
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
            conf["builddir"] = str(Path(src["builddir"]) / conf.get("builddir", ""))
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


def generate_ast_ir(
    builder: Builder, libs: dict, uts: dict, ast_dir: str, bc_dir: str, output_path: str
):
    for lib, data in libs.items():
        builder.generate_ast_ir(
            data["srcpath"],
            data["builddir"],
            lib,
            data["buildkey"],
            ast_dir,
            bc_dir,
            output_path,
        )

    for ut, data in uts.items():
        builder.generate_ast_ir(
            data["srcpath"],
            data["builddir"],
            ut,
            data["buildkey"],
            ast_dir,
            bc_dir,
            output_path,
        )


def generate_target_report(
    builder: Builder, libs: dict, build_db_dir: str, api_path: str, output_dir: str
):
    if util.clean_directory(output_dir) == False:
        raise RuntimeError(f"Failed to delete directory [{output_dir}]")

    for key in libs.keys():
        build_db_path: str = os.path.join(build_db_dir, f"{key}.json")
        output_path: str = os.path.join(output_dir, key)
        if not builder.generate_target_report(build_db_path, api_path, output_path):
            raise Exception(f"Failed to generate target report ({key})")
    # 2. Merge Public API List


def generate_ut_report(
    builder: Builder, uts: dict, build_db_dir: str, api_path: str, output_dir: str
):
    if util.clean_directory(output_dir) == False:
        raise RuntimeError(f"Failed to delete directory [{output_dir}]")

    for key in uts.keys():
        build_db_path: str = os.path.join(build_db_dir, f"{key}.json")
        output_path: str = os.path.join(output_dir, f"{key}.json")
        if not builder.generate_ut_report(build_db_path, api_path, output_path):
            raise Exception(f"Failed to generate UT report ({key})")


def generate_fuzz_driver(
    builder: Builder,
    uts: dict,
    build_db_dir: str,
    api_path: str,
    target_anlayzer_report_dir_path: str,
    ut_analyzer_report_dir_path: str,
    fuzz_generator_report_path: str,
    fuzzer_output_dir_path: str,
):
    for key in uts.keys():
        build_db_path: str = os.path.join(build_db_dir, f"{key}.json")
        ut_analyzer_report_path: str = os.path.join(
            ut_analyzer_report_dir_path, f"{key}.json"
        )
        if not builder.generate_fuzz_driver(
            build_db_path,
            api_path,
            target_anlayzer_report_dir_path,
            ut_analyzer_report_path,
            Path(uts[key]["srcpath"]),
            fuzz_generator_report_path,
            fuzzer_output_dir_path,
        ):
            raise Exception(f"Failed to generate Fuzz Driver Code ({key})")


def build_fuzz_driver(
    builder: Builder,
    uts: dict,
    lib_dir_path: str,
    fuzz_generator_report_path: str,
    output_dir: str,
):
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
        builder.build_fuzz_driver(
            ut_buildkey,
            ut_src,
            lib_dir_path,
            libs,
            builddir,
            fuzz_generator_report_path,
            output_dir,
        )


def build(
    target: dict,
    version: str,
    lib_dir: str,
    report_api_path: str,
    ast_dir: str,
    bc_dir: str,
    build_db_path: str,
    target_analyzer_report_dir_path: str,
    ut_analyzer_report_dir_path: str,
    fuzz_generator_report_path: str,
    fuzzer_dir_path: str,
):
    builder = Builder(
        target["name"],
        EXP_PATH / target["prjdir"],
        Path(target["builddir"]),
        version,
        target["framework"],
    )
    libs = target["libs"]
    uts = target["uts"]

    builder.generate_api_report(
        list(libs.keys()), os.path.join(lib_dir, "org"), report_api_path
    )
    generate_ast_ir(builder, libs, uts, ast_dir, bc_dir, build_db_path)
    generate_target_report(
        builder, libs, build_db_path, report_api_path, target_analyzer_report_dir_path
    )
    generate_ut_report(
        builder, uts, build_db_path, report_api_path, ut_analyzer_report_dir_path
    )
    generate_fuzz_driver(
        builder,
        uts,
        build_db_path,
        report_api_path,
        target_analyzer_report_dir_path,
        ut_analyzer_report_dir_path,
        fuzz_generator_report_path,
        fuzzer_dir_path,
    )
    build_fuzz_driver(
        builder, uts, lib_dir, fuzz_generator_report_path, fuzzer_dir_path
    )


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
    try:
        parser = argparse.ArgumentParser()
        parser.add_argument(
            "--config",
            default=Path(__file__).absolute().parent / "build.yml",
            type=Path,
        )
        parser.add_argument("--version", default="test", type=str)
        parser.add_argument("target", type=str)
        parser.add_argument("--debug", action="store_true")
        parser.add_argument(
            "--output_dir",
            type=str,
            default=f'{os.path.abspath("output")}',
            help="Output Diretcory",
        )
        args = parser.parse_args()

        logging.basicConfig(
            format="%(asctime)s | %(levelname)s | %(module)s | %(message)s",
            handlers=[logging.StreamHandler()],
            level=logging.DEBUG if args.debug else logging.INFO,
        )

        Setting().set_output_dir(os.path.join(args.output_dir, args.target))
        if util.clean_directory(Setting().output_fuzzer_dir) == False:
            raise RuntimeError(
                f"Failed to clean directory [{Setting().output_lib_dir}]"
            )
        os.makedirs(Setting().output_report_dir, exist_ok=True)

        targets = get_targets(args.config, args.target)
        for target in sorted(targets.keys()):
            build(
                targets[target],
                args.version,
                Setting().output_lib_dir,
                Setting().output_report_api_path,
                Setting().output_ast_dir,
                Setting().output_bc_dir,
                Setting().output_report_build_db_dir,
                Setting().target_analyzer_report_dir_path,
                Setting().ut_analyzer_report_dir_path,
                Setting().fuzzer_generator_report_path,
                Setting().output_fuzzer_dir,
            )

    except RuntimeError as e:
        logging.error(str(e))


if __name__ == "__main__":
    main()
