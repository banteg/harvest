"""Cut target objects out of the linked executable, for objdiff.

For each recovered unit, the matcher knows where every function and placed data section of our
object lives in the target. This writes a relocatable ELF with the *target's* bytes there, laid out
like our object: the same sections, each function at our offset (so objdiff compares references by
name and address alike), and relocations recovered from the target code itself: capstone decodes each
instruction, and every call or branch leaving the function, rip-relative operand and absolute
address immediate becomes a relocation against the symbol the target actually references. A call or
jump to a local function in the same section is resolved in place, as the assembler did for ours.
Nothing is copied from our object's relocations for code, so a wrong call target stays visible.

Symbols are named from this unit's placed symbols, then symbols.tsv, the PLT and copy relocations.
An address inside one of our placed data sections (such as `.bss`) becomes a reference to that
section, as in our object, and an address inside one of our functions (a jump table case, the label
a variadic prologue jumps to) a reference to that function's section at its offset in the target
object. A referenced string that our object also has in a merged string section, narrow or wide,
goes into a target copy of that section at the same offset, holding the target's bytes, so both
sides reference `.rodata.str1.1+offset` or `.rodata.str4.4+offset`; the target has a copy of every
merged section of ours, zero-filled where nothing references it. Binary merge elements are matched
by access width and content, and keep their merge type. Other literals go into `.rodata.lit`;
unnamed code becomes sub_<addr> and unnamed data lbl_<addr>.
"""

import bisect
import struct
from dataclasses import dataclass, field
from pathlib import Path

import capstone
from capstone import x86

from hv.elf import Elf

R_X86_64_64 = 1
R_X86_64_PC32 = 2
R_X86_64_32 = 10
R_X86_64_32S = 11

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS = 1, 2, 3, 4, 8
SHF_ALLOC, SHF_EXECINSTR, SHF_MERGE, SHF_STRINGS = 0x2, 0x4, 0x10, 0x20

WCHAR_SIZE = 4  # the character size of wide strings (and of their merged sections, such as .rodata.str4.4)


@dataclass
class Section:
    name: str
    data: bytes
    flags: int
    align: int = 16
    relocations: list[tuple[int, int, str, int]] = field(default_factory=list)  # offset, type, symbol, addend
    nobits: bool = False
    entsize: int = 0


@dataclass
class Symbol:
    name: str
    section: str | None  # None: undefined
    offset: int = 0
    size: int = 0
    function: bool = False
    local: bool = False


