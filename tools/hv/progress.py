"""Capture local matching evidence; publish fresh objdiff-v2 progress without originals in CI."""

import argparse
import bisect
import csv
import hashlib
import io
import json
import re
import subprocess
import sys
import tomllib
from pathlib import Path

from hv import builds, metrics, units

SCHEMA = 2


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def paths(build: str) -> Path:
    return builds.REFERENCE / build / "progress"


def measurement_paths(build: str) -> list[str]:
    return [
        "builds.json",
        "uv.lock",
        "toolchain/manifest.tsv",
        *[
            f"tools/hv/{name}.py"
            for name in (
                "builds",
                "elf",
                "match",
                "symbols",
                "toolchain",
                "units",
                "progress",
                "metrics",
                "objdiff",
                "delink",
            )
        ],
        *[f"config/{build}/{name}" for name in ("flags.json", "symbols.tsv", "units.toml")],
    ]


def input_hashes(names: list[str]) -> dict[str, str]:
    result = {}
    for name in names:
        path = (builds.ROOT / name).resolve()
        if Path(name).is_absolute() or not path.is_relative_to(builds.ROOT):
            raise ValueError(f"input outside repository: {name}")
        result[name] = builds.sha256_file(path)
    return result


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def inventory(target, image) -> tuple[dict, str]:
    sections = [
        {"name": s.name, "address": s["sh_addr"], "size": s["sh_size"], "sha256": sha(s.data())}
        for s in target.elf.iter_sections()
        if s["sh_flags"] & 6 == 6 and s["sh_size"]
    ]
    data_sections = [
        {
            "name": s.name,
            "address": s["sh_addr"],
            "size": s["sh_size"],
            "type": s["sh_type"],
            "sha256": sha(s.data()),
        }
        for s in target.elf.iter_sections()
        if s["sh_flags"] & 2 and not s["sh_flags"] & 4 and s["sh_size"]
    ]
    rows = []
    end = 0
    for address, size in sorted(target.fde_ranges()):
        owner = next(
            (s for s in sections if s["address"] <= address and address + size <= s["address"] + s["size"]),
            None,
        )
        if owner is None or size <= 0 or address < end:
            raise ValueError(f"invalid/overlapping executable FDE: {address:#x}+{size}")
        rows.append((address, size, owner["name"]))
        end = address + size
    if not rows:
        raise ValueError("no executable FDEs")
    table = "address\tsize\tsection\n" + "".join(f"{a:#x}\t{n}\t{s}\n" for a, n, s in rows)
    return {
        "schema": SCHEMA,
        "build": image.build,
        "image": image.name,
        "image_sha256": image.sha256,
        "scope": "all allocated executable ELF sections; FDE bodies plus all remaining bytes",
        "sections": sections,
        "data_sections": data_sections,
        "data_scope": "all allocated non-executable ELF sections, including BSS and linker metadata",
        "total_data": sum(s["size"] for s in data_sections),
        "functions_sha256": sha(table.encode()),
        "total_code": sum(s["size"] for s in sections),
        "total_functions": len(rows),
    }, table


