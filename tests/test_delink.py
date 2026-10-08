import io
import struct

import pytest
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

from hv import delink
from hv.elf import Elf


def read(path):
    return ELFFile(io.BytesIO(path.read_bytes()))


def wide(text):
    """A wide string as the target holds it: four-byte characters and a four-byte terminator."""
    return text.encode("utf-32-le") + bytes(4)


def test_write_object_round_trips(tmp_path):
    code = delink.Section(".text", b"\xe8\x11\x11\x11\x11\xc3", delink.SHF_ALLOC | delink.SHF_EXECINSTR)
    code.relocations.append((1, delink.R_X86_64_PC32, "memcpy", -4))
    bss = delink.Section(".bss", bytes(4), delink.SHF_ALLOC | 0x1, nobits=True)
    data = delink.Section(".rodata.x", bytes(8), delink.SHF_ALLOC)
    data.relocations.append((0, delink.R_X86_64_64, ".bss", 2))
    symbols = [
        delink.Symbol("f", ".text", 0, 6, function=True),
        delink.Symbol("T.1", ".text", 5, 1, function=True, local=True),
    ]
    path = tmp_path / "t.o"
    delink.write_object(path, [code, bss, data], symbols)

    elf = read(path)
    assert elf["e_type"] == "ET_REL" and elf["e_machine"] == "EM_X86_64"
    text = elf.get_section_by_name(".text")
    assert text.data() == b"\xe8\0\0\0\0\xc3", "relocated fields are zeroed"
    assert elf.get_section_by_name(".bss")["sh_type"] == "SHT_NOBITS"
    assert elf.get_section_by_name(".bss")["sh_size"] == 4

    table = elf.get_section_by_name(".symtab")
    names = {s.name: s for s in table.iter_symbols()}
    assert names["f"]["st_info"]["bind"] == "STB_GLOBAL"
    assert names["T.1"]["st_info"]["bind"] == "STB_LOCAL"
    assert names["memcpy"]["st_shndx"] == "SHN_UNDEF"
    locals_ = [s for s in table.iter_symbols() if s["st_info"]["bind"] == "STB_LOCAL"]
    assert table["sh_info"] == len(locals_), "sh_info is the first global symbol"

    relocations = {}
    for section in elf.iter_sections():
        if isinstance(section, RelocationSection):
            target = elf.get_section(section["sh_info"]).name
            for r in section.iter_relocations():
                sym = table.get_symbol(r["r_info_sym"])
                name = sym.name or elf.get_section(sym["st_shndx"]).name
                relocations[(target, r["r_offset"])] = (r["r_info_type"], name, r["r_addend"])
    assert relocations[(".text", 1)] == (delink.R_X86_64_PC32, "memcpy", -4)
    # a reference to a section this object defines goes through a section symbol
    assert relocations[(".rodata.x", 0)] == (delink.R_X86_64_64, ".bss", 2)


def test_merged_strings_index_by_content(tmp_path):
    section = delink.Section(".rodata.str1.1", b"GET \0MemFile\0", delink.SHF_ALLOC | 0x30, align=1)
    path = tmp_path / "s.o"
    delink.write_object(path, [section], [])
    from hv.elf import Elf

    strings = delink.merged_strings(Elf.load(path, "ET_REL"))
    assert strings[b"MemFile\0"] == (".rodata.str1.1", 5)
    assert strings[b"GET \0"] == (".rodata.str1.1", 0)


@pytest.mark.parametrize("entsize", [1, 4])
def test_merged_strings_skip_the_padding_between_aligned_strings(tmp_path, entsize):
    # GCC aligns each string of .rodata.str1.8 and .rodata.str4.8 to 8 bytes; the zeros before the next
    # one are padding, so the empty string stays the one of the section that really has it
    first, second = (b"ab\0", b"c\0") if entsize == 1 else (wide("ab"), wide("c"))
    padding = bytes(-len(first) % 8)
    section = delink.Section(
        f".rodata.str{entsize}.8", first + padding + second, delink.SHF_ALLOC | 0x30, align=8, entsize=entsize
    )
    delink.write_object(tmp_path / "s.o", [section], [])
    strings = delink.merged_strings(Elf.load(tmp_path / "s.o", "ET_REL"))
    assert strings == {first: (section.name, 0), second: (section.name, len(first) + len(padding))}


def test_target_symbol_takes_the_target_extent():
    from test_match import ADDRESS, NAME, elf_image

    from hv.elf import Elf

    # the target function is seven bytes; ours is six
    target = Elf(elf_image(b"\x90" * 6 + b"\xc3", kind=2), "ET_EXEC")
    obj = Elf(elf_image(b"\x90" * 5 + b"\xc3"), "ET_REL")
    result = {"sections": [{"name": ".text", "address": hex(ADDRESS), "functions": [
        {"symbol": NAME, "address": hex(ADDRESS), "size": 6}]}]}  # fmt: skip
    sections, symbols = delink.delink_unit(target, obj, result, [], {ADDRESS: 7})
    (symbol,) = [s for s in symbols if s.name == NAME]
    assert symbol.size == 7
    (text,) = [s for s in sections if s.name == ".text"]
    assert text.data[:7] == b"\x90" * 6 + b"\xc3"


