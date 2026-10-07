import argparse
import json
import subprocess
import sys
from pathlib import Path

from hv import builds


def cmd_match(args: argparse.Namespace) -> int:
    from elftools.common.exceptions import ELFError

    from hv import extents, match, objdiff, symbols, toolchain, units
    from hv.elf import Elf

    try:
        build = builds.load_builds()[args.build]
        (image,) = build.images.values()
        if problem := builds.check_image(image):
            raise ValueError(f"{image.path}: {problem}")
        selected = units.load(build.key)
        if args.units:
            known_sources = {u.source for u in selected}
            if missing := [u for u in args.units if u not in known_sources]:
                raise ValueError(f"not in units.toml: {', '.join(missing)}")
            selected = [u for u in selected if u.source in args.units]
        if args.object and len(selected) != 1:
            raise ValueError("--object needs exactly one unit")
        target = extents.load_target(build.key)
        known = symbols.by_name(symbols.load(build.key))
        out = builds.ROOT / "build" / "match" / build.key
        compiler = None if args.object else toolchain.Compiler(toolchain.load_flags(build.key), out)
        failed = False
        for unit in selected:
            if compiler is None:
                obj, metadata = args.object, {"note": "supplied object; compiler and source not verified"}
            else:
                obj, metadata = compiler.compile(unit.path, unit.slug)
            compiled = Elf.load(obj, "ET_REL")
            result = match.compare_object(compiled, target, known, unit.placements)
            # functions that match once their references are learned can prove more addresses
            while args.learn:
                learned, conflicts = match.learnable(result, unit.source, known)
                if not learned:
                    break
                add_learned(build.key, learned, target)
                known = symbols.by_name(symbols.load(build.key))
                result = match.compare_object(compiled, target, known, unit.placements)
                print(f"learned    {len(learned)} symbol addresses from {unit.source}")
            if args.learn:
                for name in conflicts:
                    print(f"conflict   {name}: references imply different addresses", file=sys.stderr)
            result = {"unit": unit.source, "image_sha256": image.sha256, **result, "compilation": metadata}
            report = out / f"{unit.slug}.json"
            report.parent.mkdir(parents=True, exist_ok=True)
            report.write_text(json.dumps(result, indent=2) + "\n")
            objdiff.write_unit(build.key, unit, target, compiled, Path(obj), result)
            functions = [f for s in result["sections"] for f in s.get("functions", [])]
            exact_functions = sum(f["exact"] for f in functions)
            status = "exact" if result["exact"] else "different"
            thunks = sum(f["extent"] == "thunk" for f in functions)
            proven = f", {thunks} by thunk extent" if thunks else ""
            print(f"{status:10} {unit.source}  functions {exact_functions}/{len(functions)}{proven}")
            if args.verbose or not result["exact"]:
                print_details(result)
            failed |= not result["exact"]
        objdiff.write_project(build.key, units.load(build.key))
    except (ValueError, OSError, KeyError, ELFError, subprocess.CalledProcessError) as error:
        print(f"match: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr.rstrip(), file=sys.stderr)
        return 2
    return 1 if failed else 0


def add_learned(build: str, learned: dict, target) -> None:
    from hv import symbols

    sizes = {address: size for address, (size, _) in target.function_extents().items()}
    rows = symbols.load(build)
    rows += [
        symbols.Symbol(address, sizes.get(address, 0), name, evidence)
        for name, (address, evidence) in learned.items()
    ]
    symbols.save(build, rows)


def print_details(result: dict) -> None:
    for section in result["sections"]:
        mark = "ok" if section["exact"] else "DIFF"
        print(f"  {mark:4} {section['name']} @ {section.get('address', '?')} ({section['placement']})")
        for moved in section.get("misplaced_symbols", []):
            print(f"         {moved['symbol']} placed at {moved['placed']}, known at {moved['known']}")
        for ref in section.get("bad_references", []):
            print(f"         reference +{ref['offset']:#x} {ref['symbol']}: {ref.get('reason', '')}")
        for function in section.get("functions", []):
            if function.get("exact_but_unknown"):
                unknown = ", ".join(sorted({name for name, _ in function["candidates"]}))
                print(f"         function {function['symbol']} matches except unknown symbols: {unknown}")
            elif not function["exact"]:
                extent = function["extent"] or "no FDE or thunk extent of its size"
                print(f"         function {function['symbol']} @ {function['address']} differs ({extent})")
    for name in result["unplaced_sections"]:
        print(f"  ??   {name}: not placed (add it to units.toml or name a symbol in it)")


def cmd_diff(args: argparse.Namespace) -> int:
    from hv import objdiff

    try:
        result = objdiff.diff(args.unit.removesuffix(".cpp"), args.symbol)
        print("\n".join(objdiff.render(result, args.symbol, args.context)))
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"diff: {error}", file=sys.stderr)
        return 2
    return 0


