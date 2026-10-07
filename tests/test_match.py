"""Matcher tests: synthetic ELF fixtures everywhere, the real unit with originals and the toolchain."""

import os
import shutil
import struct

import pytest

from hv import builds, cli, match, symbols, toolchain, units
from hv.elf import Elf
from hv.match import compare_object

NAME = "_Z5pilotv"
ADDRESS = 0x401000
CALL = b"\xe8\0\0\0\0\xc3"
DESTINATION = 0x402000


def elf_image(code, *, kind=1, size=None, relocations=(), machine=62, flags=6):
    """A minimal ELF64 with `.text` holding one global function and optional RELA entries.

    Relocations are (offset, type, addend, symbol name); relocation symbols are undefined.
    """
    names = [NAME, *dict.fromkeys(r[3] for r in relocations)]
    strings = b"\0"
    offsets = {}
    for name in names:
        offsets[name] = len(strings)
        strings += name.encode() + b"\0"
    symtab = bytes(24)
    symtab += struct.pack(
        "<IBBHQQ", offsets[NAME], 0x12, 0, 1, ADDRESS if kind == 2 else 0, size or len(code)
    )
    for name in names[1:]:
        symtab += struct.pack("<IBBHQQ", offsets[name], 0x10, 0, 0, 0, 0)
    relas = b"".join(
        struct.pack("<QQq", offset, (names.index(name) + 1) << 32 | rtype, addend)
        for offset, rtype, addend, name in relocations
    )
    section_names = [".text", ".strtab", ".symtab", ".rela.text", ".shstrtab"]
    shstrings = b"\0"
    shnames = {}
    for name in section_names:
        shnames[name] = len(shstrings)
        shstrings += name.encode() + b"\0"
    payloads = [code, strings, symtab, relas, shstrings]
    headers = [bytes(64)]
    body = bytearray(64)
    for i, (name, payload) in enumerate(zip(section_names, payloads, strict=True), 1):
        offset = len(body)
        body.extend(payload)
        stype = {1: 1, 2: 3, 3: 2, 4: 4, 5: 3}[i]
        link = {3: 2, 4: 3}.get(i, 0)
        info = {3: 1, 4: 1}.get(i, 0)
        entsize = 24 if i in (3, 4) else 0
        address = ADDRESS if i == 1 and kind == 2 else 0
        headers.append(
            struct.pack(
                "<IIQQQQIIQQ",
                shnames[name],
                stype,
                flags if i == 1 else 0,
                address,
                offset,
                len(payload),
                link,
                info,
                1,
                entsize,
            )  # fmt: skip
        )
    shoff = len(body)
    body.extend(b"".join(headers))
    ident = b"\x7fELF\x02\x01\x01" + bytes(9)
    body[:64] = struct.pack(
        "<16sHHIQQQIHHHHHH", ident, kind, machine, 1, 0, 0, shoff, 0, 64, 0, 0, 64, len(headers), 5
    )
    return bytes(body)


def target_image(code=None, extent=None):
    code = code or b"\xe8" + (DESTINATION - ADDRESS - 5).to_bytes(4, "little", signed=True) + b"\xc3"
    target = Elf(elf_image(code, kind=2), "ET_EXEC")
    target.fde_ranges = lambda: {(ADDRESS, extent or len(code))}
    return target


def compare(code=CALL, *, relocations=((1, 2, -4, "memcpy"),), known=None, target=None, size=None):
    obj = Elf(elf_image(code, size=size, relocations=relocations), "ET_REL")
    known = {NAME: ADDRESS, "memcpy": DESTINATION} if known is None else known
    return compare_object(obj, target or target_image(), known, {})


def text(result):
    (section,) = result["sections"]
    return section


def test_data_credit_requires_the_complete_relocated_extent():
    # An allocated non-executable section records a proof only after the whole section matches.
    obj = Elf(elf_image(b"\x01\x02\x03\x04", flags=2), "ET_REL")
    target = Elf(elf_image(b"\x01\x02\x03\x04", kind=2, flags=2), "ET_EXEC")
    target.fde_ranges = lambda: set()
    result = compare_object(obj, target, {NAME: ADDRESS}, {})
    assert text(result)["data_ranges"] == [{"address": hex(ADDRESS), "size": 4, "kind": "section"}]
    changed = Elf(elf_image(b"\x01\x02\x03\x05", kind=2, flags=2), "ET_EXEC")
    changed.fde_ranges = lambda: set()
    result = compare_object(obj, changed, {NAME: ADDRESS}, {})
    assert not result["exact"] and "data_ranges" not in text(result)


