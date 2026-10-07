"""Independent thunk boundaries: instruction operands, vtable provenance and accounting."""

import copy
import hashlib
import struct
from types import SimpleNamespace

import pytest
from capstone import CS_ARCH_X86, CS_MODE_64, Cs
from test_match import ADDRESS, NAME, compare_object, elf_image, text
from test_progress import sample

from hv import builds, extents, progress, rtti, symbols
from hv.elf import Elf

DESTINATION = ADDRESS + 0x100
ADD = bytes.fromhex("4883c7e8")
SRET_ADD = bytes.fromhex("4881c6d8feffff")
VIRTUAL_ADD = bytes.fromhex("4c8b1749037ae8")


def thunk(prefix=ADD):
    return prefix + b"\xe9" + struct.pack("<i", DESTINATION - ADDRESS - len(prefix) - 5)


def image(code):
    target = Elf(elf_image(code, kind=2), "ET_EXEC")
    target.fde_ranges = lambda: {(DESTINATION, 16)}
    return target


@pytest.mark.parametrize("prefix", [ADD, SRET_ADD, VIRTUAL_ADD])
def test_supported_adjustment_operands_and_full_extent(prefix):
    code = thunk(prefix)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    decoded = extents.thunk(decoder, code, ADDRESS)
    assert decoded is not None and decoded[0] == len(code) and decoded[3] == DESTINATION


@pytest.mark.parametrize(
    "code",
    [
        thunk(bytes.fromhex("4883c0e8")),  # wrong adjusted register
        thunk(bytes.fromhex("4883c718")),  # unsupported positive adjustment
        thunk(bytes.fromhex("4c8b1649037ae8")),  # wrong this register for vptr load
        thunk(bytes.fromhex("4c8b1749037a18")),  # positive vcall offset
        thunk(bytes.fromhex("4c8b1749037ae9")),  # unaligned vcall offset
        ADD + bytes.fromhex("ffe0"),  # indirect jump
        ADD + b"\xc3",  # return
        ADD + b"\xe9\0\0",  # truncated direct jump
        b"\x90" + thunk(),  # extra instruction before adjustment
    ],
)
def test_other_short_jump_sequences_are_not_thunks(code):
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    assert extents.thunk(decoder, code, ADDRESS) is None


def slots(monkeypatch, addresses):
    monkeypatch.setattr(
        extents,
        "vtable_slots",
        lambda target: [(a, "_ZTV1C", 0x6000, 0x6010 + i * 8) for i, a in enumerate(addresses)],
    )


def test_discovery_does_not_require_a_known_thunk_symbol_and_deduplicates(monkeypatch):
    slots(monkeypatch, [ADDRESS, ADDRESS])
    rows = extents.scan(image(thunk()), {})
    assert len(rows) == 1
    assert rows[0].symbol == ""
    assert rows[0].size == len(thunk())
    assert "slot 0x6010 (header 0x6000); jmp" in rows[0].evidence


def test_known_name_cannot_supply_missing_vtable_proof(monkeypatch):
    slots(monkeypatch, [])
    with pytest.raises(ValueError, match="lack vtable/shape proof"):
        extents.scan(image(thunk()), {ADDRESS: "_ZThn24_N1C1fEv"})


def test_loaded_target_bytes_must_match_the_image_pin(tmp_path, monkeypatch):
    original = image(thunk()).data
    path = tmp_path / "Game"
    path.write_bytes(original)
    pin = SimpleNamespace(path=path, size=len(original), sha256=hashlib.sha256(original).hexdigest())
    monkeypatch.setattr(builds, "load_builds", lambda: {"test": SimpleNamespace(images={"Game": pin})})
    assert extents.pinned_target("test").data == original
    changed = bytearray(original)
    changed[67] ^= 8  # change the adjustment while preserving a parseable ELF
    path.write_bytes(changed)
    with pytest.raises(ValueError, match="image pin mismatch"):
        extents.pinned_target("test")


def test_jump_must_target_an_fde_start(monkeypatch):
    slots(monkeypatch, [ADDRESS])
    target = image(thunk())
    target.fde_ranges = lambda: {(DESTINATION - 1, 16)}
    with pytest.raises(ValueError, match="outside FDE starts"):
        extents.scan(target, {})


def test_thunk_must_not_overlap_an_fde(monkeypatch):
    slots(monkeypatch, [ADDRESS])
    target = image(thunk())
    target.fde_ranges = lambda: {(ADDRESS - 1, 2), (DESTINATION, 16)}
    with pytest.raises(ValueError, match="overlaps an FDE"):
        extents.scan(target, {})