def cmd_search(args: argparse.Namespace) -> int:
    from elftools.common.exceptions import ELFError

    from hv import search

    try:
        search.search(
            args.unit,
            None if args.blocks == "auto" else Path(args.blocks),
            args.build,
            budget=args.budget,
            restarts=args.restarts,
            sideways=args.sideways,
            seed=args.seed,
            apply=args.apply,
            batch=args.batch,
        )
    except (ValueError, OSError, KeyError, ELFError, subprocess.CalledProcessError) as error:
        print(f"search: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr.rstrip(), file=sys.stderr)
        return 2
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    known = builds.load_builds()
    if unknown := [key for key in args.build if key not in known]:
        print(f"unknown builds: {', '.join(unknown)} (known: {', '.join(known)})", file=sys.stderr)
        return 2
    failed = 0
    for key in args.build or known:
        build = known[key]
        for image in build.images.values():
            problem = builds.check_image(image)
            status = "ok" if problem is None else problem
            print(f"{build.key:18} {image.name:16} {status}")
            failed += problem is not None
    return 1 if failed else 0


def cmd_import_mac(args: argparse.Namespace) -> int:
    from hv import macref

    build = builds.load_builds()[args.build]
    (image,) = build.images.values()
    if problem := builds.check_image(image):
        print(f"{build.key} {image.name}: {problem}", file=sys.stderr)
        return 1
    out = builds.REFERENCE / build.key
    counts = macref.import_mac(image.path, out)
    print(f"{out.relative_to(builds.ROOT)}: " + ", ".join(f"{v} {k}" for k, v in counts.items()))
    return 0


def cmd_port_symbols(args: argparse.Namespace) -> int:
    from hv import rtti, symbols
    from hv.elf import Elf

    build = builds.load_builds()[args.build]
    (image,) = build.images.values()
    if problem := builds.check_image(image):
        print(f"{build.key} {image.name}: {problem}", file=sys.stderr)
        return 1
    generated, stats = rtti.port(Elf.load(image.path, "ET_EXEC"), args.reference)
    merged = symbols.replace_generated(build.key, {"rtti", "mac-vtable"}, generated)
    print(", ".join(f"{v} {k}" for k, v in stats.items()))
    print(f"{symbols.path_for(build.key).relative_to(builds.ROOT)}: {len(merged)} symbols")
    return 0


def cmd_extents(args: argparse.Namespace) -> int:
    from hv import extents, symbols

    build = builds.load_builds()[args.build]
    rows = extents.generate(build.key)
    path = extents.path_for(build.key)
    path.write_text(extents.render(rows))
    known = {s.name for s in symbols.load(build.key)}
    named = [
        symbols.Symbol(e.address, e.size, e.symbol, f"thunk:{e.evidence.split(';')[0].removeprefix('slot ')}")
        for e in rows
        if e.symbol
    ]
    merged = symbols.replace_generated(build.key, {"thunk"}, named)
    new = sum(e.symbol not in known for e in rows if e.symbol)
    unnamed = sum(not e.symbol for e in rows)
    print(f"{path.relative_to(builds.ROOT)}: {len(rows)} thunks, {sum(e.size for e in rows)} bytes")
    table = symbols.path_for(build.key).relative_to(builds.ROOT)
    print(f"{table}: {len(merged)} symbols, {new} new thunk names")
    if unnamed:
        print(f"{unnamed} thunks jump to unnamed functions and stay unnamed in extents.tsv", file=sys.stderr)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="hv", description="Harvest decompilation tooling")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("verify", help="check orig/ images against builds.json pins")
    p.add_argument("build", nargs="*", help="limit to these builds")
    p.set_defaults(func=cmd_verify)

    p = sub.add_parser("import-mac", help="write reference tables from the Mac debug map")
    p.add_argument("build", nargs="?", default="1.18-mac-i386")
    p.set_defaults(func=cmd_import_mac)

    p = sub.add_parser("extents", help="find thunk extents from the vtables and name the thunks")
    p.add_argument("build", nargs="?", default=builds.canonical_build())
    p.set_defaults(func=cmd_extents)

    p = sub.add_parser("port-symbols", help="name target RTTI, vtables and virtual functions")
    p.add_argument("build", nargs="?", default=builds.canonical_build())
    p.add_argument("--reference", default="1.18-mac-i386")
    p.set_defaults(func=cmd_port_symbols)

    p = sub.add_parser("match", help="compile recovered units and compare them with the target")
    p.add_argument("units", nargs="*", help="sources under src/ (default: all in units.toml)")
    p.add_argument("--build", default=builds.canonical_build())
    p.add_argument("--object", type=Path, help="compare an existing object for one unit")
    p.add_argument("-v", "--verbose", action="store_true", help="list sections of exact units too")
    p.add_argument(
        "--learn",
        action="store_true",
        help="add addresses of unknown symbols referenced by functions that otherwise match",
    )
    p.set_defaults(func=cmd_match)

    p = sub.add_parser("search", help="search definition orders without losing exact matches")
    p.add_argument("unit", help="source under src/ listed in units.toml")
    p.add_argument(
        "--blocks",
        default="auto",
        help="JSON source hash and named inclusive line ranges, or auto: every top-level function",
    )
    p.add_argument("--batch", type=int, default=16, help="candidates compiled per container")
    p.add_argument("--build", default=builds.canonical_build())
    p.add_argument("--budget", type=int, default=100, help="maximum unique candidate sources evaluated")
    p.add_argument("--restarts", type=int, default=2, help="maximum seeded random restarts")
    p.add_argument("--sideways", type=int, default=3, help="maximum consecutive neutral moves")
    p.add_argument("--seed", type=int, default=0)
    p.add_argument(
        "--apply", action="store_true", help="apply an improved candidate after fresh canonical verification"
    )
    p.set_defaults(func=cmd_search)

    p = sub.add_parser("diff", help="objdiff one function of a unit, target on the left (run match first)")
    p.add_argument("unit", help="source under src/, e.g. ox/net/CHTTPConnectionHandler.cpp")
    p.add_argument("symbol", help="mangled symbol name")
    p.add_argument("-C", "--context", type=int, default=2, help="rows of context around differences")
    p.set_defaults(func=cmd_diff)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
