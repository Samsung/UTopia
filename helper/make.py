import logging
import subprocess
import argparse
from pathlib import Path
from typing import List, Set
import collections.abc

from helper import EXP_PATH

import yaml


def check_key(key, src):
    if key not in src:
        raise RuntimeError(f"Not Found {key} [{str(src)}]")


class Target:
    def __init__(
        self, name: str, download_cmds: List[str], compile_cmds: dict
    ):
        self.name = name
        self.download_cmds = download_cmds
        self.compile_cmds = compile_cmds

    @staticmethod
    def from_dict(name: str, d: dict) -> "Target":
        if "compile" not in d or d.get("repo", {}).get("cmd") is None:
            raise Exception("Invalid target dict")
        return Target(
            name=name,
            download_cmds=d["repo"]["cmd"],
            compile_cmds=d["compile"],
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
            k: Target.from_dict(k, v)
            for k, v in self.config["targets"].items()
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
            logging.warning(f"Skip download : Directory exist ({self.__exp_dir / dirname})")
            return

        for cmd in target.download_cmds:
            logging.debug(f"Exec: {cmd}")
            subprocess.run(
                self.resolve_keyword(cmd),
                shell=True,
                cwd=self.__exp_dir,
                check=True,
            )

    def build(self, name: str):
        check_key(name, self.targets)
        target = self.targets[name]

        target_dir = self.__exp_dir / target.name
        (target_dir / "output").mkdir(exist_ok=True)
        build_order = ["org", "profile", "fuzzer"]
        for build_type in build_order:
            logging.info(f"Build ({build_type})")
            for cmd in target.compile_cmds[build_type]:
                resolved_cmd = self.resolve_keyword(cmd)
                logging.debug(f"Exec: {resolved_cmd}")
                subprocess.run(
                    resolved_cmd, shell=True, cwd=target_dir, check=True
                )

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
                keywords.add(
                    src[stack[-1] : i + len(self.__keyword_end_indicator)]
                )
                del stack[-1]

        return keywords

    def __keyword_in_chunk(self, chunk: str) -> str:
        chunk = chunk.replace(self.__keyword_indicator, "")
        return chunk.replace(self.__keyword_end_indicator, "")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config",
        default=Path(__file__).absolute().parent / "make.yml",
        type=Path,
    )
    parser.add_argument("target", type=str)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(
        format="%(asctime)s | %(levelname)s | %(module)s | %(message)s"
        if args.debug
        else "%(message)s",
        level=logging.DEBUG if args.debug else logging.INFO,
    )

    make = Make(args.config)
    make.download(args.target)
    make.build(args.target)


if __name__ == "__main__":
    main()
