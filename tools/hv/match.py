"""Compare a compiled object with the target image, section by section.

Each allocated section of the object is placed at a target address: explicitly (units.toml), or
through the symbols it defines whose target addresses are known (symbols.tsv). Every known symbol
in a section must agree on the section's address. Relocations are resolved against placed
sections, known symbols, and the target's PLT and copy-relocated data, then written into the
object bytes; a placed section matches when those bytes equal the target's over its whole extent.

Merged string and constant sections are not laid out contiguously by the linker, so they are not
placed; each reference into them is checked by comparing the referenced content instead.
Nothing is masked: a relocation that cannot be resolved or checked makes its section inexact.
"""

import bisect
import functools
import hashlib
from dataclasses import dataclass, field

from hv.elf import Elf

R_X86_64_64 = 1
R_X86_64_PC32 = 2
R_X86_64_PLT32 = 4
R_X86_64_32 = 10
R_X86_64_32S = 11
WIDTHS = {R_X86_64_64: 8, R_X86_64_PC32: 4, R_X86_64_PLT32: 4, R_X86_64_32: 4, R_X86_64_32S: 4}
PC_RELATIVE = {R_X86_64_PC32, R_X86_64_PLT32}

SHF_ALLOC = 0x2
SHF_WRITE = 0x1
SHF_EXECINSTR = 0x4
SHF_MERGE = 0x10
SHF_STRINGS = 0x20
SHF_GROUP = 0x200

# sections that are not part of the image comparison yet
SKIPPED = {".eh_frame", ".ctors", ".dtors", ".init_array", ".fini_array", ".note.GNU-stack", ".comment"}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


@dataclass
class Reference:
    offset: int
    type: int
    symbol: str
    addend: int
    resolved: bool = False
    matches: bool = False
    reason: str = ""
    destination: int | None = None
    width: int = 0
    candidate: int | None = None  # destination the target encodes, for a symbol with no known address

    def report(self) -> dict:
        row = {"offset": self.offset, "type": self.type, "symbol": self.symbol, "addend": self.addend}
        if self.destination is not None:
            row["destination"] = hex(self.destination)
        if self.reason:
            row["reason"] = self.reason
        if self.candidate is not None:
            row["candidate"] = hex(self.candidate)
        row["matches"] = self.matches
        return row


@dataclass
class SectionResult:
    name: str
    index: int
    size: int
    address: int | None = None
    placement: str = ""
    nobits: bool = False
    exact: bool = False
    differences: list[int] = field(default_factory=list)
    misplaced: list[dict] = field(default_factory=list)
    references: list[Reference] = field(default_factory=list)
    functions: list[dict] = field(default_factory=list)
    data_ranges: list[dict] = field(default_factory=list)

    def report(self) -> dict:
        row = {"name": self.name, "size": self.size, "placement": self.placement, "exact": self.exact}
        if self.address is not None:
            row["address"] = hex(self.address)
        if self.differences:
            row["first_difference"] = self.differences[0]
            row["differing_bytes"] = len(self.differences)
        if self.misplaced:
            row["misplaced_symbols"] = self.misplaced
        bad = [r.report() for r in self.references if not r.matches]
        if bad:
            row["bad_references"] = bad
        row["references"] = len(self.references)
        if self.functions:
            row["functions"] = self.functions
        if self.data_ranges:
            row["data_ranges"] = self.data_ranges
        return row


class Resolver:
    """Target addresses of symbols: known symbols, then PLT entries, then copied library data."""

    def __init__(self, target: Elf, known: dict[str, int]):
        self.known = known
        self.plt = target.plt_symbols()
        self.copies = target.copy_symbols()

    def address(self, name: str) -> int | None:
        for table in (self.known, self.plt, self.copies):
            if name in table:
                return table[name]
        return None


def section_symbols(obj: Elf):
    return obj.symtab()


@dataclass
class Layout:
    """Target addresses of a placed section: a base, plus functions placed on their own."""

    base: int
    segments: list[tuple[int, int, int]] = field(default_factory=list)  # (start, end, address)

    def address_of(self, offset: int) -> int:
        for start, end, address in self.segments:
            if start <= offset < end:
                return address + offset - start
        return self.base + offset

    def in_function(self, offset: int) -> bool:
        return any(start <= offset < end for start, end, _ in self.segments)

    @property
    def contiguous(self) -> bool:
        return all(address == self.base + start for start, _, address in self.segments)


