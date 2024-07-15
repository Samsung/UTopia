from helper.git import Git
from pathlib import Path
import os
import shutil


def clean_directory(dir_path: str) -> bool:
    if os.path.exists(dir_path):
        if not os.path.isdir(dir_path):
            return False
        shutil.rmtree(dir_path)
    os.makedirs(dir_path)


def prepare_git(git: Git, name: str, dst: Path, src: Path):
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