@pytest.mark.parametrize("kind", [2, 4])
def test_call_resolves_to_its_destination(kind):
    result = compare(relocations=[(1, kind, -4, "memcpy")])
    assert result["exact"]
    assert text(result)["references"] == 1
    assert text(result)["functions"][0]["exact"]


def test_changed_call_target_fails():
    result = compare(relocations=[(1, 2, -4, "memmove")], known={NAME: ADDRESS, "memmove": 0x403000})
    assert not result["exact"]
    (ref,) = text(result)["bad_references"]
    assert ref["reason"] == "different destination"


def test_changed_addend_fails():
    assert not compare(relocations=[(1, 2, -3, "memcpy")])["exact"]


def test_unknown_symbol_cannot_pass_even_if_raw_bytes_match():
    raw = b"\xe8" + (DESTINATION - ADDRESS - 5).to_bytes(4, "little", signed=True) + b"\xc3"
    result = compare(raw, known={NAME: ADDRESS})
    assert not result["exact"]
    assert text(result)["bad_references"][0]["reason"] == "no target address for symbol"


def test_unsupported_relocation_cannot_pass():
    result = compare(relocations=[(1, 24, -4, "memcpy")])  # R_X86_64_PC64
    assert text(result)["bad_references"][0]["reason"] == "unsupported relocation type"


def test_overflow_cannot_pass():
    result = compare(known={NAME: ADDRESS, "memcpy": 1 << 40})
    assert text(result)["bad_references"][0]["reason"] == "relocation overflow"


@pytest.mark.parametrize("offset", [3, 6])
def test_relocation_outside_section_rejected(offset):
    with pytest.raises(ValueError, match="outside"):
        compare(relocations=[(offset, 2, -4, "memcpy")])


def test_overlapping_relocations_rejected():
    with pytest.raises(ValueError, match="overlapping"):
        compare(relocations=[(1, 2, -4, "memcpy"), (2, 2, -4, "memcpy")])


def test_changed_constant_rejected():
    good, bad = b"\xb8\x01\0\0\0\xc3", b"\xb8\x02\0\0\0\xc3"
    target = target_image(good)
    assert compare(good, relocations=(), target=target)["exact"]
    result = compare(bad, relocations=(), target=target)
    assert not result["exact"]
    assert text(result)["first_difference"] == 1


def test_function_extent_must_match_target_fde():
    # identical bytes, but the target's function is longer than ours
    result = compare(target=target_image(extent=7))
    assert not result["exact"]
    assert text(result)["functions"][0]["extent"] is None


def test_thunk_extent_proves_a_function_without_an_fde():
    # GCC emits no FDE for thunks; a thunk extent from extents.tsv proves the extent instead
    target = target_image()
    ((_, size),) = target.fde_ranges()
    target.fde_ranges = lambda: set()
    assert not compare(target=target)["exact"]
    target.thunks = frozenset({(ADDRESS, size)})
    result = compare(target=target)
    assert result["exact"]
    assert text(result)["functions"][0]["extent"] == "thunk"


def test_section_without_known_symbol_is_unplaced():
    result = compare(known={"memcpy": DESTINATION})
    assert not result["exact"]
    assert result["unplaced_sections"] == [".text"]


def test_explicit_placement_contradicting_a_symbol_is_inexact():
    obj = Elf(elf_image(CALL, relocations=[(1, 2, -4, "memcpy")]), "ET_REL")
    known = {NAME: ADDRESS, "memcpy": DESTINATION}
    result = compare_object(obj, target_image(), known, {".text": ADDRESS + 16})
    assert not result["exact"]
    (moved,) = text(result)["misplaced_symbols"]
    assert moved == {"symbol": NAME, "known": hex(ADDRESS), "placed": hex(ADDRESS + 16)}
    with pytest.raises(ValueError, match="not in the object"):
        compare_object(obj, target_image(), known, {".data": ADDRESS})


def test_wrong_elf_architecture_and_kind_rejected():
    with pytest.raises(ValueError, match="expected"):
        Elf(elf_image(b"\xc3", machine=3), "ET_REL")
    with pytest.raises(ValueError, match="expected"):
        Elf(elf_image(b"\xc3"), "ET_EXEC")


def test_depfile_parsing():
    text_ = (
        "/out/x.o: src/ox/io/CMemReadFile.cpp src/ox/io/CMemReadFile.h \\\n /usr/include/c++/4.4/iostream\n"
    )
    assert toolchain.parse_depfile(text_) == [
        "src/ox/io/CMemReadFile.cpp",
        "src/ox/io/CMemReadFile.h",
        "/usr/include/c++/4.4/iostream",
    ]