@functools.lru_cache(maxsize=4)
def fde_index(fdes: frozenset) -> tuple[dict[int, int], list[int], list[tuple[int, int]]]:
    """Each FDE's size by start address, the sorted start addresses, and the sorted FDEs."""
    sizes = dict(fdes)
    return sizes, sorted(sizes), sorted(fdes)


def layout_functions(section_index: int, base: int, symbols, resolver: Resolver, fdes) -> Layout:
    """Place each function of an executable section at its own target address.

    A function goes to its known address. One without a name in symbols.tsv (a static initializer,
    a compiler clone) goes to the target function right after the one before it when that has its
    size, or where an FDE of its size starts at the section base plus its offset, or else to the one
    unclaimed FDE of its size among the known functions' range. Failing all of those it still takes
    the target function after the one before it, so a length difference earlier in the section does
    not shift the rest. The section
    is contiguous when every function lands at base + offset: its order and lengths match the target's.
    `fdes` holds every proven function extent: the FDEs and the thunk extents from extents.tsv.
    """
    functions = sorted(
        (s for s in symbols if s["st_shndx"] == section_index and s["st_info"]["type"] == "STT_FUNC"),
        key=lambda s: s["st_value"],
    )
    layout = Layout(base)
    fde_sizes, starts, ranges = fde_index(frozenset(fdes))
    known = {s.name: resolver.known[s.name] for s in functions if s.name in resolver.known}
    if known:
        low = min(known.values())
        high = max(address + s["st_size"] for s in functions if (address := known.get(s.name)) is not None)
        taken = set(known.values())
        span = ranges[bisect.bisect_left(ranges, (low,)) : bisect.bisect_left(ranges, (high,))]
        free = [(a, n) for a, n in span if a not in taken]
    else:
        free = []
    for symbol in functions:
        start, size = symbol["st_value"], symbol["st_size"]
        address = known.get(symbol.name)
        if address is None:
            candidates = [a for a, n in free if n == size]
            following = base + start
            if layout.segments:
                # the target function that comes next after the one before it
                previous_start, previous_end, previous_address = layout.segments[-1]
                extent = fde_sizes.get(previous_address, previous_end - previous_start)
                after = bisect.bisect_left(starts, previous_address + extent)
                if after < len(starts):
                    following = starts[after]
                else:
                    following = previous_address + start - previous_start
            if (following, size) in fdes:
                address = following
            elif (base + start, size) in fdes:
                address = base + start
            elif len(candidates) == 1:
                address = candidates[0]
            else:
                address = following
            free = [(a, n) for a, n in free if a != address]
        layout.segments.append((start, start + size, address))
    return layout


def place_sections(
    obj: Elf, resolver: Resolver, explicit: dict[str, int], fdes=frozenset()
) -> dict[int, tuple[Layout, str, list]]:
    """Section index -> (target address, how it was placed, symbols whose known address disagrees).

    Explicit placements win; otherwise the earliest known symbol anchors the section, since a
    length difference only shifts what follows it. A later symbol implying another address marks
    where the lengths diverge: the code just before it differs.
    """
    by_section: dict[int, list] = {}
    for symbol in section_symbols(obj):
        if isinstance(symbol["st_shndx"], int) and symbol.name and symbol["st_info"]["type"] != "STT_SECTION":
            by_section.setdefault(symbol["st_shndx"], []).append(symbol)
    placements = {}
    names = {}
    for index, section in enumerate(obj.sections):
        names[section.name] = index
        if not section["sh_flags"] & SHF_ALLOC or section.name in SKIPPED or is_merged(section):
            continue
        if not section["sh_size"]:
            continue
        implied = []  # (base, offset in section, name)
        for symbol in by_section.get(index, []):
            known = resolver.known.get(symbol.name)
            if known is not None:
                implied.append((known - symbol["st_value"], symbol["st_value"], symbol.name))
        if section.name in explicit:
            address, how = explicit[section.name], "explicit"
        elif implied:
            address, _, anchor = min(implied, key=lambda i: i[1])
            how = f"symbol: {anchor}"
        else:
            continue
        misplaced = [
            {"symbol": name, "known": hex(base + offset), "placed": hex(address + offset)}
            for base, offset, name in sorted(implied, key=lambda i: i[1])
            if base != address
        ]
        if section["sh_flags"] & SHF_EXECINSTR:
            layout = layout_functions(index, address, section_symbols(obj), resolver, fdes)
        else:
            layout = Layout(address)
        placements[index] = (layout, how, misplaced)
    if unknown := set(explicit) - set(names):
        raise ValueError(f"explicit placement for sections not in the object: {', '.join(sorted(unknown))}")
    return placements


