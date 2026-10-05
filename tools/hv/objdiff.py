"""objdiff integration: target objects, the project file, and one-shot diffs."""

import json
import shutil
import subprocess
from pathlib import Path

from hv import builds, delink, symbols
from hv.elf import Elf

VERSION = "3.8.1"
TOOL = builds.ROOT / "build" / "tools" / "objdiff-cli"
PROJECT = builds.ROOT / "objdiff.json"


def unit_paths(build: str, slug: str) -> tuple[Path, Path]:
    base = builds.ROOT / "build" / "objdiff" / build
    return base / "target" / f"{slug}.o", base / "base" / f"{slug}.o"


def write_unit(build: str, unit, target: Elf, compiled: Elf, obj: Path, result: dict) -> None:
    """Write the unit's delinked target object and a copy of our object next to it."""
    target_path, base_path = unit_paths(build, unit.slug)
    known = [(s.address, s.size, s.name) for s in symbols.load(build)]
    sections, syms = delink.delink_unit(target, compiled, result, known, dict(target.fde_ranges()))
    delink.write_object(target_path, sections, syms)
    base_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(obj, base_path)


def write_project(build: str, units) -> None:
    """objdiff.json at the repository root, for objdiff-cli and the objdiff GUI."""
    entries = []
    for unit in units:
        target_path, base_path = unit_paths(build, unit.slug)
        if target_path.exists() and base_path.exists():
            entries.append(
                {
                    "name": unit.source.removesuffix(".cpp"),
                    "target_path": str(target_path.relative_to(builds.ROOT)),
                    "base_path": str(base_path.relative_to(builds.ROOT)),
                    "metadata": {"source_path": f"src/{unit.source}"},
                }
            )
    project = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_target": False,
        "build_base": False,
        "watch_patterns": ["*.cpp", "*.h"],
        "units": entries,
    }
    PROJECT.write_text(json.dumps(project, indent=2) + "\n")


def tool() -> Path:
    if TOOL.exists():
        return TOOL
    found = shutil.which("objdiff-cli")
    if found:
        return Path(found)
    raise ValueError("objdiff-cli not found: run `just objdiff-cli`")


def diff(unit: str, symbol: str) -> dict:
    """One-shot objdiff result for one symbol of a unit (run `hv match` first)."""
    output = subprocess.run(
        [str(tool()), "diff", "-p", str(builds.ROOT), "-u", unit, "-o", "-",
         "-c", "functionRelocDiffs=data_value", symbol],
        capture_output=True, text=True, check=True,
    ).stdout  # fmt: skip
    return json.loads(output)


def render(result: dict, symbol: str, context: int = 2) -> list[str]:
    """Differing instruction rows of one symbol, target on the left, ours on the right."""
    sides = {}
    for side in ("left", "right"):
        symbols = result[side]["symbols"]
        match = next((s for s in symbols if s.get("name") == symbol), None)
        if match is None:
            raise ValueError(f"{symbol} not found on the {'target' if side == 'left' else 'base'} side")
        sides[side] = (match, symbols)

    def text(row: dict, symbols: list) -> str:
        instruction = row.get("instruction")
        if not instruction:
            return ""
        line = instruction.get("formatted", "")
        relocation = instruction.get("relocation")
        if relocation is not None:
            target = symbols[int(relocation.get("target_symbol", 0))]
            line += f"  <{target.get('demangled_name') or target.get('name')}>"
        return line

    left, right = sides["left"][0].get("instructions", []), sides["right"][0].get("instructions", [])
    rows = list(zip(left, right, strict=False))
    changed = [i for i, (a, b) in enumerate(rows) if "diff_kind" in a or "diff_kind" in b]
    lines = [f"{symbol}: {sides['right'][0].get('match_percent', 0):.2f}% ({len(changed)} rows differ)"]
    shown = sorted({j for i in changed for j in range(i - context, i + context + 1) if 0 <= j < len(rows)})
    previous = None
    for i in shown:
        if previous is not None and i != previous + 1:
            lines.append("  ...")
        a, b = rows[i]
        mark = "!" if i in changed else " "
        lines.append(f"{mark} {text(a, sides['left'][1]):<60} | {text(b, sides['right'][1])}")
        previous = i
    return lines