def target_with_rodata(tmp_path, code, data):
    from test_match import ADDRESS, DESTINATION

    path = tmp_path / "target.elf"
    delink.write_object(path, [delink.Section(".text", code, 0x6), delink.Section(".rodata", data, 0x2)], [])
    raw = bytearray(path.read_bytes())
    struct.pack_into("<H", raw, 16, 2)  # ET_EXEC
    shoff = struct.unpack_from("<Q", raw, 40)[0]
    for index, address in ((1, ADDRESS), (2, DESTINATION)):
        struct.pack_into("<Q", raw, shoff + index * 64 + 16, address)
    target = Elf(bytes(raw), "ET_EXEC")
    target.fde_ranges = lambda: {(ADDRESS, len(code))}
    return target


@pytest.mark.parametrize("opcode", [b"\x8b\x04\x85", b"\x48\x8b\x04\xc5"])
def test_indexed_absolute_address_recovers_the_table_relocation(tmp_path, opcode):
    from test_match import ADDRESS, DESTINATION

    code = opcode + DESTINATION.to_bytes(4, "little") + b"\xc3"
    target = target_with_rodata(tmp_path, code, bytes(8))
    namer = delink.Namer(target, [(DESTINATION, 8, "TABLE")], {}, [], {})
    assert delink.code_relocations(target, ADDRESS, code, namer) == [
        (len(opcode), delink.R_X86_64_32S, "TABLE", 0)
    ]


def test_metadata_strings_are_not_literal_candidates(tmp_path):
    delink.write_object(tmp_path / "comment.o", [delink.Section(".comment", b"\0GCC\0", 0x30)], [])
    assert delink.merged_strings(Elf.load(tmp_path / "comment.o", "ET_REL")) == {}


def test_unknown_binary_literal_keeps_the_largest_access_width(tmp_path):
    from test_match import DESTINATION

    data = bytes(range(8))
    target = target_with_rodata(tmp_path, b"\xc3", data)
    namer = delink.Namer(target, [], {}, [], {})
    assert namer.name(DESTINATION, 8) == namer.name(DESTINATION, 4)
    assert namer.literals[DESTINATION] == data


@pytest.mark.parametrize("changed", [False, True])
def test_indexed_static_copy_requires_the_entire_table_to_match(tmp_path, changed):
    from test_match import DESTINATION

    table = bytes(range(8))
    data = table[:-1] + b"\xff" if changed else table
    target = target_with_rodata(tmp_path, b"\xc3", data)
    namer = delink.Namer(target, [], {}, [], {}, copies={"TABLE": table})
    name, offset = namer.name(DESTINATION, 4)
    assert offset == 0
    assert name == (f"lbl_{DESTINATION:x}" if changed else "TABLE")


@pytest.mark.parametrize("value", [bytes(4), struct.pack("<f", 1.0)])
def test_float_access_keeps_its_binary_value_and_merge_type(tmp_path, value):
    from test_match import ADDRESS, DESTINATION

    from hv.match import compare_object

    # ucomiss xmm0, [rip + zero]; ret. An allocated empty string and .comment must not
    # turn the first zero byte of this float into a one-byte string literal.
    code = b"\x0f\x2e\x05" + bytes(4) + b"\xc3"
    sections = [
        delink.Section(".text", code, 0x6, relocations=[(3, delink.R_X86_64_PC32, ".LC0", -4)]),
        delink.Section(".rodata.cst4", bytes(4), 0x12, align=4, entsize=4),
        delink.Section(".rodata.str1.1", b"\0", 0x32, align=1),
        delink.Section(".comment", b"\0GCC\0", 0x30, align=1),
    ]
    delink.write_object(
        tmp_path / "float.o",
        sections,
        [
            delink.Symbol("f", ".text", 0, len(code), function=True),
            delink.Symbol(".LC0", ".rodata.cst4", 0, local=True),
        ],
    )
    obj = Elf.load(tmp_path / "float.o", "ET_REL")
    linked = code[:3] + (DESTINATION - ADDRESS - 7).to_bytes(4, "little", signed=True) + code[7:]
    target = target_with_rodata(tmp_path, linked, value)
    result = compare_object(obj, target, {"f": ADDRESS}, {})
    assert result["exact"] == (value == bytes(4))
    sections, syms = delink.delink_unit(target, obj, result, [], {ADDRESS: len(code)})
    expected = ".rodata.cst4" if value == bytes(4) else ".rodata.lit"
    assert {s.name for s in sections} == {".text", expected}
    data = next(s for s in sections if s.name == expected)
    assert data.data == value
    assert not data.flags & delink.SHF_STRINGS
    delink.write_object(tmp_path / "delinked.o", sections, syms)
    exported = Elf.load(tmp_path / "delinked.o", "ET_REL")
    if value == bytes(4):
        assert exported.elf.get_section_by_name(expected)["sh_entsize"] == 4
    [(relocation, symbol)] = list(exported.relocations(1))
    assert relocation["r_addend"] == -4
    name = symbol.name or exported.elf.get_section(symbol["st_shndx"]).name
    assert name == (expected if value == bytes(4) else f"lbl_{DESTINATION:x}")