def test_cli_rejects_unknown_unit(capsys, monkeypatch):
    # This argument-validation test does not need a proprietary image.
    monkeypatch.setattr(builds, "check_image", lambda image: None)
    assert cli.main(["match", "no/such/File.cpp"]) == 2
    assert "not in units.toml" in capsys.readouterr().err


BUILD = "1.18-linux-amd64"
UNIT = "ox/io/CMemReadFile.cpp"


@pytest.fixture(scope="module")
def linux_target():
    image = builds.load_builds()[BUILD].images["Harvest"]
    if builds.check_image(image) is not None:
        pytest.skip("pinned Linux image not present under orig/")
    return Elf.load(image.path, "ET_EXEC")


@pytest.fixture(scope="module")
def compiler(tmp_path_factory):
    if os.environ.get("HARVEST_TEST_TOOLCHAIN") != "1":
        pytest.skip("set HARVEST_TEST_TOOLCHAIN=1 to compile with Docker")
    return toolchain.Compiler(toolchain.load_flags(BUILD), tmp_path_factory.mktemp("compile"))


def compile_variant(compiler, name, edits):
    """Compile a copy of src/ox with (file, before, after) edits applied."""
    root = compiler.out / name
    shutil.copytree(builds.ROOT / "src" / "ox", root / "ox")
    for relative, before, after in edits:
        path = root / relative
        source = path.read_text()
        assert source.count(before) == 1, before
        path.write_text(source.replace(before, after))
    obj, _ = compiler.compile(root / UNIT, name)
    return Elf.load(obj, "ET_REL")


def unit_result(obj, linux_target):
    (unit,) = [u for u in units.load(BUILD) if u.source == UNIT]
    return compare_object(obj, linux_target, symbols.by_name(symbols.load(BUILD)), unit.placements)


@pytest.mark.originals
@pytest.mark.toolchain
def test_unit_matches_and_mutations_are_rejected(linux_target, compiler):
    assert unit_result(compile_variant(compiler, "clean", []), linux_target)["exact"]
    mutations = {
        "constant": [(UNIT, "return Len - Pos;", "return Len - Pos + 1;")],
        "call": [(UNIT, "memcpy(buffer", "memmove(buffer")],
        # swapping two virtuals keeps every function body but reorders the vtable
        "vtable": [
            (
                "ox/io/CMemReadFile.h",
                "    virtual void* getCurrentPointer();\n    virtual int getRemainingSize();\n",
                "    virtual int getRemainingSize();\n    virtual void* getCurrentPointer();\n",
            )
        ],
    }
    for name, edits in mutations.items():
        result = unit_result(compile_variant(compiler, name, edits), linux_target)
        assert not result["exact"], name


@pytest.mark.originals
@pytest.mark.toolchain
def test_source_overlay_compiles_at_the_canonical_path(linux_target, compiler):
    source = builds.ROOT / "src" / UNIT
    original = source.read_bytes()
    replacement = compiler.out / "overlay.cpp"
    replacement.write_bytes(original)
    obj, metadata = compiler.compile(source, "overlay-clean", source_override=replacement)
    assert unit_result(Elf.load(obj, "ET_REL"), linux_target)["exact"]
    assert metadata["inputs"][f"src/{UNIT}"] == match.digest(original)
    assert f"src/{UNIT}" in metadata["command"]
    mutated = original.replace(b"return Len - Pos;", b"return Len - Pos + 1;")
    assert mutated != original
    replacement.write_bytes(mutated)
    obj, metadata = compiler.compile(source, "overlay-mutated", source_override=replacement)
    assert not unit_result(Elf.load(obj, "ET_REL"), linux_target)["exact"]
    assert metadata["inputs"][f"src/{UNIT}"] == match.digest(mutated)
    assert source.read_bytes() == original


def test_learning_reports_disagreeing_references_in_one_function():
    function = {
        "symbol": NAME,
        "exact": False,
        "global": True,
        "exact_but_unknown": True,
        "candidates": [["helper", "0x402000"], ["helper", "0x403000"], ["other", "0x404000"]],
    }
    learned, conflicts = match.learnable({"sections": [{"functions": [function]}]}, "u.cpp", {})
    assert conflicts == ["helper"]
    assert learned == {"other": (0x404000, f"reloc:u.cpp:{NAME}")}


def test_learning_records_exact_global_functions():
    functions = [
        {"symbol": "f", "address": "0x401000", "exact": True, "global": True},
        {"symbol": "T.1", "address": "0x401010", "exact": True, "global": False},
        {"symbol": "g", "address": "0x401020", "exact": False, "global": True},
        {"symbol": "k", "address": "0x401030", "exact": True, "global": True},
    ]
    learned, _ = match.learnable({"sections": [{"functions": functions}]}, "u.cpp", {"k": 0x401030})
    assert learned == {"f": (0x401000, "match:u.cpp")}


