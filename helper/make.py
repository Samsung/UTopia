import logging
import subprocess
import argparse
from pathlib import Path
from typing import List, Set
from helper.common.setting import Setting
import collections.abc

from helper import EXP_PATH
from helper.common import util

import yaml
import os
import shutil


def check_key(key, src):
    if key not in src:
        raise RuntimeError(f"Not Found {key} [{str(src)}]")


class Target:
    def __init__(
        self, name: str, download_cmds: List[str], compile_cmds: dict, libs: List[str]
    ):
        self.name = name
        self.download_cmds = download_cmds
        self.compile_cmds = compile_cmds
        self.libs = libs

    @staticmethod
    def from_dict(name: str, d: dict) -> "Target":
        if "compile" not in d or d.get("repo", {}).get("cmd") is None:
            raise Exception("Invalid target dict")

        libs = d["libs"] if "libs" in d else []
        libs = list(map(lambda x: os.path.join(EXP_PATH, name, x), libs))
        return Target(
            name=name,
            download_cmds=d["repo"]["cmd"],
            compile_cmds=d["compile"],
            libs=libs,
        )


class Make:
    __keyword_indicator = "k#:"
    __keyword_end_indicator = ":#"
    __exp_dir = EXP_PATH

    def __init__(self, path: Path):
        with open(path) as f:
            self.config: collections.abc.Mapping = yaml.safe_load(f)
            if not isinstance(self.config, collections.abc.Mapping):
                raise Exception("Invalid config file")
            self.__keywords()
        self.targets = {
            k: Target.from_dict(k, v) for k, v in self.config["targets"].items()
        }
        self.__exp_dir.mkdir(exist_ok=True)

    def has_keyword(self, src: str) -> bool:
        return self.__keyword_indicator in src

    def resolve_keyword(self, src: str, visit: Set[str] = None) -> str:
        if visit is None:
            visit = set()

        chunks = self.__keyword_chunks(src)
        replace = dict()
        for chunk in chunks:
            keyword = self.__keyword_in_chunk(chunk)
            content = self.keywords[keyword]

            if not self.has_keyword(content):
                replace[chunk] = content
                continue

            if keyword in visit:
                raise RuntimeError(f"Circlular reference [{keyword}]")

            visit.add(keyword)
            content = self.resolve_keyword(self.keywords[keyword], visit)
            visit.remove(keyword)
            replace[chunk] = content

        reptargets = list(replace.keys())
        for reptarget in reptargets:
            src = src.replace(reptarget, replace[reptarget])

        if self.has_keyword(src):
            raise RuntimeError(f"Error: {src}")

        return src

    def download(self, name: str):

        check_key(name, self.targets)
        target = self.targets[name]

        dirname = target.name

        if (self.__exp_dir / dirname).exists():
            logging.warning(
                f"Skip download : Directory exist ({self.__exp_dir / dirname})"
            )
            return

        for cmd in target.download_cmds:
            logging.debug(f"Exec: {cmd}")
            subprocess.run(
                self.resolve_keyword(cmd),
                shell=True,
                cwd=self.__exp_dir,
                check=True,
            )

    def build(self, name: str, output_dir: str):
        check_key(name, self.targets)
        target = self.targets[name]

        target_dir = self.__exp_dir / target.name
        (target_dir / "output").mkdir(exist_ok=True)
        build_order = ["org", "profile", "fuzzer"]

        output_dir: str = os.path.join(output_dir, "libs")

        for build_type in build_order:
            logging.info(f"Build ({build_type})")
            for cmd in target.compile_cmds[build_type]:
                resolved_cmd = self.resolve_keyword(cmd)
                logging.debug(f"Exec: {resolved_cmd}")
                subprocess.run(resolved_cmd, shell=True, cwd=target_dir, check=True)

            # NOTE: So far, we `copy` built libraries into output directory instead of `moving`
            # not to cause unknown side effect. However, it must be `move` since to minimize
            # the total output volume.
            for lib in target.libs:
                dst: str = os.path.join(output_dir, build_type, os.path.basename(lib))
                os.makedirs(os.path.dirname(dst))
                shutil.copy(lib, dst)
                logging.info(f"Library {lib} is copied into {dst}")

    def __keywords(self):
        self.keywords = self.config.get("keywords")
        if not self.keywords:
            return

        for k, v in self.keywords.items():
            if self.has_keyword(v):
                self.keywords[k] = self.resolve_keyword(v)

    def __keyword_chunks(self, src: str) -> Set[str]:
        stack = list()
        keywords = set()
        for i in range(0, len(src)):
            if src[i:].startswith(self.__keyword_indicator):
                stack.append(i)
            if src[i:].startswith(self.__keyword_end_indicator):
                if len(stack) == 0:
                    return set()
                keywords.add(src[stack[-1] : i + len(self.__keyword_end_indicator)])
                del stack[-1]

        return keywords

    def __keyword_in_chunk(self, chunk: str) -> str:
        chunk = chunk.replace(self.__keyword_indicator, "")
        return chunk.replace(self.__keyword_end_indicator, "")


def clean_previous_result(output_dir: str) -> None:
    if os.path.exists(output_dir):
        if not os.path.isdir(output_dir):
            raise RuntimeError(
                f"There is a file whose name is same as the output directory [{output_dir}]"
            )
        logging.info(f"Delete existing directory [{output_dir}]")
        shutil.rmtree(output_dir)
    logging.info(f"Create output directory [{output_dir}]")
    os.makedirs(output_dir)


def main():
    try:
        parser = argparse.ArgumentParser()
        parser.add_argument(
            "--config",
            default=Path(__file__).absolute().parent / "make.yml",
            type=Path,
        )
        parser.add_argument("target", type=str)
        parser.add_argument("--debug", action="store_true")
        parser.add_argument(
            "--output_dir", type=str, default="output", help="Output Directory"
        )
        args = parser.parse_args()

        logging.basicConfig(
            format=(
                "%(asctime)s | %(levelname)s | %(module)s | %(message)s"
                if args.debug
                else "%(message)s"
            ),
            level=logging.DEBUG if args.debug else logging.INFO,
        )

        Setting().set_output_dir(os.path.join(args.output_dir, args.target))
        if util.clean_directory(Setting().output_lib_dir) == False:
            raise RuntimeError(
                f"Failed to clean directory [{Setting().output_lib_dir}]"
            )
        logging.info(str(Setting()))

        make = Make(args.config)
        make.download(args.target)
        make.build(args.target, Setting().output_dir)
    except RuntimeError as e:
        logging.error(str(e))


if __name__ == "__main__":
    main()
