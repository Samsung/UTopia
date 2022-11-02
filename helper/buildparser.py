import json
import re

from pathlib import Path
from typing import Tuple, Dict, List


class CMD:
    def __init__(self, cmd: str, path: Path = Path()):
        self.path = path.absolute()
        self.cmd = cmd

    def __str__(self):
        return f"CMD[path: {self.path}\n" + f"    cmd: {self.cmd}]"

    def is_link(self):
        if (" -c " not in self.cmd) and (not self.cmd.endswith("-c")):
            return True
        return False

    def is_assemble(self):
        tool = self.cmd.split()[0]
        if "yasm" in tool:
            return True

        output = re.search(r"(-o )(.+?)( |$)", self.cmd)
        if output is not None and re.search(
            r"[\w._/-]+?\.l?(s)", output.group(3)
        ):
            return True

        tokens = [
            token
            for token in self.cmd.split()
            if Path(token).exists() and token.endswith(".S")
        ]
        return True if tokens else False

    def abspath(self, src: Path) -> Path:
        if src.is_absolute():
            return src.resolve()
        return (self.path / src).resolve()

    def output(self, real: bool = True) -> Path:
        result = ""
        if match := re.search(r"(-o )(.+?)( |$)", self.cmd):
            result = match.group(2)
        elif match := re.search(r"[\w._/-]+?\.l?a", self.cmd):
            result = match.group(0)
        elif match := re.search(r"[\w._/-]+?\.l?(c|cpp|cc)", self.cmd):
            result = match.group(0)
            result = result[: result.rfind(".")]
            result = result + ".o"
        result = Path(result)

        return self.abspath(result) if real else result

    def objects(self) -> List[Path]:
        tokens = self.cmd.split()

        if self.is_link():
            result = {token for token in tokens if token.endswith(".o")}
        else:
            try:
                idx = tokens.index("-o")
            except ValueError:
                raise RuntimeError(f"Unable to find objects in ({self.cmd})")
            result = {
                tokens[idx + 1],
            }

        return [self.abspath(Path(obj)) for obj in result]

    def sources(self, real: bool = True) -> List[Path]:
        tokens = self.cmd.split()
        sources = [
            Path(token)
            for token in tokens
            if re.search(r"\.(c|cpp|cc)$", token)
        ]
        if real:
            sources = [self.abspath(source) for source in sources]
        return sources


class BuildParser:
    def __init__(self, path: Path, buildpath: Path):

        self.path = path.resolve()
        self.buildpath = buildpath.resolve()
        assert self.path.is_absolute()
        assert self.buildpath.is_absolute()
        assert self.output_dir.exists()
        assert self.build_log_path.exists()
        assert self.buildpath.exists()

    @property
    def output_dir(self) -> Path:
        return self.path / "output"

    @property
    def build_db_dir(self) -> Path:
        return self.output_dir / "build_db"

    @property
    def build_log_path(self) -> Path:
        return self.output_dir / "build.log"

    def parse(self, real: bool = True) -> Tuple[Dict, Dict, Dict, Dict]:
        with open(self.build_log_path) as f:
            lines = f.read().replace("\\\n", "").split("\n")

        compiles, links, assems = [], [], []
        for line in lines:
            if not line:
                continue
            command = self.parse_command(line)
            if command.is_link():
                links.append(command)
            else:
                compiles.append(command)
            if command.is_assemble():
                assems.append(command)

        srcs = dict()
        for cmd in compiles:
            for src in cmd.sources(True):
                srcs[src] = cmd

        compiles = {
            compile_cmd.output(real): compile_cmd for compile_cmd in compiles
        }
        links = {link_cmd.output(real): link_cmd for link_cmd in links}
        assems = {assem_cmd.output(real): assem_cmd for assem_cmd in assems}

        return srcs, compiles, links, assems

    def parse_command(self, captured_command: str) -> CMD:
        tokens = [
            token.strip()
            for token in captured_command.split("|autofuzz_splitter|")
            if token.strip()
        ]
        if len(tokens) != 2:
            raise Exception("Invalid command")

        path = Path(tokens[0])
        command = tokens[1]
        command = self.normalize_command(command)
        return CMD(command, path)

    def make_build_db(
        self,
        name: str,
        src_path: Path,
        bc_file_path: Path,
        ast_paths: List[Path],
    ):
        data = {
            "bc": str(bc_file_path),
            "ast": [str(ast_path) for ast_path in ast_paths],
            "project_dir": str(src_path),
        }

        self.build_db_dir.mkdir(parents=True, exist_ok=True)
        build_db_path = self.build_db_dir / f"{name}.json"
        with open(build_db_path, "w") as f:
            json.dump(data, f)

    def normalize_command(self, cmd: str) -> str:
        cmd_pattern = re.compile(r"(ar|llvm-ar|llvm-ar-10|clang|clang\+\+) ")
        unnecessary_opts = {
            "-O4",
            "-O3",
            "-O2",
            "-O1",
            "-Og",
            "-Os",
            "-Oz",
            "-Ofast",
            "-O0",
            "-O",
            "-DNDEBUG",
            "-fno-exceptions",
            "-std=gnu99",
        }

        cmd = cmd[cmd_pattern.search(cmd).start() :]
        tokens = []
        for token in cmd.split():
            token = token.strip()
            if (
                not token
                or token in unnecessary_opts
                or token.startswith("-Werror")
                or token.startswith("-fsanitize")
            ):
                continue
            if token.startswith("&&"):
                tokens.clear()
            tokens.append(token)

        return " ".join(tokens)