class FakeSection(dict):
    def __init__(self, data, entsize, flags=0x30):
        super().__init__(sh_entsize=entsize, sh_flags=flags)
        self._data = data

    def data(self):
        return self._data


@pytest.mark.parametrize(
    ("data", "offset", "reason"),
    [
        (b"\0\0\0\0", 1, "not aligned"),  # one byte into an empty wide string
        (b"a\0\0\0b\0\0", 4, "unterminated"),  # the last element is incomplete
        (b"a\0\0\0", 0, "unterminated"),  # no zero element
    ],
)
def test_merged_string_check_rejects_malformed_references(data, offset, reason):
    ok, message = match.check_merged(target_image(), FakeSection(data, 4), offset, ADDRESS)
    assert not ok and reason in message


def test_merged_constant_must_be_complete():
    ok, message = match.check_merged(target_image(), FakeSection(b"\0\0\0\0\0\0", 4, flags=0x10), 4, ADDRESS)
    assert not ok and "incomplete" in message


class FakeSymbol(dict):
    def __init__(self, name, value, size):
        super().__init__(st_shndx=1, st_info={"type": "STT_FUNC"}, st_value=value, st_size=size)
        self.name = name


class FakeResolver:
    def __init__(self, known):
        self.known = known


def test_unnamed_function_follows_its_predecessor():
    # ours: a (0x40), b (0x20); the target's a is 0x10 longer, so b sits 0x10 later than base + 0x40
    symbols = [FakeSymbol("a", 0x0, 0x40), FakeSymbol("b", 0x40, 0x20), FakeSymbol("c", 0x60, 0x20)]
    fdes = {(0x1000, 0x50), (0x1050, 0x20), (0x1070, 0x20)}
    layout = match.layout_functions(1, 0x1000, symbols, FakeResolver({"a": 0x1000}), fdes)
    assert layout.address_of(0x40) == 0x1050
    # c has a twin FDE of its size, so it keeps following b
    assert layout.address_of(0x60) == 0x1070
    assert not layout.contiguous


def inline_copy_reading_a_static(
    tmp_path, group, other_table, *, writable=False, target_writable=False, store=False
):
    """Our object: an inline function reading a file-level static table through the `.rodata`
    section symbol. The target keeps the linked copy of the function from another object, which
    reads that object's copy of the table, not ours."""
    from hv.delink import Section, Symbol, write_object

    table = bytes(range(1, 9))
    flags = 0x206 if group else 0x6  # SHF_ALLOC | SHF_EXECINSTR, plus SHF_GROUP for an inline copy
    opcode = b"\x89" if store else b"\x8b"
    code = opcode + b"\x04\x85\0\0\0\0\xc3"  # mov [rax*4 + table], eax (or its load); ret
    data_section = ".data" if writable else ".rodata"
    sections = [
        Section(".text._Z3getv", code, flags, relocations=[(3, 11, data_section, 0x10)]),
        Section(data_section, bytes(0x10) + table, 0x3 if writable else 0x2),
    ]
    symbols = [
        Symbol("_Z3getv", ".text._Z3getv", 0, len(code), function=True),
        Symbol("TABLE", data_section, 0x10, 8, local=True),
    ]
    write_object(tmp_path / "unit.o", sections, symbols)
    obj = Elf.load(tmp_path / "unit.o", "ET_REL")
    # target: the function, our unit's .rodata at +0x10 and the other object's table at +0x30
    other = ADDRESS + 0x30
    image = (
        code[:3]
        + other.to_bytes(4, "little")
        + code[7:]
        + bytes(8)
        + bytes(0x10)
        + table
        + bytes(8)
        + other_table
    )
    target = Elf(elf_image(image, kind=2, flags=0x7 if target_writable else 0x6), "ET_EXEC")
    target.fde_ranges = lambda: {(ADDRESS, len(code))}
    return compare_object(obj, target, {"_Z3getv": ADDRESS}, {data_section: ADDRESS + 0x10})


def function_section(result):
    return next(s for s in result["sections"] if s["name"].startswith(".text"))


def test_inline_copy_may_read_another_objects_copy_of_a_static(tmp_path):
    assert inline_copy_reading_a_static(tmp_path, True, bytes(range(1, 9)))["exact"]


def test_other_copy_of_a_static_must_hold_the_same_bytes(tmp_path):
    result = inline_copy_reading_a_static(tmp_path, True, bytes(range(2, 10)))
    assert not result["exact"]
    assert function_section(result)["bad_references"][0]["reason"] == "local copy"