def test_two_decodable_entrypoints_must_not_overlap(monkeypatch):
    # The first add's negative imm32 also encodes a second add at +3, sharing its jump.
    code = thunk(bytes.fromhex("4881c74883c7e8"))
    slots(monkeypatch, [ADDRESS, ADDRESS + 3])
    with pytest.raises(ValueError, match="thunks .* overlap"):
        extents.scan(image(code), {})


def test_vtable_walk_uses_headers_and_stops_before_unrelated_data(monkeypatch):
    class Target:
        def section_at(self, address):
            return {"sh_addr": 0x6000, "sh_size": 0x100}

        def word(self, address):
            return {
                0x6010: ADDRESS,
                0x6018: 0,
                0x6020: DESTINATION,
                0x6028: 123,
                0x6030: ADDRESS + 1,  # pointer after non-slot must be ignored
                0x6050: ADDRESS + 2,
            }.get(address, 123)

        def is_code(self, address):
            return address >= ADDRESS

    info = rtti.ClassInfo("1C", 0x6080, "class", 0x6090, [(0x6000, 0), (0x6040, -24)])
    monkeypatch.setattr(rtti, "discover", lambda target: {"1C": info})
    found = list(extents.vtable_slots(Target()))
    assert [(a, slot) for a, _, _, slot in found] == [
        (ADDRESS, 0x6010),
        (DESTINATION, 0x6020),
        (ADDRESS + 2, 0x6050),
    ]


@pytest.mark.parametrize("change", ["size", "slot", "missing", "extra"])
def test_changed_extent_table_cannot_be_installed(tmp_path, monkeypatch, change):
    slots(monkeypatch, [ADDRESS])
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    monkeypatch.setattr(symbols, "load", lambda build: [])
    path = extents.path_for("test")
    path.parent.mkdir(parents=True)
    rows = extents.scan(image(thunk()), {})
    value = extents.render(rows)
    if change == "size":
        value = value.replace(f"\t{len(thunk())}\t", "\t1\t")
    elif change == "slot":
        value = value.replace("slot 0x6010", "slot 0x6030")
    elif change == "missing":
        value = value.splitlines()[0] + "\n"
    else:
        value += value.splitlines()[1] + "\n"
    path.write_text(value)
    target = image(thunk())
    monkeypatch.setattr(extents, "pinned_target", lambda build: target)
    with pytest.raises(ValueError, match="stale/invalid"):
        extents.load_target("test")
    assert not target.thunks


@pytest.mark.parametrize("change", [None, "bytes", "size", "reference"])
def test_thunk_credit_still_requires_full_bytes_size_and_relocations(monkeypatch, change):
    code = thunk()
    target = image(code)
    target.thunks = frozenset({(ADDRESS, len(code))})
    ours = ADD + b"\xe9" + bytes(4)
    known = {NAME: ADDRESS, "method": DESTINATION}
    if change == "bytes":
        ours = bytes.fromhex("4883c7e0") + ours[4:]
    if change == "reference":
        known["method"] += 1
    obj = Elf(
        elf_image(ours, size=len(code) + int(change == "size"), relocations=[(5, 2, -4, "method")]),
        "ET_REL",
    )
    result = text(compare_object(obj, target, known, {}))["functions"][0]
    assert result["exact"] == (change is None)
    assert result["extent"] == (None if change == "size" else "thunk")


def test_thunk_counts_once_preserves_denominator_and_records_source():
    inv, functions, evidence = sample()
    functions[0]["extent"] = "thunk"
    fn = evidence["units"][0]["sections"][0]["functions"][0]
    fn.update(extent="thunk")
    shared = copy.deepcopy(evidence["units"][0])
    shared["unit"] = "b.cpp"
    evidence["units"].append(shared)
    report = progress.make_report(inv, functions, evidence, {}, {})
    assert report["measures"]["total_code"] == "100"
    assert report["measures"]["matched_code"] == "20"
    assert report["measures"]["matched_functions"] == 1
    assert report["units"][0]["metadata"]["progress_categories"] == ["functions", "thunks"]
    categories = {c["id"]: c["measures"] for c in report["categories"]}
    assert categories["thunks"]["matched_functions"] == 1
    assert categories["thunks"]["matched_code"] == "20"
    fn["extent"] = "fde"
    with pytest.raises(ValueError, match="inventoried extent"):
        progress.make_report(inv, functions, evidence, {}, {})