def infer_placements(obj: Elf, target: Elf, placements: dict, sections: list) -> None:
    """Place unplaced data sections that placed code references, from the target's encoded values.

    Each reference implies the referenced section's base: its target value minus the addend and the
    symbol's offset. A section is placed only when every reference implies the same base; its bytes
    are then compared like any other placed section.
    """
    implied: dict[int, set[int]] = {}
    for index, (layout, _, _) in list(placements.items()):
        section = sections[index]
        if section["sh_type"] == "SHT_NOBITS":
            continue
        for relocation, symbol in obj.relocations(index):
            shndx = symbol["st_shndx"]
            rtype = relocation["r_info_type"]
            width = WIDTHS.get(rtype)
            if not isinstance(shndx, int) or shndx in placements or width is None:
                continue
            referenced = sections[shndx]
            flags = referenced["sh_flags"]
            if not flags & SHF_ALLOC or flags & SHF_EXECINSTR or is_merged(referenced):
                continue
            if referenced.name in SKIPPED or referenced["sh_type"] == "SHT_NOBITS":
                continue
            place = layout.address_of(relocation["r_offset"])
            try:
                field = target.read(place, width)
            except ValueError:
                continue
            signed = rtype in PC_RELATIVE or rtype == R_X86_64_32S
            value = int.from_bytes(field, "little", signed=signed)
            absolute = value + place if rtype in PC_RELATIVE else value
            implied.setdefault(shndx, set()).add(absolute - relocation["r_addend"] - symbol["st_value"])
    for shndx, bases in implied.items():
        if len(bases) == 1:
            placements[shndx] = (Layout(bases.pop()), "references", [])


def infer_exception_tables(obj: Elf, target: Elf, placements: dict, sections: list) -> None:
    """Place exception tables (LSDAs), which only our FDEs reference, from the target FDE of each
    placed function: its LSDA pointer minus the offset our FDE points to. A table is placed only when
    every function using it implies the same base."""
    frames = [i for i, s in enumerate(sections) if s.name == ".eh_frame"]
    if not frames:
        return
    data = sections[frames[0]].data()
    relocations = [(r["r_offset"], r, s) for r, s in obj.relocations(frames[0])]
    lsdas = target.fde_lsdas()
    implied: dict[int, set[int]] = {}
    offset = 0
    while offset + 8 <= len(data):
        length = int.from_bytes(data[offset : offset + 4], "little")
        if not length:
            break
        end = offset + 4 + length
        if int.from_bytes(data[offset + 4 : offset + 8], "little"):  # an FDE; a CIE has id 0
            code = lsda = None
            for at, relocation, symbol in relocations:
                shndx = symbol["st_shndx"]
                if not offset <= at < end or not isinstance(shndx, int):
                    continue
                where = (shndx, symbol["st_value"] + relocation["r_addend"])
                if sections[shndx]["sh_flags"] & SHF_EXECINSTR:
                    code = where
                elif sections[shndx]["sh_flags"] & SHF_ALLOC:
                    lsda = where
            if code and lsda and code[0] in placements and lsda[0] not in placements:
                pointer = lsdas.get(placements[code[0]][0].address_of(code[1]))
                implied.setdefault(lsda[0], set()).add(None if pointer is None else pointer - lsda[1])
        offset = end
    for shndx, bases in implied.items():
        if len(bases) == 1 and None not in bases:
            placements[shndx] = (Layout(bases.pop()), "exception frames", [])


def is_merged(section) -> bool:
    return bool(section["sh_flags"] & SHF_MERGE)