def capture(build: str) -> dict:
    from hv import match, symbols, toolchain
    from hv.elf import Elf

    (image,) = builds.load_builds()[build].images.values()
    if problem := builds.check_image(image):
        raise ValueError(f"{image.path}: {problem}")
    before = input_hashes(measurement_paths(build))
    source_names = [
        str(p.relative_to(builds.ROOT))
        for root in ("src", "third_party")
        for p in (builds.ROOT / root).rglob("*")
        if p.is_file()
    ]
    source_before = input_hashes(source_names)
    target = Elf.load(image.path, "ET_EXEC")
    inv, table = inventory(target, image)
    selected = units.load(build)
    out = builds.ROOT / "build" / "progress" / build
    compiler = toolchain.Compiler(toolchain.load_flags(build), out)
    known = symbols.by_name(symbols.load(build))
    results = []
    for unit in selected:
        obj, metadata = compiler.compile(unit.path, unit.slug)
        if any(
            source_before.get(str((builds.ROOT / p).resolve().relative_to(builds.ROOT))) != h
            for p, h in metadata["inputs"].items()
        ):
            raise ValueError(
                "source dependencies changed during compilation or lie outside src/ and third_party/"
            )
        result = match.compare_object(Elf.load(obj, "ET_REL"), target, known, unit.placements)
        results.append({"unit": unit.source, **result, "compilation": metadata})
        print(f"{'exact' if result['exact'] else 'different':10} {unit.source}")
    fuzzy = metrics.fuzzy_capture(
        target, [(s.address, s.size, s.name) for s in symbols.load(build)], selected, results, out
    )
    if input_hashes(measurement_paths(build)) != before or input_hashes(source_names) != source_before:
        raise ValueError("measurement inputs changed during capture")
    evidence = {
        "schema": SCHEMA,
        "build": build,
        "image_sha256": image.sha256,
        "measurement_inputs": before,
        "units": results,
        "fuzzy": fuzzy,
        "link": {"status": "unavailable", "reason": "no executable link step; compiled objects only"},
    }
    # Originals and compiled objects stay local. Only metadata and measured outcomes are committed.
    dest = paths(build)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "functions.tsv").write_text(table)
    write_json(dest / "inventory.json", inv)
    evidence["inventory_sha256"] = builds.sha256_file(dest / "inventory.json")
    write_json(dest / "evidence.json", evidence)
    return evidence


def validate(build: str) -> tuple[dict, list[dict], dict]:
    dest = paths(build)
    inv = json.loads((dest / "inventory.json").read_text())
    evidence = json.loads((dest / "evidence.json").read_text())
    (image,) = builds.load_builds()[build].images.values()
    for record in (inv, evidence):
        if record["schema"] != SCHEMA or record["build"] != build or record["image_sha256"] != image.sha256:
            raise ValueError("progress schema/build/image pin mismatch")
    if evidence["inventory_sha256"] != builds.sha256_file(dest / "inventory.json"):
        raise ValueError("inventory changed since capture")
    table = (dest / "functions.tsv").read_bytes()
    if sha(table) != inv["functions_sha256"]:
        raise ValueError("function inventory hash mismatch")
    expected = evidence["measurement_inputs"]
    current = input_hashes(measurement_paths(build))
    if expected != current:
        changed = sorted(k for k in expected.keys() | current.keys() if expected.get(k) != current.get(k))
        raise ValueError(f"stale measurement inputs: {', '.join(changed)}; run progress capture")
    selected = {u.source for u in units.load(build)}
    measured = [u["unit"] for u in evidence["units"]]
    if len(set(measured)) != len(measured) or selected != set(measured):
        raise ValueError("captured units differ from units.toml; run progress capture")
    for unit in evidence["units"]:
        compilation = unit["compilation"]
        inputs = compilation["inputs"]
        if f"src/{unit['unit']}" not in inputs or input_hashes(list(inputs)) != inputs:
            raise ValueError(f"stale source/header evidence: {unit['unit']}; run progress capture")
        if compilation["object_sha256"] != unit["object_sha256"]:
            raise ValueError("compiled object identity mismatch")
        if compilation["package_manifest_sha256"] != current["toolchain/manifest.tsv"]:
            raise ValueError("toolchain manifest mismatch")
        flags = json.loads((builds.ROOT / "config" / build / "flags.json").read_text())
        if compilation["compiler_version"] != flags["version"]:
            raise ValueError("compiler version mismatch")
    if (
        evidence["fuzzy"]["version"] != "3.8.1"
        or evidence["fuzzy"]["sha256"] not in metrics.OBJDIFF_HASHES
        or evidence["fuzzy"]["config"] != {"functionRelocDiffs": "data_value"}
    ):
        raise ValueError("fuzzy measurement tool/config mismatch")
    if evidence["link"] != {
        "status": "unavailable",
        "reason": "no executable link step; compiled objects only",
    }:
        raise ValueError("unsupported linked evidence")
    sections = {s["name"]: s for s in inv["sections"]}
    ordered = sorted(sections.values(), key=lambda s: s["address"])
    if len(sections) != len(inv["sections"]) or not ordered:
        raise ValueError("empty or duplicate executable sections")
    end = 0
    for section in ordered:
        if section["size"] <= 0 or section["address"] < end:
            raise ValueError("invalid/overlapping executable sections")
        end = section["address"] + section["size"]
    functions = []
    end = 0
    for row in csv.DictReader(io.StringIO(table.decode()), delimiter="\t"):
        address, size = int(row["address"], 0), int(row["size"])
        section = sections[row["section"]]
        if (
            size <= 0
            or address < end
            or address < section["address"]
            or address + size > section["address"] + section["size"]
        ):
            raise ValueError("invalid/overlapping function inventory")
        functions.append({"address": address, "size": size, "section": row["section"]})
        end = address + size
    if len(functions) != inv["total_functions"] or sum(s["size"] for s in ordered) != inv["total_code"]:
        raise ValueError("inventory totals disagree")
    data = inv["data_sections"]
    end = 0
    for section in sorted([*ordered, *data], key=lambda s: s["address"]):
        if section["size"] <= 0 or section["address"] < end:
            raise ValueError("invalid/overlapping code/data sections")
        end = section["address"] + section["size"]
    if len({s["name"] for s in data}) != len(data) or sum(s["size"] for s in data) != inv["total_data"]:
        raise ValueError("data inventory totals disagree")
    metrics.validate_fuzzy(functions, evidence)
    metrics.data_ranges(inv, evidence)
    return inv, functions, evidence


