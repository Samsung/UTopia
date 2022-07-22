import logging
import subprocess
from typing import Tuple, List
from pathlib import Path


class Git:
    def __init__(self, root_dir: Path):
        self.branch = "autofuzz_base"
        self.root_dir = root_dir.resolve()

        self.config("user.name", "autofuzz")
        self.config("user.email", "autofuzz@autofuzz.com")
        self.checkout(self.branch)

        adds, mods = self.diff()
        if adds or mods:
            raise RuntimeError("Modified files found [" + self.branch + "]")

    def checkout(self, branch: str, create: bool = False):
        cmd = ["git", "checkout", branch]
        if create:
            cmd.insert(2, "-b")
        self.execute(cmd)

    def diff(self) -> Tuple[List[str], List[str]]:
        cmd = ["git", "diff", "--name-status"]
        out = self.execute(cmd)
        adds = []
        mods = []
        for line in out.splitlines():
            line = line.strip()
            if not line:
                continue
            tokens = line.split()
            if tokens[0] == "A":
                adds.append(tokens[1])
            elif tokens[0] == "M":
                mods.append(tokens[1])
        return adds, mods

    def delete_branch(self, branch: str):
        cmd = ["git", "branch", "-D", branch]
        self.execute(cmd)

    def add(self, path: Path):
        cmd = ["git", "add", path]
        self.execute(cmd)

    def commit(self, message: str):
        cmd = ["git", "commit", "-m", message]
        self.execute(cmd)

    def config(self, name: str, value: str = None) -> str:
        cmd = ["git", "config", name]
        if value:
            cmd.append(value)
        return self.execute(cmd)

    def changed(self) -> List[Path]:
        cmd = ["git", "show", "--name-status"]
        out = self.execute(cmd)
        changed_files = []
        for line in out.splitlines():
            line = line.strip()
            if not line or not line.startswith("M\t"):
                continue
            changed_files.append(self.root_dir / line.split("\t")[1])
        return changed_files

    def execute(self, cmd) -> str:
        logging.debug(cmd)
        try:
            res = subprocess.run(
                cmd,
                cwd=self.root_dir,
                check=True,
                capture_output=True,
                text=True,
            )
        except subprocess.CalledProcessError as e:
            logging.error(f"cmd: {' '.join(cmd)}")
            logging.error(f"out: {e.stdout}")
            logging.error(f"err: {e.stderr}")
            raise RuntimeError("Git command failed")
        return res.stdout