def merged_content(section, offset: int) -> bytes:
    """The complete merge element (or terminated string) checked by a reference."""
    data = section.data()
    size = section["sh_entsize"] or 1
    if not 0 <= offset < len(data):
        raise ValueError("reference outside merged section")
    if offset % size:
        raise ValueError("reference not aligned to a merged element")
    if section["sh_flags"] & SHF_STRINGS:
        end = offset
        while end + size <= len(data) and data[end : end + size] != bytes(size):
            end += size
        if end + size > len(data):
            raise ValueError("unterminated merged string")
        return data[offset : end + size]
    if offset + size > len(data):
        raise ValueError("incomplete merged constant")
    return data[offset : offset + size]


def check_merged(target: Elf, section, offset: int, address: int) -> tuple[bool, str]:
    """Compare the object content at a merged-section offset with the target's at an address."""
    try:
        ours = merged_content(section, offset)
        kind = "merged string" if section["sh_flags"] & SHF_STRINGS else "merged constant"
        return target.read(address, len(ours)) == ours, kind
    except ValueError as error:
        return False, str(error)


def check_local_copy(
    obj: Elf, target: Elf, shndx: int, offset: int, address: int, matched_ranges: list | None = None
) -> tuple[bool, str]:
    """Compare the file-level static object holding a section offset with the target's copy, which
    holds `address` at the same place. Only read-only, relocation-free objects are comparable:
    equal initial bytes do not make two mutable objects interchangeable."""
    section = obj.sections[shndx]
    if section["sh_type"] != "SHT_PROGBITS" or not section["sh_flags"] & SHF_ALLOC:
        return False, "different destination"
    if section["sh_flags"] & SHF_WRITE:
        return False, "mutable local object"
    holders = [
        s
        for s in obj.symtab()
        if s["st_shndx"] == shndx
        and s["st_info"]["bind"] == "STB_LOCAL"
        and s["st_info"]["type"] == "STT_OBJECT"
        and s["st_value"] <= offset < s["st_value"] + s["st_size"]
    ]
    if len(holders) != 1:
        return False, "different destination"
    start, size = holders[0]["st_value"], holders[0]["st_size"]
    owner = target.section_at(address - (offset - start))
    if owner is None or owner["sh_flags"] & SHF_WRITE:
        return False, "mutable or unmapped target copy"
    if any(start <= r["r_offset"] < start + size for r, _ in obj.relocations(shndx)):
        return False, "different destination"
    ours = section.data()[start : start + size]
    if len(ours) != size:
        return False, "truncated local object"
    try:
        destination = address - (offset - start)
        exact = target.read(destination, size) == ours
        if exact and matched_ranges is not None:
            matched_ranges.append({"address": hex(destination), "size": size, "kind": "local copy"})
        return exact, "local copy"
    except ValueError as error:
        return False, str(error)


def compare_object(obj: Elf, target: Elf, known: dict[str, int], explicit: dict[str, int]) -> dict:
    resolver = Resolver(target, known)
    extents = target.function_extents()
    fdes = frozenset((address, size) for address, (size, _) in extents.items())
    placements = place_sections(obj, resolver, explicit, fdes)
    symbols = section_symbols(obj)
    sections = obj.sections
    infer_placements(obj, target, placements, sections)
    infer_exception_tables(obj, target, placements, sections)
    results = []
    unplaced = []
    for index, section in enumerate(sections):
        if not section["sh_flags"] & SHF_ALLOC or section.name in SKIPPED:
            continue
        if is_merged(section) or not section["sh_size"]:
            continue
        if index not in placements:
            unplaced.append(section.name)
            continue
        layout, how, misplaced = placements[index]
        address = layout.base
        result = SectionResult(section.name, index, section["sh_size"], address, how, misplaced=misplaced)
        if section["sh_type"] == "SHT_NOBITS":
            result.nobits = True
            owner = target.section_at(address)
            result.exact = (
                owner is not None
                and owner["sh_type"] == "SHT_NOBITS"
                and address + result.size <= owner["sh_addr"] + owner["sh_size"]
                and not misplaced
                and layout.contiguous
            )
            if result.exact:
                result.data_ranges.append({"address": hex(address), "size": result.size, "kind": "bss"})
            results.append(result)
            continue
        compare_section(obj, target, section, index, layout, placements, resolver, sections, result)
        if section["sh_flags"] & SHF_EXECINSTR:
            # FDE extents pin each function's full length, including the section's last one
            result.functions = function_results(symbols, index, layout, result, extents)
            result.exact = result.exact and all(f["exact"] for f in result.functions)
        result.exact = result.exact and not misplaced and layout.contiguous
        if result.exact and not section["sh_flags"] & SHF_EXECINSTR:
            result.data_ranges.append({"address": hex(address), "size": result.size, "kind": "section"})
        results.append(result)
    exact = bool(results) and not unplaced and all(r.exact for r in results)
    return {
        "object_sha256": digest(obj.data),
        "exact": exact,
        "sections": [r.report() for r in results],
        "unplaced_sections": unplaced,
    }