def measured_functions(functions: list[dict], evidence: dict) -> dict[int, dict]:
    """Credit each function whose body matched at its own target address.

    A function earns credit when the matcher found its bytes and every reference in it exact and its
    extent equal to the inventoried FDE, whether or not its unit matched as a whole (the unit can
    still differ in function order or in other functions). An exact unit must be exact throughout.
    """
    extents = {f["address"]: f["size"] for f in functions}
    matched = {}
    for unit in evidence["units"]:
        if unit["exact"] and (unit["unplaced_sections"] or any(not s["exact"] for s in unit["sections"])):
            raise ValueError("inconsistent exact-unit evidence")
        for section in unit["sections"]:
            for function in section.get("functions", []):
                if not function["exact"]:
                    if unit["exact"]:
                        raise ValueError("inconsistent exact-unit evidence")
                    continue
                address = int(function["address"], 0)
                if not function["fde"] or extents.get(address) != function["size"]:
                    raise ValueError("matched function lacks its complete inventoried FDE extent")
                row = matched.setdefault(address, {"symbol": function["symbol"], "sources": []})
                if unit["unit"] not in row["sources"]:
                    row["sources"].append(unit["unit"])
    return matched


def measures(
    code: int,
    matched: int,
    functions: int,
    matched_functions: int,
    unit_count: int,
    *,
    fuzzy_code: float | None = None,
    data: int = 0,
    matched_data: int = 0,
) -> dict:
    percent = 100 * matched / code if code else 0
    fuzzy_code = matched if fuzzy_code is None else fuzzy_code
    return {
        "fuzzy_match_percent": 100 * (fuzzy_code + matched_data) / (code + data) if code + data else 0,
        "total_code": str(code),
        "matched_code": str(matched),
        "matched_code_percent": percent,
        "total_data": str(data),
        "matched_data": str(matched_data),
        "matched_data_percent": 100 * matched_data / data if data else 0,
        "total_functions": functions,
        "matched_functions": matched_functions,
        "matched_functions_percent": 100 * matched_functions / functions if functions else 0,
        "complete_code": "0",
        "complete_code_percent": 0,
        "complete_data": "0",
        "complete_data_percent": 0,
        "total_units": unit_count,
        "complete_units": 0,
    }


def load_layers(build: str) -> dict | None:
    """The port-relevance layer config (config/<build>/layers.toml), or None when the build has none."""
    path = builds.ROOT / "config" / build / "layers.toml"
    if not path.exists():
        return None
    return tomllib.loads(path.read_text())


