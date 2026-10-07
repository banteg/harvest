import capstone
import pytest

from hv import builds, extents, symbols

BUILD = "1.18-linux-amd64"
ADDRESS = 0x1000


def decode(code: bytes):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = True
    return extents.thunk(md, code, ADDRESS)


def test_non_virtual_thunk_adjusts_this_then_jumps():
    # add rdi, -0x18; jmp +0x10
    assert decode(bytes.fromhex("4883c7e8") + bytes.fromhex("eb0e")) == (6, -0x18, None, ADDRESS + 6 + 0x0E)


def test_virtual_thunk_loads_the_vcall_offset():
    # mov r10, [rdi]; add rdi, [r10 - 0x18]; jmp +0x10
    code = bytes.fromhex("4c8b17") + bytes.fromhex("49037ae8") + bytes.fromhex("eb07")
    assert decode(code) == (9, 0, -0x18, ADDRESS + 9 + 7)


def test_this_moves_to_rsi_behind_a_hidden_return_pointer():
    # add rsi, -0x128; jmp +0x8
    assert decode(bytes.fromhex("4881c6d8feffff") + bytes.fromhex("eb08"))[:3] == (9, -0x128, None)


@pytest.mark.parametrize(
    "code",
    [
        bytes.fromhex("e9fb0f0000"),  # a bare jmp adjusts nothing
        bytes.fromhex("6a00e9f6ffffff"),  # a PLT stub: push 0; jmp
        bytes.fromhex("4883c7e8e800000000c3"),  # add rdi; call; ret is not a thunk
    ],
)
def test_other_code_is_not_a_thunk(code):
    assert decode(code) is None


def test_names_follow_the_itanium_thunk_mangling():
    target = "_ZN5daisy3gui11CGUICheckBoxD1Ev"
    assert extents.mangle(-24, None, target) == "_ZThn24_N5daisy3gui11CGUICheckBoxD1Ev"
    assert extents.mangle(0, -24, target) == "_ZTv0_n24_N5daisy3gui11CGUICheckBoxD1Ev"


def test_committed_extents_agree_with_symbols_and_the_inventory():
    # runs without originals, so CI catches an extents.tsv out of step with the other committed tables
    rows = extents.load(BUILD)
    assert rows and all(e.symbol for e in rows)
    named = {s.name: s.address for s in symbols.load(BUILD)}
    assert all(named.get(e.symbol) == e.address for e in rows)
    assert all(e.evidence.startswith(("slot _ZTV", "slot _ZTC")) for e in rows)
    inventory = (builds.ROOT / "reference" / BUILD / "progress" / "functions.tsv").read_text().splitlines()
    thunks = {
        (int(a, 16), int(n))
        for a, n, _, kind in (line.split("\t") for line in inventory[1:])
        if kind == "thunk"
    }
    assert thunks == {(e.address, e.size) for e in rows}


@pytest.mark.originals
def test_regenerating_reproduces_the_committed_extents():
    (image,) = builds.load_builds()[BUILD].images.values()
    if builds.check_image(image) is not None:
        pytest.skip("pinned Linux image not present under orig/")
    rows = extents.generate(BUILD)
    assert extents.render(rows) == extents.path_for(BUILD).read_text()
    named = {s.name: s.address for s in symbols.load(BUILD)}
    assert all(named.get(e.symbol) == e.address for e in rows if e.symbol)
