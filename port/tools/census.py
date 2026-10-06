"""Link census: the symbols the kept units need that nothing in the port defines yet.

    uv run python port/tools/census.py            # port census (links port/ with zig, host target)
    uv run python port/tools/census.py --original # the same over the matching build's GCC objects

The port census links every kept unit (`zig build census`), takes the linker's undefined symbols and
finds, from each object's relocations, the functions that reference them. `--original` reads every
recovered unit's object from build/match/1.18-linux-amd64 (run `just match` first), including the
platform units the port replaces, and lists what the original link took from unrecovered code and
libraries. Symbols are grouped by owner with the rules below; anything unmatched lands in `other`.
"""

import argparse
import re
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "port"
MATCH = ROOT / "build" / "match" / "1.18-linux-amd64"

# (group, pattern on the demangled name); the first match wins.
GROUPS = [
    (
        "platform",
        r"^createDevice$|^daisy::os::|^daisy::C(IrrDevice|LinuxOperator|Logger)|^daisy::input::|"
        r"^daisy::audio::COpenAL|^daisy::video::(COpenGL|CVideoOpenGL|createOpenGLDriver)|"
        r"^daisy::scene::createSceneManager|^daisy::net::CWinsockNetworkDevice",
    ),
    ("unrecovered", r"^daisy::|^ox::|^harvest::|^irr::"),
    ("lua", r"^luaL?_"),
    ("zlib", r"^(deflate|inflate|compress|uncompress|crc32|adler32)"),
    ("cg", r"^cg"),
    ("libraries", r"^(gl[A-Z]|glu[A-Z]|glX|X[A-Z]|al[A-Z]|alut[A-Z]|sf::|gtk_|gdk_|ov_)"),
    ("cxxrt", r"^(std::|operator (new|delete)|vtable for|VTT for|typeinfo|__cxa|__gxx|_Unwind|__dso_handle)"),
    ("libc", r"^[a-z_][a-z0-9_]*$"),
]
# Groups listed one line per symbol, with the referencing units only.
LIBRARY_GROUPS = {"lua", "zlib", "libraries", "cxxrt", "libc"}
GROUP_TITLES = {
    "platform": "Platform seams (the replaced units, the scene manager and the network device)",
    "unrecovered": "Unrecovered daisy/ox/game code",
    "lua": "Lua 5.1 C API",
    "zlib": "zlib",
    "cg": "Cg",
    "libraries": "Platform libraries of the replaced units (OpenGL, X11, OpenAL, SFML, GTK, Vorbis)",
    "cxxrt": "C++ runtime and standard library",
    "libc": "C library and POSIX",
    "other": "Other",
}


def run(*args: str, cwd: Path | None = None) -> str:
    return subprocess.run(args, cwd=cwd, capture_output=True, text=True).stdout


def demangle(names: list[str]) -> dict[str, str]:
    out = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True).stdout
    return dict(zip(names, out.splitlines(), strict=True))


def strip_prefix(name: str, macho: bool) -> str:
    return name[1:] if macho and name.startswith("_") else name


def references(obj: Path) -> dict[str, set[str]]:
    """Map each referenced symbol to the functions of `obj` whose relocations name it."""
    bases = {}
    for line in run("objdump", "-h", str(obj)).splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[0].isdigit():
            bases[parts[1]] = int(parts[3], 16)
    functions = defaultdict(list)
    for line in run("objdump", "-t", str(obj)).splitlines():
        match = re.match(r"^([0-9a-f]{16}) (.{7}) (\S+)\s+(?:[0-9a-f]{16} )?(.+)$", line)
        if not match or "F" not in match[2] or match[4].startswith("ltmp"):
            continue
        section = match[3].split(",")[-1]
        functions[section].append((int(match[1], 16) - bases.get(section, 0), match[4].split()[-1]))
    for symbols in functions.values():
        symbols.sort()
    refs = defaultdict(set)
    section = None
    for line in run("objdump", "-r", str(obj)).splitlines():
        if line.startswith("RELOCATION RECORDS FOR ["):
            section = line.split("[")[1].rstrip("]:")
            continue
        parts = line.split()
        if section not in functions or len(parts) != 3 or not re.fullmatch(r"[0-9a-f]{16}", parts[0]):
            continue
        offset, target = int(parts[0], 16), re.sub(r"[-+]0x[0-9a-f]+$", "", parts[2])
        owner = None
        for start, name in functions[section]:
            if start > offset:
                break
            owner = name
        if owner:
            refs[target].add(owner)
    return refs