def assign_layers(functions: list[dict], names: dict[int, str], config: dict) -> dict[int, str]:
    """Each function's layer: the first matching rule, else the closest earlier named function in its range,
    else the range's own layer. Copies (template and inline code a unit emits beside its own) take a rule's
    layer or the inherited one, and never become what later functions inherit."""
    rules = [(re.compile(rule["pattern"]), rule["layer"]) for rule in config.get("rule", [])]
    copies = [re.compile(pattern) for pattern in config.get("copies", [])]
    ranges = sorted((r["start"], r["end"], r["layer"]) for r in config["range"])
    starts = [start for start, _, _ in ranges]
    result = {}
    current_range, inherited = None, None
    for function in sorted(functions, key=lambda f: f["address"]):
        address = function["address"]
        index = bisect.bisect_right(starts, address) - 1
        if index < 0 or address >= ranges[index][1]:
            raise ValueError(f"function {address:#x} lies outside every layer range")
        if index != current_range:
            current_range, inherited = index, ranges[index][2]
        name = names.get(address)
        if name is None:
            result[address] = inherited
            continue
        ruled = next((layer for pattern, layer in rules if pattern.search(name)), None)
        if any(pattern.search(name) for pattern in copies):
            result[address] = ruled or inherited
        else:
            inherited = result[address] = ruled or ranges[index][2]
    if unknown := set(result.values()) - set(config["layers"]):
        raise ValueError(f"undefined layers: {', '.join(sorted(unknown))}")
    return result


def make_report(
    inv: dict,
    functions: list[dict],
    evidence: dict,
    names: dict[int, str],
    demangled: dict[str, str],
    layers: dict | None = None,
) -> dict:
    matched = measured_functions(functions, evidence)
    fuzzy = metrics.fuzzy_functions(functions, evidence)
    data = metrics.data_ranges(inv, evidence)
    layer_of = assign_layers(functions, names, layers) if layers else {}
    layer_totals = {layer: [0, 0, 0, 0, 0.0] for layer in layers["layers"]} if layers else {}
    fuzzy_code = 0.0
    report_units = []
    covered = dict.fromkeys((s["name"] for s in inv["sections"]), 0)
    for function in functions:
        address, size = function["address"], function["size"]
        proof = matched.get(address)
        score = fuzzy.get(address)
        percent = 100 if proof else score["percent"] if score else 0
        fuzzy_code += size * percent / 100
        symbol = (
            proof["symbol"] if proof else score["symbol"] if score else names.get(address, f"sub_{address:x}")
        )
        display = demangled.get(symbol, symbol)
        metadata = {"complete": False, "progress_categories": ["functions"]}
        if layer := layer_of.get(address):
            metadata["progress_categories"].append(layer)
            totals = layer_totals[layer]
            totals[0] += size
            totals[1] += size if proof else 0
            totals[2] += 1
            totals[3] += int(bool(proof))
            totals[4] += size * percent / 100
        if proof:
            metadata["source_path"] = f"src/{proof['sources'][0]}"
        elif score:
            metadata["source_path"] = f"src/{score['source']}"
        report_units.append(
            {
                "name": f"{display} @ {address:#x}",
                "measures": measures(
                    size, size if proof else 0, 1, int(bool(proof)), 1, fuzzy_code=size * percent / 100
                ),
                "functions": [
                    {
                        "name": symbol,
                        "size": str(size),
                        "fuzzy_match_percent": percent,
                        "metadata": {"demangled_name": display, "virtual_address": str(address)},
                    }
                ],
                "metadata": metadata,
            }
        )
        covered[function["section"]] += size
    gaps = []
    for section in inv["sections"]:
        remaining = section["size"] - covered[section["name"]]
        if remaining:
            gaps.append(
                {
                    "name": f"Unclaimed {section['name']} bytes (includes padding/stubs)",
                    "measures": measures(remaining, 0, 0, 0, 1),
                    "metadata": {"complete": False, "progress_categories": ["unclaimed"]},
                }
            )
    matched_code = sum(f["size"] for f in functions if f["address"] in matched)
    function_code = sum(f["size"] for f in functions)
    data_units = []
    for section in inv.get("data_sections", []):
        spans = data[section["name"]]
        cursor = section["address"]
        for start, end in [*spans, (cursor + section["size"], cursor + section["size"])]:
            if start > cursor:
                size = start - cursor
                data_units.append(
                    {
                        "name": f"Unclaimed {section['name']} data @ {cursor:#x}",
                        "measures": measures(0, 0, 0, 0, 1, data=size),
                        "metadata": {"complete": False, "progress_categories": ["data"]},
                    }
                )
            if end > start:
                size = end - start
                data_units.append(
                    {
                        "name": f"Matched {section['name']} data @ {start:#x}",
                        "measures": measures(0, 0, 0, 0, 1, data=size, matched_data=size),
                        "sections": [
                            {
                                "name": section["name"],
                                "size": str(size),
                                "fuzzy_match_percent": 100,
                                "metadata": {"virtual_address": str(start)},
                            }
                        ],
                        "metadata": {"complete": False, "progress_categories": ["data"]},
                    }
                )
            cursor = end
    matched_data = sum(end - start for spans in data.values() for start, end in spans)
    total_data = inv.get("total_data", 0)
    return {
        "version": 2,
        "measures": measures(
            inv["total_code"],
            matched_code,
            len(functions),
            len(matched),
            len(report_units) + len(gaps) + len(data_units),
            fuzzy_code=fuzzy_code,
            data=total_data,
            matched_data=matched_data,
        ),
        "units": report_units + gaps + data_units,
        "categories": [
            {
                "id": "functions",
                "name": "FDE function bodies",
                "measures": measures(
                    function_code,
                    matched_code,
                    len(functions),
                    len(matched),
                    len(report_units),
                    fuzzy_code=fuzzy_code,
                ),
            },
            {
                "id": "data",
                "name": "Allocated data (including BSS and linker metadata)",
                "measures": measures(0, 0, 0, 0, len(data_units), data=total_data, matched_data=matched_data),
            },
            {
                "id": "unclaimed",
                "name": "Unclaimed executable bytes",
                "measures": measures(inv["total_code"] - function_code, 0, 0, 0, len(gaps)),
            },
            *(
                {
                    "id": layer,
                    "name": layers["layers"][layer]["name"],
                    "measures": measures(
                        code, matched_bytes, count, matched_count, count, fuzzy_code=fuzzy_bytes
                    ),
                }
                for layer, (code, matched_bytes, count, matched_count, fuzzy_bytes) in layer_totals.items()
            ),
        ],
    }


