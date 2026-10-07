"""Function extents outside the unwind table: the this-adjusting thunks.

GCC 4.4 gives every function an FDE except the thunks it emits directly as
assembly. A thunk's extent is still provable: it is a short fixed sequence that
adjusts `this` and ends in a direct jmp, and a vtable slot points at its start.
`generate` finds every data word that points into code outside the FDEs at such
a sequence, names the thunk from the Itanium mangling of its adjustment and its
jump target, and writes config/<build>/extents.tsv with the evidence for each
row: the vtable (or construction vtable) slot that points at it and its target.
The matcher and the progress inventory read that table beside .eh_frame.
"""

import bisect
import csv
import struct
from dataclasses import dataclass
from pathlib import Path

import capstone
from capstone import x86

from hv import builds
from hv.elf import Elf

HEADER = ["address", "size", "symbol", "evidence"]
MAX_INSTRUCTIONS = 5


@dataclass(frozen=True)
class Extent:
    address: int
    size: int
    symbol: str  # "" when the jump target has no name to derive it from
    evidence: str


def path_for(build: str) -> Path:
    return builds.ROOT / "config" / build / "extents.tsv"


def load(build: str) -> list[Extent]:
    path = path_for(build)
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return [
            Extent(int(r["address"], 16), int(r["size"]), r["symbol"], r["evidence"])
            for r in csv.DictReader(f, delimiter="\t")
        ]


def render(rows: list[Extent]) -> str:
    lines = ["\t".join(HEADER)]
    lines += [
        f"{e.address:#x}\t{e.size}\t{e.symbol}\t{e.evidence}" for e in sorted(rows, key=lambda e: e.address)
    ]
    return "\n".join(lines) + "\n"


def thunk(md: capstone.Cs, code: bytes, address: int) -> tuple[int, int, int | None, int] | None:
    """(size, fixed this offset, vcall offset or None, jump target) when the code
    at address is a this-adjusting thunk: `add rdi, imm`, optionally followed by
    `mov r10, [rdi]; add rdi, [r10 + disp]`, then a direct jmp. `this` is in rsi
    instead when the method returns through a hidden pointer in rdi."""
    fixed, vcall, loaded, this = 0, None, False, None
    for i in md.disasm(code, address):
        ops = i.operands
        register = i.reg_name(ops[0].reg) if ops and ops[0].type == x86.X86_OP_REG else None
        if register in ("rdi", "rsi") and this is None and i.mnemonic in ("add", "sub"):
            this = register
        if i.mnemonic in ("add", "sub") and len(ops) == 2 and register == this:
            if ops[1].type == x86.X86_OP_IMM and not loaded and vcall is None:
                fixed += ops[1].imm if i.mnemonic == "add" else -ops[1].imm
            elif ops[1].type == x86.X86_OP_MEM and loaded and vcall is None and i.mnemonic == "add":
                if i.reg_name(ops[1].mem.base) != "r10" or ops[1].mem.index:
                    return None
                vcall = ops[1].mem.disp
            else:
                return None
        elif (
            i.mnemonic == "mov"
            and not loaded
            and i.op_str in ("r10, qword ptr [rdi]", "r10, qword ptr [rsi]")
        ):
            base = i.op_str[-4:-1]
            if this not in (None, base):
                return None
            this, loaded = base, True
        elif i.mnemonic == "jmp" and ops and ops[0].type == x86.X86_OP_IMM:
            if loaded != (vcall is not None) or (fixed == 0 and vcall is None):
                return None
            return i.address + i.size - address, fixed, vcall, ops[0].imm
        else:
            return None
        if i.address - address >= 32:
            return None
    return None


def mangle(fixed: int, vcall: int | None, target: str) -> str:
    """The Itanium thunk name for a this adjustment into target (a _Z name)."""
    number = lambda n: f"n{-n}" if n < 0 else str(n)  # noqa: E731
    body = target.removeprefix("_Z")
    if vcall is None:
        return f"_ZTh{number(fixed)}_{body}"
    return f"_ZTv{number(fixed)}_{number(vcall)}_{body}"


def scan(target: Elf, names: dict[int, str]) -> list[Extent]:
    fdes = sorted(target.fde_ranges())
    starts = [a for a, _ in fdes]
    code = [(s["sh_addr"], s["sh_addr"] + s["sh_size"]) for s in target.sections if s["sh_flags"] & 4]
    named = sorted(names)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = True
    found: dict[int, Extent] = {}
    for section, data in target.mapped:
        if section["sh_flags"] & 4:
            continue
        for offset in range(0, len(data) - 7, 8):
            word = struct.unpack_from("<Q", data, offset)[0]
            span = next(((lo, hi) for lo, hi in code if lo <= word < hi), None)
            k = bisect.bisect_right(starts, word) - 1
            if span is None or word in found or (k >= 0 and word < fdes[k][0] + fdes[k][1]):
                continue
            shape = thunk(md, target.read(word, min(32, span[1] - word)), word)
            if shape is None:
                continue
            size, fixed, vcall, destination = shape
            k = bisect.bisect_right(starts, word + size - 1) - 1
            if k >= 0 and fdes[k][0] + fdes[k][1] > word:
                raise ValueError(f"thunk at {word:#x} overlaps an FDE")
            where = section["sh_addr"] + offset
            owner = named[bisect.bisect_right(named, where) - 1] if named and named[0] <= where else None
            slot = (
                f"{names[owner]}+{where - owner:#x}"
                if owner is not None and names[owner].startswith(("_ZTV", "_ZTC"))
                else f"{where:#x}"
            )
            jump = names.get(destination)
            symbol = mangle(fixed, vcall, jump) if jump and jump.startswith("_Z") else ""
            found[word] = Extent(word, size, symbol, f"slot {slot}; jmp {jump or hex(destination)}")
    rows = sorted(found.values(), key=lambda e: e.address)
    for a, b in zip(rows, rows[1:], strict=False):
        if a.address + a.size > b.address:
            raise ValueError(f"thunks at {a.address:#x} and {b.address:#x} overlap")
    return rows


def generate(build: str) -> list[Extent]:
    from hv import symbols

    (image,) = builds.load_builds()[build].images.values()
    if problem := builds.check_image(image):
        raise ValueError(f"{image.path}: {problem}")
    names = {s.address: s.name for s in symbols.load(build)}
    rows = scan(Elf.load(image.path, "ET_EXEC"), names)
    for row in rows:
        known = names.get(row.address)
        if known and row.symbol and known != row.symbol:
            raise ValueError(
                f"{row.address:#x}: derived name {row.symbol} disagrees with symbols.tsv {known}"
            )
    return rows


def load_target(build: str) -> Elf:
    """The build's pinned image, with its thunk extents attached beside the FDEs."""
    (image,) = builds.load_builds()[build].images.values()
    target = Elf.load(image.path, "ET_EXEC")
    target.thunks = frozenset((e.address, e.size) for e in load(build))
    return target