class Namer:
    """Names for target addresses."""

    def __init__(
        self,
        target: Elf,
        known: list[tuple[int, int, str]],
        local: dict[int, str],
        placed: list[tuple[int, int, str]],
        strings: dict[bytes, tuple[str, int]],
        constants: dict[bytes, tuple[str, int]] | None = None,
        copies: dict[str, bytes] | None = None,
        bodies: list[tuple[int, int, str, int]] | None = None,
    ):
        """known: (address, size, name), size 0 running to the next known symbol; placed: our data
        sections as (start, end, name); strings: our merged strings by content -> (section, offset);
        bodies: our functions as (start, end, section, offset of the start in the target object)."""
        self.target = target
        self.by_address: dict[int, str] = {}
        for address, _, name in known:
            self.by_address.setdefault(address, name)
        self.by_address.update(local)
        for name, address in target.plt_symbols().items():
            self.by_address.setdefault(address, name)
        text = target.elf.get_section_by_name(".text")
        self.text = range(text["sh_addr"], text["sh_addr"] + text["sh_size"])
        self.bodies = sorted(bodies or [])
        self.body_starts = [start for start, _, _, _ in self.bodies]
        self.placed = placed
        self.strings = strings
        self.constants = constants or {}
        self.copies = copies or {}
        self.merged: dict[str, dict[int, bytes]] = {}  # our merged section -> offset -> target bytes
        self.literals: dict[int, bytes] = {}
        # data symbols, for addresses inside them (a vtable's address point, a copied vtable)
        data = [(a, n, name) for a, n, name in known if a not in self.text]
        data += [(address, 0, name) for name, address in target.copy_symbols().items()]
        data.sort()
        self.data = []
        for i, (address, size, name) in enumerate(data):
            end = address + size if size else (data[i + 1][0] if i + 1 < len(data) else address + 1)
            self.data.append((address, end, name))
        self.starts = [a for a, _, _ in self.data]

    def name(self, address: int, size: int | None = None) -> tuple[str, int]:
        """(symbol or section, addend) for a target address."""
        if address in self.by_address:
            return self.by_address[address], 0
        # A label inside one of our functions (a case of a jump table, the target of a computed
        # jump) is where the assembler put it: in that function's section, at its offset there.
        i = bisect.bisect_right(self.body_starts, address) - 1
        if i >= 0 and address < self.bodies[i][1]:
            start, _, section, offset = self.bodies[i]
            return section, offset + address - start
        if address in self.text:
            return f"sub_{address:x}", 0
        for start, end, section in self.placed:
            if start <= address < end:
                return section, address - start
        i = bisect.bisect_right(self.starts, address) - 1
        if i >= 0 and address < self.data[i][1]:
            return self.data[i][2], address - self.data[i][0]
        owner = self.target.section_at(address)
        if size is not None and owner is not None and not owner["sh_flags"] & 1:
            # A kept inline function can reference another object's copy of a local constant
            # table. Name it only when the entire target table equals the target definition
            # exported in this unit, not just the single indexed element's access width.
            copies = []
            for name, data in self.copies.items():
                try:
                    if len(data) >= size and self.target.read(address, len(data)) == data:
                        copies.append(name)
                except ValueError:
                    continue
            if len(copies) == 1:
                return copies[0], 0
        literal = self.literal(address, size)
        wide = self.literal(address, width=WCHAR_SIZE)
        if size is None:
            # A pointer may address a wide string. Read narrow, it stops after the first character
            # (an empty wide string reads as the empty narrow one), so the wide reading goes first.
            readings = [(wide, self.strings), (literal, self.strings)]
        else:
            # A memory operand supplies its access width. Read its binary value rather than
            # interpreting, for example, the first zero byte of a float as an empty C string. A load
            # of the first character of a wide string reaches the string by the same address.
            readings = [(literal, self.constants), (wide, self.strings)]
        for content, candidates in readings:
            if content in candidates:
                section, offset = candidates[content]
                self.merged.setdefault(section, {})[offset] = content
                return section, offset
        # Different instructions may read different widths at the same address.
        if literal is not None and len(literal) > len(self.literals.get(address, b"")):
            self.literals[address] = literal
        return f"lbl_{address:x}", 0

    def literal(self, address: int, size: int | None = None, width: int = 1) -> bytes | None:
        """The bytes of `size` at an address, or the string of `width`-byte characters there."""
        section = self.target.section_at(address)
        if section is None or section.name != ".rodata" or address % width:
            return None
        try:
            if size is not None:
                return self.target.read(address, size)
            return self.target.cstring(address, width) + bytes(width)
        except ValueError:
            return None


def merged_strings(obj: Elf) -> dict[bytes, tuple[str, int]]:
    """Every zero-terminated string of our merged string sections, by content. Each string starts at
    the section's alignment: the zeros between aligned strings are padding, not empty strings."""
    strings: dict[bytes, tuple[str, int]] = {}
    for section in obj.elf.iter_sections():
        flags = SHF_ALLOC | SHF_MERGE | SHF_STRINGS
        if section["sh_flags"] & flags != flags:
            continue
        data = section.data()
        size = section["sh_entsize"] or 1
        align = section["sh_addralign"] or 1
        start = 0
        while start < len(data):
            end = start
            while end + size <= len(data) and data[end : end + size] != bytes(size):
                end += size
            if end + size > len(data):
                raise ValueError(f"unterminated merged string in {section.name}")
            strings.setdefault(data[start : end + size], (section.name, start))
            start = (end + size + align - 1) // align * align
    return strings


def merged_constants(obj: Elf) -> dict[bytes, tuple[str, int]]:
    """Allocated binary merge elements, indexed separately from strings."""
    constants = {}
    for section in obj.elf.iter_sections():
        if section["sh_flags"] & (SHF_ALLOC | SHF_MERGE | SHF_STRINGS) != SHF_ALLOC | SHF_MERGE:
            continue
        size = section["sh_entsize"]
        data = section.data()
        if not size or len(data) % size:
            raise ValueError(f"invalid merged constants in {section.name}")
        for offset in range(0, len(data), size):
            constants.setdefault(data[offset : offset + size], (section.name, offset))
    return constants


def mapped(target: Elf, value: int) -> bool:
    section = target.section_at(value)
    return section is not None and value >= 0x400000