def expected_bytes(target: Elf, layout: Layout, size: int) -> tuple[bytes, set[int]]:
    """Target bytes for each object offset, and the offsets that cannot be compared: padding
    between functions placed out of order."""
    if layout.contiguous:
        return target.read(layout.base, size), set()
    expected = bytearray(size)
    ignored = set(range(size))
    for start, end, address in layout.segments:
        expected[start:end] = target.read(address, end - start)
        ignored.difference_update(range(start, end))
    return bytes(expected), ignored


def compare_section(obj, target, section, index, layout, placements, resolver, sections, result):
    raw = section.data()
    try:
        expected, ignored = expected_bytes(target, layout, len(raw))
    except ValueError as error:
        result.differences = list(range(len(raw)))
        result.references.append(Reference(0, 0, "", 0, reason=str(error)))
        return
    relocated = bytearray(raw)
    covered: set[int] = set()
    for relocation, symbol in obj.relocations(index):
        rtype = relocation["r_info_type"]
        offset = relocation["r_offset"]
        name = symbol.name or (
            sections[symbol["st_shndx"]].name if isinstance(symbol["st_shndx"], int) else ""
        )
        ref = Reference(offset, rtype, name, relocation["r_addend"])
        result.references.append(ref)
        width = WIDTHS.get(rtype)
        if width is None:
            ref.reason = "unsupported relocation type"
            continue
        if offset < 0 or offset + width > len(raw):
            raise ValueError(f"relocation outside {section.name} at {offset:#x}")
        if covered & set(range(offset, offset + width)):
            raise ValueError(f"overlapping relocations in {section.name} at {offset:#x}")
        covered.update(range(offset, offset + width))
        ref.width = width
        place = layout.address_of(offset)
        field_bytes = expected[offset : offset + width]
        field_value = int.from_bytes(
            field_bytes, "little", signed=rtype in PC_RELATIVE or rtype == R_X86_64_32S
        )

        target_section = sections[symbol["st_shndx"]] if isinstance(symbol["st_shndx"], int) else None
        if target_section is not None and is_merged(target_section) and symbol["st_shndx"] not in placements:
            # S + A as the target encodes it; a rip-relative operand without a trailing immediate
            # points 4 bytes past its addend
            bias = 4 if rtype in PC_RELATIVE else 0
            absolute = field_value + place if rtype in PC_RELATIVE else field_value
            ours = symbol["st_value"] + ref.addend + bias
            ref.resolved = True
            ref.matches, ref.reason = check_merged(target, target_section, ours, absolute + bias)
            if ref.matches:
                result.data_ranges.append(
                    {
                        "address": hex(absolute + bias),
                        "size": len(merged_content(target_section, ours)),
                        "kind": ref.reason,
                    }
                )
            relocated[offset : offset + width] = field_bytes if ref.matches else bytes(width)
            continue

        bias = 4 if rtype in PC_RELATIVE else 0
        destination = resolve(symbol, placements, resolver, ref.addend + bias)
        if destination is None:
            ref.reason = "no target address for symbol"
            if symbol.name and symbol["st_info"]["type"] != "STT_SECTION":
                ref.candidate = (
                    field_value + place - ref.addend if rtype in PC_RELATIVE else field_value - ref.addend
                )
            continue
        ref.destination = destination
        value = destination + ref.addend - (place if rtype in PC_RELATIVE else 0)
        signed = rtype in PC_RELATIVE or rtype == R_X86_64_32S
        bits = width * 8
        low, high = (-(1 << (bits - 1)), 1 << (bits - 1)) if signed else (0, 1 << bits)
        if not low <= value < high:
            ref.reason = "relocation overflow"
            continue
        encoded = value.to_bytes(width, "little", signed=signed)
        relocated[offset : offset + width] = encoded
        ref.resolved = True
        ref.matches = encoded == field_bytes
        if not ref.matches:
            ref.reason = "different destination"
            if section["sh_flags"] & SHF_GROUP and symbol["st_info"]["bind"] == "STB_LOCAL":
                # an inline copy kept from another object reads that object's copy of a static
                pointed = field_value + place + bias if rtype in PC_RELATIVE else field_value
                ours = symbol["st_value"] + ref.addend + bias
                ref.matches, ref.reason = check_local_copy(
                    obj, target, symbol["st_shndx"], ours, pointed, result.data_ranges
                )
                if ref.matches:
                    relocated[offset : offset + width] = field_bytes
    result.differences = [i for i in range(len(raw)) if relocated[i] != expected[i] and i not in ignored]
    result.exact = not result.differences and all(r.matches for r in result.references)


