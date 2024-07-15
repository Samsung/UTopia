import logging
import os
import re
import subprocess
import tempfile
from pathlib import Path
from typing import List

from helper.buildparser import BuildParser, CMD
from helper.common import util


def emit(command: CMD, opts: str, emit_type: str, output_dir: str) -> Path:
    emit_opt = "-emit-"
    if emit_type == "ast":
        emit_opt = emit_opt + "ast"
    elif emit_type == "bc":
        emit_opt = emit_opt + "llvm"

    cmd = command.cmd
    cmd = re.compile(r"^(clang\+\+|clang) ").sub(rf"\g<1> {emit_opt} ", cmd)
    cmd = f"{cmd} {opts}"

    output = command.output(False)
    toutput = Path(output)
    if toutput.is_absolute():
        toutput = toutput.relative_to(command.path)

    eoutput: Path = (Path(output_dir) / f"{toutput}.{emit_type}").absolute()
    eoutput.parent.mkdir(exist_ok=True, parents=True)

    cmd = (
        f"cmd -o {eoutput}"
        if str(output) not in cmd
        else cmd.replace(f" {output}", f" {eoutput}")
    )

    subprocess.run(cmd, cwd=command.path, check=True, shell=True)

    assert eoutput.exists()
    return eoutput


def emit_ast(cmd: CMD, opts: str, output_dir: str) -> Path:
    return emit(cmd, opts, "ast", output_dir)


def emit_bc(cmd: CMD, opts: str, output_dir: str) -> Path:
    return emit(cmd, opts, "bc", output_dir)


def emit_linked_bc(build_path: Path, output_dir: str, name: str, bcs: List[Path]):
    output_path: str = os.path.join(output_dir, f"{name}.bc")
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    cmd = "llvm-link " + " ".join(str(bc) for bc in bcs) + f" -o {output_path}"
    subprocess.run(cmd, shell=True, cwd=build_path, check=True)
    return output_path


def build_ast_ir(
    path: Path,
    src_path: Path,
    build_path: Path,
    lib_name: str,
    target_name: str,
    ast_dir: str,
    bc_dir: str,
    output_path: str,
):
    key = os.path.basename(lib_name)
    logging.info(f"Create Build DB [{key}]")

    bp = BuildParser(path, build_path)

    opts = "-O0 -g"
    # TODO: seems not needed
    # if buildtype == 'target':
    #     opts = '-O1 -Xclang -disable-llvm-passes'

    srcs, compiles, links, assems = bp.parse()

    targets = [
        link_target
        for link_target in links.keys()
        if os.path.basename(link_target) == os.path.basename(target_name)
    ]
    if len(targets) != 1:
        raise RuntimeError(f"Ambiguous Target: {target_name}")
    target = targets[0]

    try:
        link_cmd = links[target]
    except KeyError:
        raise RuntimeError(f"Not Found Binary (Binary: {target_name})")

    ast_files, bcs = [], []
    ast_dir: str = os.path.join(ast_dir, key)

    if util.clean_directory(ast_dir) == False:
        raise RuntimeError(f"Failed to delete directory [{ast_dir}]")

    with tempfile.TemporaryDirectory() as temp_dir:
        for obj in link_cmd.objects():
            if obj in assems:
                continue
            if obj not in compiles:
                logging.warning(f"Not Found Compile Command For [{obj}]")
                continue
            compile_cmd = compiles[obj]
            ast_files.append(emit_ast(compile_cmd, opts, ast_dir))
            bcs.append(emit_bc(compile_cmd, opts, temp_dir))
        bc = emit_linked_bc(build_path, bc_dir, key, bcs)
        bp.make_build_db(key, src_path, bc, ast_files, output_path)
