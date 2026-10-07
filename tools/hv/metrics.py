"""Native objdiff scores and address-based data accounting for progress snapshots."""

import json
import math
import subprocess
from pathlib import Path

from hv import builds, delink, objdiff
from hv.elf import Elf

# The same v3.8.1 binaries pinned in justfile and the report workflow.
OBJDIFF_HASHES = {
    "98f8275c27900c4fe2248fce3af37617658be49648fa7dbb5b376371f046dfdb",
    "c8290281e82114bcc1a06ff73061110d3902a177822e750337de2537188e358f",
}


def fuzzy_capture(target: Elf, known: list, selected: list, results: list, out: Path) -> dict:
    """Score newly compiled objects, without using or changing the root objdiff project."""
    tool = objdiff.tool()
    digest = builds.sha256_file(tool)
    if digest not in OBJDIFF_HASHES:
        raise ValueError("objdiff executable differs from the pinned v3.8.1 release")
    project = out / "fuzzy"
    project.mkdir(parents=True, exist_ok=True)
    entries = []
    extents = {address: size for address, (size, _) in target.function_extents().items()}
    for unit, result in zip(selected, results, strict=True):
        obj = out / f"{unit.slug}.o"
        if builds.sha256_file(obj) != result["object_sha256"]:
            raise ValueError("compiled object changed before fuzzy capture")
        sections, symbols = delink.delink_unit(target, Elf.load(obj, "ET_REL"), result, known, extents)
        destination = project / f"{unit.slug}.o"
        delink.write_object(destination, sections, symbols)
        result["fuzzy_target_sha256"] = builds.sha256_file(destination)
        entries.append({"name": unit.source, "target_path": str(destination), "base_path": str(obj)})
    config = {"build_target": False, "build_base": False, "units": entries}
    path = project / "objdiff.json"
    path.write_text(json.dumps(config, indent=2) + "\n")
    output = project / "report.json"
    subprocess.run(
        [str(tool), "report", "generate", "-p", str(project), "-c", "functionRelocDiffs=data_value",
         "-o", str(output)],
        check=True, capture_output=True, text=True,
    )  # fmt: skip
    report = json.loads(output.read_text())
    by_unit = {u["name"]: u for u in report["units"]}
    if len(by_unit) != len(selected) or set(by_unit) != {u.source for u in selected}:
        raise ValueError("objdiff report unit inventory differs from capture")
    for result in results:
        scores = {f["name"]: f for f in by_unit[result["unit"]].get("functions", [])}
        measured = []
        for section in result["sections"]:
            for function in section.get("functions", []):
                address = int(function["address"], 0)
                if address not in extents:
                    continue
                score = scores.get(function["symbol"])
                if score is None or int(score["size"]) != extents[address]:
                    raise ValueError(f"objdiff lacks the target extent of {function['symbol']}")
                percent = score.get("fuzzy_match_percent", 0)
                if not math.isfinite(percent) or not 0 <= percent <= 100:
                    raise ValueError("invalid objdiff fuzzy score")
                measured.append(
                    {
                        "address": hex(address),
                        "size": extents[address],
                        "symbol": function["symbol"],
                        "percent": percent,
                    }
                )
        result["fuzzy_functions"] = measured
    if builds.sha256_file(tool) != digest:
        raise ValueError("objdiff executable changed during capture")
    return {
        "version": objdiff.VERSION,
        "sha256": digest,
        "config": {"functionRelocDiffs": "data_value"},
        "report_sha256": builds.sha256_file(output),
    }


def fuzzy_functions(functions: list[dict], evidence: dict) -> dict[int, dict]:
    extents = {f["address"]: f["size"] for f in functions}
    scores = {}
    for unit in evidence["units"]:
        for score in unit.get("fuzzy_functions", []):
            address, percent = int(score["address"], 0), score["percent"]
            if extents.get(address) != score["size"]:
                raise ValueError("fuzzy function lacks its inventoried target extent")
            if not math.isfinite(percent) or not 0 <= percent <= 100:
                raise ValueError("invalid fuzzy percentage")
            if address not in scores or scores[address]["percent"] < percent:
                scores[address] = {**score, "source": unit["unit"]}
    return scores


def validate_fuzzy(functions: list[dict], evidence: dict) -> None:
    """Require one native score for every captured function with an inventoried target address."""
    extents = {f["address"]: f["size"] for f in functions}
    for unit in evidence["units"]:
        digest = unit["fuzzy_target_sha256"]
        if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            raise ValueError("invalid fuzzy target object hash")
        expected = {
            (f["symbol"], int(f["address"], 0), extents[int(f["address"], 0)])
            for section in unit["sections"]
            for f in section.get("functions", [])
            if int(f["address"], 0) in extents
        }
        actual = [(f["symbol"], int(f["address"], 0), f["size"]) for f in unit["fuzzy_functions"]]
        if len(actual) != len(set(actual)) or set(actual) != expected:
            raise ValueError("fuzzy score inventory differs from captured functions")
    fuzzy_functions(functions, evidence)


def data_ranges(inv: dict, evidence: dict) -> dict[str, list[tuple[int, int]]]:
    """Union complete verified data extents, including shared/overlapping constants and strings."""
    sections = inv.get("data_sections", [])
    ranges = {s["name"]: [] for s in sections}
    for unit in evidence["units"]:
        for section in unit["sections"]:
            for row in section.get("data_ranges", []):
                start, size = int(row["address"], 0), row["size"]
                owner = next(
                    (
                        s
                        for s in sections
                        if s["address"] <= start and start + size <= s["address"] + s["size"]
                    ),
                    None,
                )
                if size <= 0 or owner is None:
                    raise ValueError("matched data range lies outside the data inventory")
                if row["kind"] not in {"section", "bss", "merged string", "merged constant", "local copy"}:
                    raise ValueError("unknown data proof kind")
                if row["kind"] == "bss" and owner.get("type") != "SHT_NOBITS":
                    raise ValueError("BSS proof lies in file-backed data")
                if row["kind"] in {"section", "bss"} and not section["exact"]:
                    raise ValueError("matched section has inexact evidence")
                if row["kind"] in {"section", "bss"} and (
                    start != int(section["address"], 0) or size != section["size"]
                ):
                    raise ValueError("matched section extent differs from its evidence")
                if row["kind"] != "bss" and owner.get("type") == "SHT_NOBITS":
                    raise ValueError("file-backed data proof lies in BSS")
                ranges[owner["name"]].append((start, start + size))
    for name, spans in ranges.items():
        merged = []
        for start, end in sorted(spans):
            if merged and start <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
            else:
                merged.append((start, end))
        ranges[name] = merged
    return ranges