def export(build: str, output: Path) -> dict:
    from hv import symbols

    inv, functions, evidence = validate(build)
    names = {}
    for symbol in symbols.load(build):
        names.setdefault(symbol.address, symbol.name)
    demangled = {}
    reference = builds.REFERENCE / "1.18-mac-i386" / "functions.csv"
    with reference.open() as stream:
        demangled = {row["symbol"]: row["name"] for row in csv.DictReader(stream)}
    report = make_report(inv, functions, evidence, names, demangled, load_layers(build))
    write_json(output, report)
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("capture", "report"))
    parser.add_argument("--build", default=builds.canonical_build())
    parser.add_argument("--output", type=Path, default=builds.ROOT / "build/progress/report.json")
    args = parser.parse_args(argv)
    try:
        if args.command == "capture":
            capture(args.build)
        report = export(args.build, args.output)
        m = report["measures"]
        print(
            f"{m['matched_code']}/{m['total_code']} bytes ({m['matched_code_percent']:.5f}%), "
            f"{m['matched_functions']}/{m['total_functions']} functions -> {args.output}"
        )
        for category in report["categories"][3:]:
            c = category["measures"]
            print(
                f"  {category['id']:8} {c['matched_code']:>7}/{c['total_code']:>7} bytes "
                f"({c['matched_code_percent']:6.2f}%), "
                f"{c['matched_functions']}/{c['total_functions']} functions"
            )
        print(
            f"fuzzy {m['fuzzy_match_percent']:.5f}%; "
            f"data {m['matched_data']}/{m['total_data']} ({m['matched_data_percent']:.5f}%); "
            "linked code 0%, linked data 0% (no executable link step)"
        )
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"progress: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr.rstrip(), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