def defined_and_undefined(objs: list[Path]) -> tuple[set[str], set[str]]:
    defined, undefined = set(), set()
    for obj in objs:
        for line in run("nm", str(obj)).splitlines():
            parts = line.split()
            if len(parts) == 2 and parts[0] in "Uw":
                undefined.add(parts[1])
            elif len(parts) == 3 and parts[1] not in "Uw":
                defined.add(parts[2])
    return defined, undefined


def port_census(work: Path) -> tuple[list[Path], set[str], bool]:
    build = subprocess.run(["zig", "build", "install", "census"], cwd=PORT, capture_output=True, text=True)
    missing = set(re.findall(r"undefined symbol: (\S+)", build.stderr))
    if build.returncode and not missing:
        raise SystemExit(build.stderr)
    subprocess.run(["ar", "x", str(PORT / "zig-out" / "lib" / "libharvest.a")], cwd=work, check=True)
    objs = sorted(work.glob("*.o"))
    for obj in objs:
        obj.chmod(0o644)  # zig writes archive members with mode 000
    return objs, missing, True


def original_census() -> tuple[list[Path], set[str], bool]:
    objs = sorted(MATCH.glob("*.o"))
    if not objs:
        raise SystemExit(f"no objects in {MATCH}; run `just match` first")
    defined, undefined = defined_and_undefined(objs)
    return objs, undefined - defined, False


def unit_names() -> dict[str, str]:
    """Object stem -> unit, for both the port's archive members and the matching build's objects."""
    text = (ROOT / "config" / "1.18-linux-amd64" / "units.toml").read_text()
    names = {}
    for unit in re.findall(r'^\["(.+)\.cpp"\]', text, re.M):
        names[Path(unit).name] = names[unit.replace("/", "__")] = unit
    return names


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--original", action="store_true", help="census of the matching build's GCC objects")
    parser.add_argument("--libc", action="store_true", help="also list C library calls the port links")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        objs, missing, macho = original_census() if args.original else port_census(Path(tmp))
        if args.libc and not args.original:
            defined, undefined = defined_and_undefined(objs)
            missing |= {s for s in undefined - defined if re.fullmatch(r"_[a-z][a-z0-9_]*", s)}
        callers = defaultdict(set)
        units = unit_names()
        for obj in objs:
            for target, owners in references(obj).items():
                if target in missing:
                    callers[target] |= {(units.get(obj.stem, obj.stem), owner) for owner in owners}

    names = sorted(missing)
    plain = {n: strip_prefix(n, macho) for n in names}
    pretty = demangle([plain[n] for n in names])
    owners = sorted({f for fs in callers.values() for _, f in fs})
    pretty_owner = demangle([strip_prefix(f, macho) for f in owners]) if owners else {}

    grouped = defaultdict(list)
    for name in names:
        text = pretty[plain[name]]
        group = next((g for g, pattern in GROUPS if re.search(pattern, text)), "other")
        grouped[group].append((text, name))

    for group in [g for g, _ in GROUPS] + ["other"]:
        if not grouped[group]:
            continue
        print(f"## {GROUP_TITLES[group]} ({len(grouped[group])})\n")
        for text, name in sorted(grouped[group]):
            units = sorted({u for u, _ in callers[name]})
            funcs = sorted({pretty_owner[strip_prefix(f, macho)].split("(")[0] for _, f in callers[name]})
            if group in LIBRARY_GROUPS:
                print(f"- `{text}`: {', '.join(Path(u).name for u in units) or '-'}")
                continue
            print(f"- `{text}`")
            print(f"  - units: {', '.join(units) or '-'}")
            print(f"  - callers: {', '.join(f'`{f}`' for f in funcs) or '-'}")
        print()


if __name__ == "__main__":
    main()