def code_relocations(target: Elf, address: int, code: bytes, namer: Namer):
    """Relocations for one target function, recovered from its instructions."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = True
    end = address + len(code)
    relocations = []
    for insn in md.disasm(code, address):
        offset = insn.address - address
        tail_end = insn.address + insn.size
        for op in insn.operands:
            branch = insn.group(capstone.CS_GRP_CALL) or insn.group(capstone.CS_GRP_JUMP)
            if op.type == x86.X86_OP_IMM and branch:
                destination = op.imm
                if address <= destination < end or insn.imm_size != 4:
                    continue
                field = offset + insn.imm_offset
                name, addend = namer.name(destination)
                relocations.append((field, R_X86_64_PC32, name, addend - (tail_end - (address + field))))
            elif op.type == x86.X86_OP_MEM and op.mem.base == x86.X86_REG_RIP:
                destination = tail_end + op.mem.disp
                field = offset + insn.disp_offset
                size = None if insn.id == x86.X86_INS_LEA else op.size
                name, addend = namer.name(destination, size)
                relocations.append((field, R_X86_64_PC32, name, addend - (tail_end - (address + field))))
            elif op.type == x86.X86_OP_MEM and op.mem.base == 0 and insn.disp_size == 4:
                if mapped(target, op.mem.disp):
                    size = None if insn.id == x86.X86_INS_LEA else op.size
                    name, addend = namer.name(op.mem.disp, size)
                    relocations.append((offset + insn.disp_offset, R_X86_64_32S, name, addend))
            elif op.type == x86.X86_OP_IMM and insn.imm_size == 4 and mapped(target, op.imm & 0xFFFFFFFF):
                value = op.imm & 0xFFFFFFFF
                name, addend = namer.name(value)
                # a sign-extended 64-bit destination uses 32S, a 32-bit register destination 32
                kind = R_X86_64_32S if op.size == 8 else R_X86_64_32
                relocations.append((offset + insn.imm_offset, kind, name, addend))
    return relocations


def is_local(sym) -> bool:
    return sym["st_info"]["bind"] == "STB_LOCAL"


def delink_unit(target: Elf, obj: Elf, result: dict, known: list[tuple[int, int, str]], fdes: dict[int, int]):
    """Sections and symbols of the target object for one unit, from its match result."""
    obj_sections = list(obj.elf.iter_sections())
    placed_at = {s["name"]: int(s["address"], 16) for s in result["sections"] if "address" in s}
    data_sections = [
        (index, osec)
        for index, osec in enumerate(obj_sections)
        if osec.name in placed_at and not osec["sh_flags"] & SHF_EXECINSTR
    ]
    placed = [(placed_at[o.name], placed_at[o.name] + o["sh_size"], o.name) for _, o in data_sections]
    symtab = obj.elf.get_section_by_name(".symtab")
    ours = {sym.name: sym for sym in symtab.iter_symbols() if sym["st_info"]["type"] == "STT_FUNC"}
    local = {}
    functions = []  # (symbol, target address, target size, our section index, our offset)
    for section in result["sections"]:
        for function in section.get("functions", []):
            address = int(function["address"], 16)
            sym = ours[function["symbol"]]
            local[address] = function["symbol"]
            size = fdes.get(address, function["size"])
            functions.append((function["symbol"], address, size, sym["st_shndx"], sym["st_value"]))
    copies = {}
    for index, section in data_sections:
        if section["sh_flags"] & 1 or section["sh_type"] != "SHT_PROGBITS":
            continue
        for symbol in symtab.iter_symbols():
            if (
                symbol["st_shndx"] != index
                or not is_local(symbol)
                or symbol["st_info"]["type"] != "STT_OBJECT"
                or not symbol["st_size"]
            ):
                continue
            start, size = symbol["st_value"], symbol["st_size"]
            owner = target.section_at(placed_at[section.name] + start)
            if owner is None or owner["sh_flags"] & 1:
                continue
            if any(start <= r["r_offset"] < start + size for r, _ in obj.relocations(index)):
                continue
            copies[symbol.name] = target.read(placed_at[section.name] + start, size)
    # our offsets, unless a longer target function before would overlap: then shift down
    layout = {}  # our section index -> [(symbol, target address, target size, our section index, offset)]
    for index in sorted({f[3] for f in functions}):
        layout[index], end = [], 0
        for name, address, size, _, offset in sorted(
            (f for f in functions if f[3] == index), key=lambda f: f[4]
        ):
            offset = max(offset, end)
            layout[index].append((name, address, size, index, offset))
            end = offset + size
    offset_of = {
        name: (index, offset) for members in layout.values() for name, _, _, index, offset in members
    }
    bodies = [
        (address, address + size, obj_sections[index].name, offset)
        for members in layout.values()
        for _, address, size, index, offset in members
    ]
    namer = Namer(target, known, local, placed, merged_strings(obj), merged_constants(obj), copies, bodies)

    sections, symbols = [], []
    for index, members in layout.items():
        osec = obj_sections[index]
        data = bytearray(max([osec["sh_size"]] + [offset + size for _, _, size, _, offset in members]))
        section = Section(osec.name, b"", SHF_ALLOC | SHF_EXECINSTR, align=osec["sh_addralign"])
        for _, address, size, _, offset in members:
            data[offset : offset + size] = target.read(address, size)
        for name, address, size, _, offset in members:
            code = bytes(data[offset : offset + size])
            for at, kind, symbol, addend in code_relocations(target, address, code, namer):
                where, destination = offset + at, offset_of.get(symbol)
                if (
                    kind == R_X86_64_PC32
                    and destination
                    and destination[0] == index
                    and is_local(ours[symbol])
                ):
                    # resolved by the assembler: the displacement to a local function in this section
                    value = destination[1] + addend - where
                    data[where : where + 4] = value.to_bytes(4, "little", signed=True)
                else:
                    section.relocations.append((where, kind, symbol, addend))
            # the target's own extent (its FDE), so objdiff sees every target byte
            symbols.append(Symbol(name, osec.name, offset, size, function=True, local=is_local(ours[name])))
        section.data = bytes(data)
        sections.append(section)

    # placed data sections keep our layout; their words are named from the target values
    for index, osec in data_sections:
        size = osec["sh_size"]
        if osec["sh_type"] == "SHT_NOBITS":
            sections.append(Section(osec.name, bytes(size), SHF_ALLOC | 0x1, align=8, nobits=True))
        else:
            data = target.read(placed_at[osec.name], size)
            section = Section(osec.name, data, SHF_ALLOC, align=8)
            for relocation, _ in obj.relocations(index):
                if relocation["r_info_type"] != R_X86_64_64:
                    continue
                offset = relocation["r_offset"]
                value = struct.unpack_from("<Q", data, offset)[0]
                if value:
                    name, addend = namer.name(value)
                    section.relocations.append((offset, R_X86_64_64, name, addend))
            sections.append(section)
        for sym in symtab.iter_symbols():
            if sym["st_shndx"] == index and sym.name and sym["st_info"]["type"] != "STT_SECTION":
                symbols.append(Symbol(sym.name, osec.name, sym["st_value"], sym["st_size"]))

    # Target merge elements at the offsets our merged sections hold them, with their original type.
    # Every merged section of ours has a copy, zero-filled where nothing references it: objdiff
    # combines the sections of a class (such as .rodata.str1.1 and .rodata.str1.8) only when an
    # object has at least two, and the two objects must combine alike.
    for osec in obj_sections:
        if osec["sh_flags"] & (SHF_ALLOC | SHF_MERGE) == SHF_ALLOC | SHF_MERGE:
            data = bytearray(osec["sh_size"])
            for offset, text in namer.merged.get(osec.name, {}).items():
                data[offset : offset + len(text)] = text
            sections.append(
                Section(
                    osec.name,
                    bytes(data),
                    osec["sh_flags"],
                    align=osec["sh_addralign"],
                    entsize=osec["sh_entsize"],
                )
            )
    if namer.literals:
        blob = bytearray()
        for address, text in sorted(namer.literals.items()):
            symbols.append(Symbol(f"lbl_{address:x}", ".rodata.lit", len(blob), len(text)))
            blob += text
        sections.append(Section(".rodata.lit", bytes(blob), SHF_ALLOC, align=1))
    return sections, symbols


WIDTHS = {R_X86_64_64: 8, R_X86_64_PC32: 4, R_X86_64_32: 4, R_X86_64_32S: 4}


def clear_fields(section: Section) -> bytes:
    """RELA objects keep zeros in relocated fields; the value lives in the relocation."""
    data = bytearray(section.data)
    for offset, kind, _, _ in section.relocations:
        data[offset : offset + WIDTHS[kind]] = bytes(WIDTHS[kind])
    return bytes(data)


def write_object(path: Path, sections: list[Section], symbols: list[Symbol]) -> None:
    """A minimal ELF64 x86-64 relocatable object."""
    defined = {s.name for s in symbols}
    referenced = {r[2] for s in sections for r in s.relocations} - defined
    section_names = {s.name for s in sections}
    section_symbols = [Symbol(name, name, local=True) for name in sorted(referenced & section_names)]
    undefined = sorted(referenced - section_names)
    locals_ = section_symbols + [s for s in symbols if s.local]
    ordered = locals_ + [s for s in symbols if not s.local] + [Symbol(name, None) for name in undefined]
    first_global = 1 + len(locals_)
    is_section_symbol = {id(s) for s in section_symbols}

    shstrtab, strtab = bytearray(b"\0"), bytearray(b"\0")

    def add(table: bytearray, text: str) -> int:
        position = len(table)
        table.extend(text.encode() + b"\0")
        return position

    section_index = {s.name: i + 1 for i, s in enumerate(sections)}
    symbol_index = {s.name: i + 1 for i, s in enumerate(ordered)}
    symtab = bytearray(24)
    for s in ordered:
        shndx = section_index[s.section] if s.section else 0
        if id(s) in is_section_symbol:
            symtab += struct.pack("<IBBHQQ", 0, 3, 0, shndx, 0, 0)  # STB_LOCAL, STT_SECTION
            continue
        info = (0 if s.local else 1) << 4 | (2 if s.function else (1 if s.section else 0))
        symtab += struct.pack("<IBBHQQ", add(strtab, s.name), info, 0, shndx, s.offset, s.size)

    headers, blobs, sizes = [], [], []  # (name, type, flags, link, info, align, entsize), payload, size
    for s in sections:
        kind = SHT_NOBITS if s.nobits else SHT_PROGBITS
        entsize = s.entsize or (1 if s.flags & SHF_STRINGS else 0)
        headers.append((s.name, kind, s.flags, 0, 0, s.align, entsize))
        blobs.append(b"" if s.nobits else clear_fields(s))
        sizes.append(len(s.data))
    symtab_index = len(sections) + 1
    for s in sections:
        if s.relocations:
            rela = b"".join(
                struct.pack("<QQq", offset, symbol_index[name] << 32 | kind, addend)
                for offset, kind, name, addend in sorted(s.relocations)
            )
            headers.append((".rela" + s.name, SHT_RELA, 0x40, symtab_index, section_index[s.name], 8, 24))
            blobs.append(rela)
            sizes.append(len(rela))
    # .symtab must come right after the sections its relocations refer to; fix its index
    symtab_position = len(headers) + 1
    headers = [
        (n, t, f, symtab_position if t == SHT_RELA else lk, i, a, e) for n, t, f, lk, i, a, e in headers
    ]
    headers.append((".symtab", SHT_SYMTAB, 0, symtab_position + 1, first_global, 8, 24))
    blobs.append(bytes(symtab))
    sizes.append(len(symtab))
    headers.append((".strtab", SHT_STRTAB, 0, 0, 0, 1, 0))
    blobs.append(bytes(strtab))
    sizes.append(len(strtab))
    headers.append((".shstrtab", SHT_STRTAB, 0, 0, 0, 1, 0))
    names = [add(shstrtab, h[0]) for h in headers]
    blobs.append(bytes(shstrtab))
    sizes.append(len(shstrtab))

    body = bytearray(64)
    offsets = []
    for blob in blobs:
        while len(body) % 8:
            body.append(0)
        offsets.append(len(body))
        body += blob
    while len(body) % 8:
        body.append(0)
    shoff = len(body)
    body += bytes(64)
    for (_, stype, flags, link, info, align, entsize), name_offset, offset, size in zip(
        headers, names, offsets, sizes, strict=True
    ):
        body += struct.pack(
            "<IIQQQQIIQQ", name_offset, stype, flags, 0, offset, size, link, info, align, entsize
        )
    ident = b"\x7fELF\x02\x01\x01" + bytes(9)
    struct.pack_into(
        "<16sHHIQQQIHHHHHH", body, 0, ident, 1, 62, 1, 0, 0, shoff, 0, 64, 0, 0, 64, len(headers) + 1,
        len(headers),
    )  # fmt: skip
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(body))