def test_only_inline_copies_may_read_another_copy_of_a_static(tmp_path):
    result = inline_copy_reading_a_static(tmp_path, False, bytes(range(1, 9)))
    assert not result["exact"]
    assert function_section(result)["bad_references"][0]["reason"] == "different destination"


@pytest.mark.parametrize("store", [False, True])
def test_inline_copy_cannot_substitute_a_mutable_static(tmp_path, store):
    result = inline_copy_reading_a_static(tmp_path, True, bytes(range(1, 9)), writable=True, store=store)
    assert not result["exact"]
    assert function_section(result)["bad_references"][0]["reason"] == "mutable local object"


def test_inline_copy_cannot_substitute_a_writable_target_copy(tmp_path):
    result = inline_copy_reading_a_static(tmp_path, True, bytes(range(1, 9)), target_writable=True)
    assert not result["exact"]
    assert function_section(result)["bad_references"][0]["reason"] == "mutable or unmapped target copy"


def bss_result(tmp_path, size, address=ADDRESS, known=None):
    from hv.delink import Section, Symbol, write_object

    # The target has exactly four allocated NOBITS bytes at ADDRESS.
    data = bytearray(elf_image(bytes(4), kind=2, flags=0x3))
    shoff = struct.unpack_from("<Q", data, 40)[0]
    struct.pack_into("<I", data, shoff + 64 + 4, 8)  # SHT_NOBITS
    target = Elf(bytes(data), "ET_EXEC")
    write_object(
        tmp_path / "bss.o",
        [Section(".bss", bytes(size), 0x3, nobits=True)],
        [Symbol("g", ".bss", 0, size)],
    )
    return compare_object(Elf.load(tmp_path / "bss.o", "ET_REL"), target, known or {}, {".bss": address})


@pytest.mark.parametrize(
    ("size", "offset", "exact"), [(4, 0, True), (5, 0, False), (1, 3, True), (2, 3, False)]
)
def test_bss_placement_requires_the_entire_extent(tmp_path, size, offset, exact):
    assert bss_result(tmp_path, size, ADDRESS + offset)["exact"] == exact


def test_bss_placement_must_agree_with_known_symbols(tmp_path):
    result = bss_result(tmp_path, 1, ADDRESS + 1, {"g": ADDRESS})
    assert not result["exact"]
    assert result["sections"][0]["misplaced_symbols"] == [
        {"symbol": "g", "known": hex(ADDRESS), "placed": hex(ADDRESS + 1)}
    ]


def function_with_exception_table(tmp_path, target_lsda):
    """Our object: one function whose FDE points to its exception table (LSDA). Only the FDE
    references the table, so it is placed from the target FDE's LSDA pointer."""
    from hv.delink import Section, Symbol, write_object

    code, lsda = b"\x90\x90\x90\xc3", b"\xff\x03\x05\x01\x00\x00\x00\x00"
    cie = struct.pack("<II", 12, 0) + bytes(8)
    fde = struct.pack("<II", 20, len(cie) + 4) + bytes(16)  # CIE pointer, pc_begin, range, LSDA
    sections = [
        Section(".text", code, 0x6),
        Section(".gcc_except_table", lsda, 0x2, align=4),
        Section(
            ".eh_frame",
            cie + fde,
            0x2,
            align=8,
            relocations=[(len(cie) + 8, 2, ".text", 0), (len(cie) + 16, 10, ".gcc_except_table", 0)],
        ),
    ]
    write_object(tmp_path / "unit.o", sections, [Symbol("f", ".text", 0, len(code), function=True)])
    obj = Elf.load(tmp_path / "unit.o", "ET_REL")
    target = Elf(elf_image(code + bytes(0x1C) + target_lsda, kind=2), "ET_EXEC")
    target.fde_ranges = lambda: {(ADDRESS, len(code))}
    target.fde_lsdas = lambda: {ADDRESS: ADDRESS + 0x20}
    result = compare_object(obj, target, {"f": ADDRESS}, {})
    return next(s for s in result["sections"] if s["name"] == ".gcc_except_table")


def test_exception_table_is_placed_from_the_target_fde(tmp_path):
    table = function_with_exception_table(tmp_path, b"\xff\x03\x05\x01\x00\x00\x00\x00")
    assert (table["address"], table["placement"], table["exact"]) == (
        hex(ADDRESS + 0x20),
        "exception frames",
        True,
    )


def test_placed_exception_table_is_compared(tmp_path):
    assert not function_with_exception_table(tmp_path, b"\xff\x03\x05\x02\x00\x00\x00\x00")["exact"]
