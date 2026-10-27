"""Native objdiff scores and address-based data accounting for progress snapshots."""

import json
import math
import re
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


def _data_claims(inv: dict, evidence: dict) -> list[dict]:
    """Validated data ranges paired with the evidence unit section that recorded each one."""
    sections = inv.get("data_sections", [])
    claims = []
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
                claims.append(
                    {
                        "unit": unit["unit"],
                        "section": section["name"],
                        "target": owner["name"],
                        "kind": row["kind"],
                        "start": start,
                        "end": start + size,
                    }
                )
    return claims


def data_ranges(inv: dict, evidence: dict) -> dict[str, list[tuple[int, int]]]:
    """Union complete verified data extents, including shared/overlapping constants and strings."""
    ranges = {s["name"]: [] for s in inv.get("data_sections", [])}
    for claim in _data_claims(inv, evidence):
        ranges[claim["target"]].append((claim["start"], claim["end"]))
    for name, spans in ranges.items():
        merged = []
        for start, end in sorted(spans):
            if merged and start <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
            else:
                merged.append((start, end))
        ranges[name] = merged
    return ranges


# Object sections named for the symbol they define (COMDAT vtables, typeinfo and clones): every unit
# that emits one places the same target bytes, so the section itself is the owner, not one unit.
_SYMBOL_SECTION = re.compile(r"^\.(?:data\.rel\.ro|text|rodata|data|bss|gcc_except_table)\._")


def data_owners(inv: dict, evidence: dict) -> list[dict]:
    """Assign every unioned matched data byte to one stable owner for report grouping.

    A placed section (kinds section and bss) owns the bytes it defines, ahead of units that only
    reference a merged string or constant inside them; the unit and evidence section that placed it
    are the owner, unless the section is named for a symbol, in which case the shared section owns
    its bytes whichever unit emitted it. Merged strings, merged constants and local copies that no
    placed section covers belong to no single referencing unit, so they form one group per target
    section. Ties between placed sections go to the earliest claim in evidence order. Each record
    aggregates one owner in one target section: total assigned size and its lowest address.
    """
    groups = {}
    order = {}
    for claim in _data_claims(inv, evidence):
        if claim["kind"] in {"section", "bss"}:
            group = (
                ("section", claim["section"])
                if _SYMBOL_SECTION.match(claim["section"])
                else (
                    "unit",
                    claim["unit"],
                    claim["section"],
                )
            )
            placed = True
        else:
            group, placed = ("merged",), False
        key = (group, claim["target"])
        order.setdefault(key, len(order))
        record = groups.setdefault(
            key,
            {
                "owner": group,
                "target": claim["target"],
                "placed": placed,
                "unit": claim["unit"] if group[0] != "merged" else None,
                "spans": [],
            },
        )
        record["spans"].append((claim["start"], claim["end"]))
    spans = []
    for key, record in groups.items():
        merged = []
        for start, end in sorted(record["spans"]):
            if merged and start <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
            else:
                merged.append((start, end))
        spans.extend((record["target"], start, end, key) for start, end in merged)
    assigned = {}
    for section in inv.get("data_sections", []):
        target = [(start, end, key) for name, start, end, key in spans if name == section["name"]]
        if not target:
            continue
        target.sort()
        bounds = sorted({point for start, end, _ in target for point in (start, end)})
        active, index = [], 0
        for start, end in zip(bounds, bounds[1:], strict=False):
            while index < len(target) and target[index][0] <= start:
                active.append(target[index])
                index += 1
            active = [span for span in active if span[1] >= end]
            if not active:
                continue
            key = min(
                (span[2] for span in active),
                key=lambda k: (not groups[k]["placed"], order[k]),
            )
            record = assigned.setdefault(key, {"size": 0, "address": start})
            record["size"] += end - start
            record["address"] = min(record["address"], start)
    return [
        {
            "owner": groups[key]["owner"],
            "target": groups[key]["target"],
            "unit": groups[key]["unit"],
            "size": record["size"],
            "address": record["address"],
        }
        for key, record in assigned.items()
    ]