def resolve(symbol, placements, resolver: Resolver, reach: int) -> int | None:
    """Target address of a symbol (S). For a section symbol, `reach` is the object offset the
    reference points into, so a section laid out per function maps it through that function."""
    shndx = symbol["st_shndx"]
    if isinstance(shndx, int):
        if shndx in placements:
            layout = placements[shndx][0]
            if symbol["st_info"]["type"] == "STT_SECTION":
                return layout.address_of(reach) - reach
            return layout.address_of(symbol["st_value"])
        # defined here in a section placed elsewhere, such as an inline copy the linker discarded
        return resolver.known.get(symbol.name) if symbol.name else None
    if shndx == "SHN_UNDEF":
        return resolver.address(symbol.name)
    return None


def function_results(symbols, index, layout: Layout, result: SectionResult, extents) -> list[dict]:
    """Per-function verdicts. `extent` says what proves the function's extent in the target: an
    FDE of exactly its size, a thunk extent (extents.tsv), or nothing, in which case it cannot be
    exact. `exact_but_unknown` means every byte matches except the fields of references to symbols
    without a known address, whose target destinations are then trustworthy."""
    unknown = [r for r in result.references if r.candidate is not None]
    unknown_bytes = {r.offset + i for r in unknown for i in range(r.width)}
    rows = []
    for symbol in sorted(
        (s for s in symbols if s["st_shndx"] == index and s["st_info"]["type"] == "STT_FUNC"),
        key=lambda s: s["st_value"],
    ):
        start, size = symbol["st_value"], symbol["st_size"]
        address = layout.address_of(start)
        span = range(start, start + size)
        known_size, source = extents.get(address, (None, None))
        extent = source if known_size == size else None
        differs = [o for o in result.differences if o in span]
        bad = [r for r in result.references if not r.matches and r.offset in span]
        row = {"symbol": symbol.name, "address": hex(address), "size": size, "extent": extent}
        row["global"] = symbol["st_info"]["bind"] != "STB_LOCAL"
        row["exact"] = extent is not None and not differs and not bad
        if not row["exact"] and extent is not None and bad and all(r.candidate is not None for r in bad):
            if not [o for o in differs if o not in unknown_bytes]:
                row["exact_but_unknown"] = True
                # every reference, so learning can see when two of them disagree
                row["candidates"] = [[r.symbol, hex(r.candidate)] for r in bad]
        rows.append(row)
    return rows


def learnable(
    result: dict, source: str, known: dict[str, int]
) -> tuple[dict[str, tuple[int, str]], list[str]]:
    """Symbol addresses a match proves: unknown symbols referenced by functions that match except
    for those references (evidence `reloc:<unit>:<function>`), and global functions of this unit
    that match exactly (evidence `match:<unit>`), so other units can reference them.

    Returns {symbol: (address, evidence)} and the symbols whose implied addresses disagree.
    """
    found: dict[str, set[int]] = {}
    evidence: dict[str, str] = {}
    for section in result["sections"]:
        for function in section.get("functions", []):
            for name, address in function.get("candidates", []):
                found.setdefault(name, set()).add(int(address, 16))
                evidence.setdefault(name, f"reloc:{source}:{function['symbol']}")
            if function["exact"] and function["global"] and function["symbol"] not in known:
                found.setdefault(function["symbol"], set()).add(int(function["address"], 16))
                evidence.setdefault(function["symbol"], f"match:{source}")
    learned = {name: (next(iter(a)), evidence[name]) for name, a in found.items() if len(a) == 1}
    return learned, sorted(name for name, a in found.items() if len(a) > 1)