def test_wide_string_ends_at_its_first_zero_character(tmp_path):
    from test_match import DESTINATION

    # "a" and its terminator, with zero bytes that straddle two characters
    target = target_with_rodata(tmp_path, b"\xc3", wide("a") + wide("bc"))
    assert target.cstring(DESTINATION, 4) == b"a\0\0\0"
    assert target.cstring(DESTINATION) == b"a"
    with pytest.raises(ValueError, match="unterminated"):
        target_with_rodata(tmp_path, b"\xc3", b"a\0\0\0b\0\0\0").cstring(DESTINATION, 4)


@pytest.mark.parametrize("size", [None, 4])
def test_wide_string_is_resolved_whole_by_a_pointer_or_a_load_of_its_first_character(tmp_path, size):
    from test_match import DESTINATION

    # Read narrow, the string would be "a", which our object also has
    target = target_with_rodata(tmp_path, b"\xc3", wide("ab"))
    strings = {b"a\0": (".rodata.str1.1", 0), wide("ab"): (".rodata.str4.4", 8)}
    namer = delink.Namer(target, [], {}, [], strings)
    assert namer.name(DESTINATION, size) == (".rodata.str4.4", 8)
    assert namer.merged == {".rodata.str4.4": {8: wide("ab")}}
    assert not namer.literals


def test_empty_wide_string_is_not_resolved_to_the_empty_narrow_string(tmp_path):
    from test_match import DESTINATION

    target = target_with_rodata(tmp_path, b"\xc3", wide("") + wide("="))
    strings = {b"\0": (".rodata.str1.1", 0), wide(""): (".rodata.str4.4", 0x14)}
    namer = delink.Namer(target, [], {}, [], strings)
    assert namer.name(DESTINATION) == (".rodata.str4.4", 0x14)


def test_narrow_string_is_not_resolved_to_a_wide_string(tmp_path):
    from test_match import DESTINATION

    # the empty string before "abc"; the characters read as a wide string end at the zero after them
    target = target_with_rodata(tmp_path, b"\xc3", b"\0abc\0\0\0\0")
    strings = {b"\0": (".rodata.str1.1", 0), wide(""): (".rodata.str4.4", 0x14)}
    assert delink.Namer(target, [], {}, [], strings).name(DESTINATION) == (".rodata.str1.1", 0)


def test_loaded_binary_constant_wins_over_a_wide_string(tmp_path):
    from test_match import DESTINATION

    # A zero float and an empty wide string are the same four bytes; the access tells them apart
    target = target_with_rodata(tmp_path, b"\xc3", bytes(8))
    namer = delink.Namer(
        target, [], {}, [], {wide(""): (".rodata.str4.4", 0)}, {bytes(4): (".rodata.cst4", 4)}
    )
    assert namer.name(DESTINATION, 4) == (".rodata.cst4", 4)
    assert namer.name(DESTINATION) == (".rodata.str4.4", 0)


def test_wide_string_reference_goes_to_a_target_copy_of_our_wide_string_section(tmp_path):
    from test_match import ADDRESS, DESTINATION

    code = b"\xbf" + bytes(4) + b"\xc3"  # mov edi, offset L"ab"; ret
    text = delink.Section(".text", code, 0x6, relocations=[(1, delink.R_X86_64_32, ".rodata.str4.4", 4)])
    strings = delink.Section(
        ".rodata.str4.4", wide("") + wide("ab"), delink.SHF_ALLOC | 0x30, align=4, entsize=4
    )
    delink.write_object(
        tmp_path / "unit.o", [text, strings], [delink.Symbol("f", ".text", 0, len(code), function=True)]
    )
    obj = Elf.load(tmp_path / "unit.o", "ET_REL")
    linked = code[:1] + DESTINATION.to_bytes(4, "little") + code[5:]
    target = target_with_rodata(tmp_path, linked, wide("ab"))
    result = {"sections": [{"name": ".text", "address": hex(ADDRESS), "functions": [
        {"symbol": "f", "address": hex(ADDRESS), "size": len(code)}]}]}  # fmt: skip
    sections, symbols = delink.delink_unit(target, obj, result, [], {ADDRESS: len(code)})
    assert {s.name for s in sections} == {".text", ".rodata.str4.4"}
    (text,) = [s for s in sections if s.name == ".text"]
    assert text.relocations == [(1, delink.R_X86_64_32, ".rodata.str4.4", 4)]
    (copy,) = [s for s in sections if s.name == ".rodata.str4.4"]
    assert copy.data == bytes(4) + wide("ab")
    assert (copy.flags, copy.align, copy.entsize) == (delink.SHF_ALLOC | 0x30, 4, 4)
