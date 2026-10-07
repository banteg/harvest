"""Function extents outside the unwind table: the this-adjusting thunks.

A thunk's extent is provable without an FDE: it is a short fixed sequence that
adjusts `this` and ends in a direct jmp, and a vtable slot points at its start.
`generate` walks RTTI-proven vtable slots pointing into code outside the FDEs at such
a sequence, names the thunk from the Itanium mangling of its adjustment and its
jump target, and writes config/<build>/extents.tsv with the evidence for each
row: the vtable (or construction vtable) slot that points at it and its target.
The matcher and the progress inventory read that table beside .eh_frame.
"""

import bisect
import csv
import hashlib
from dataclasses import dataclass
from pathlib import Path

import capstone
from capstone import x86

from hv import builds, rtti
from hv.elf import Elf

HEADER = ["address", "size", "symbol", "evidence"]


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
        reader = csv.DictReader(f, delimiter="\t")
        if reader.fieldnames != HEADER:
            raise ValueError("invalid thunk extent fields")
        rows = [Extent(int(r["address"], 16), int(r["size"]), r["symbol"], r["evidence"]) for r in reader]
    end = 0
    for row in rows:
        if row.size <= 0 or row.address < end or not row.evidence.startswith("slot "):
            raise ValueError("invalid/overlapping thunk extent metadata")
        end = row.address + row.size
    return rows


def render(rows: list[Extent]) -> str:
    lines = ["\t".join(HEADER)]
    lines += [
        f"{e.address:#x}\t{e.size}\t{e.symbol}\t{e.evidence}" for e in sorted(rows, key=lambda e: e.address)
    ]
    return "\n".join(lines) + "\n"


def vtable_slots(target: Elf):
    """Walk function slots after RTTI-proven headers, stopping at the next header/data object.

    Secondary tables have their own offset-to-top/typeinfo header. Virtual-base offsets before a
    header are not slots. A null slot is allowed (abstract destructors); other non-code words end it.
    """
    classes = rtti.discover(target)
    boundaries = sorted(
        {a for c in classes.values() for a, _ in c.vtables}
        | {c.typeinfo for c in classes.values()}
        | {c.type_name for c in classes.values()}
    )
    for c in sorted(classes.values(), key=lambda c: c.name):
        for header, _ in sorted(c.vtables):
            section = target.section_at(header)
            end = section["sh_addr"] + section["sh_size"]
            following = bisect.bisect_right(boundaries, header)
            if following < len(boundaries):
                end = min(end, boundaries[following])
            for slot in range(header + 16, end - 7, 8):
                address = target.word(slot)
                if not address:
                    continue
                if not target.is_code(address):
                    break
                yield address, f"_ZTV{c.name}", header, slot


def thunk(md: capstone.Cs, code: bytes, address: int) -> tuple[int, int, int | None, int] | None:
    """Decode only the observed GCC add-this/direct-jump shapes, with exact operand roles.

    A struct-return method adjusts rsi behind sret in rdi. Virtual thunks load r10 from
    [rdi], add the negative aligned vcall offset to rdi, and jump directly to the method.
    """
    insns = []
    for insn in md.disasm(code, address):
        insns.append(insn)
        if insn.mnemonic == "jmp" or len(insns) == 3:
            break
    if len(insns) not in (2, 3):
        return None
    jump = insns[-1]
    if jump.mnemonic != "jmp" or len(jump.operands) != 1 or jump.operands[0].type != x86.X86_OP_IMM:
        return None

    def reg(op, register):
        return op.type == x86.X86_OP_REG and op.reg == register and op.size == 8

    def mem(op, base):
        return (
            op.type == x86.X86_OP_MEM
            and op.size == 8
            and op.mem.base == base
            and not op.mem.index
            and not op.mem.segment
        )

    add = insns[-2]
    if add.mnemonic != "add" or len(add.operands) != 2:
        return None
    fixed, vcall = 0, None
    if len(insns) == 2:
        if (
            not any(reg(add.operands[0], r) for r in (x86.X86_REG_RDI, x86.X86_REG_RSI))
            or add.operands[1].type != x86.X86_OP_IMM
            or add.operands[1].imm >= 0
        ):
            return None
        fixed = add.operands[1].imm
    else:
        mov = insns[0]
        if (
            not reg(add.operands[0], x86.X86_REG_RDI)
            or mov.mnemonic != "mov"
            or len(mov.operands) != 2
            or not reg(mov.operands[0], x86.X86_REG_R10)
            or not mem(mov.operands[1], x86.X86_REG_RDI)
            or mov.operands[1].mem.disp != 0
            or not mem(add.operands[1], x86.X86_REG_R10)
            or add.operands[1].mem.disp >= 0
            or add.operands[1].mem.disp % 8
        ):
            return None
        vcall = add.operands[1].mem.disp
    return jump.address + jump.size - address, fixed, vcall, jump.operands[0].imm


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
    start_set = set(starts)
    named = sorted(names)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = True
    found: dict[int, Extent] = {}
    for word, _, header, where in sorted(vtable_slots(target), key=lambda slot: slot[3]):
        if word in found or word in start_set:
            continue
        section = target.section_at(word)
        size = min(32, section["sh_addr"] + section["sh_size"] - word)
        shape = thunk(md, target.read(word, size), word)
        if shape is None:
            continue
        size, fixed, vcall, destination = shape
        if destination not in start_set:
            raise ValueError(f"thunk at {word:#x} jumps outside FDE starts: {destination:#x}")
        k = bisect.bisect_right(starts, word + size - 1) - 1
        if k >= 0 and fdes[k][0] + fdes[k][1] > word:
            raise ValueError(f"thunk at {word:#x} overlaps an FDE")
        owner = named[bisect.bisect_right(named, where) - 1] if named and named[0] <= where else None
        slot = (
            f"{names[owner]}+{where - owner:#x}"
            if owner is not None and names[owner].startswith(("_ZTV", "_ZTC"))
            else f"{where:#x} (header {header:#x})"
        )
        jump = names.get(destination)
        symbol = mangle(fixed, vcall, jump) if jump and jump.startswith("_Z") else ""
        found[word] = Extent(word, size, symbol, f"slot {slot}; jmp {jump or hex(destination)}")
    rows = sorted(found.values(), key=lambda e: e.address)
    for a, b in zip(rows, rows[1:], strict=False):
        if a.address + a.size > b.address:
            raise ValueError(f"thunks at {a.address:#x} and {b.address:#x} overlap")
    if missing := {a for a, name in names.items() if name.startswith(("_ZTh", "_ZTv"))} - found.keys():
        raise ValueError(
            f"named thunks lack vtable/shape proof: {', '.join(hex(a) for a in sorted(missing))}"
        )
    return rows


def pinned_target(build: str) -> Elf:
    (image,) = builds.load_builds()[build].images.values()
    target = Elf.load(image.path, "ET_EXEC")
    if len(target.data) != image.size or hashlib.sha256(target.data).hexdigest() != image.sha256:
        raise ValueError(f"{image.path}: image pin mismatch")
    return target


def target_rows(target: Elf, build: str) -> list[Extent]:
    from hv import symbols

    names = {s.address: s.name for s in symbols.load(build)}
    rows = scan(target, names)
    for row in rows:
        known = names.get(row.address)
        if known and row.symbol and known != row.symbol:
            raise ValueError(
                f"{row.address:#x}: derived name {row.symbol} disagrees with symbols.tsv {known}"
            )
    return rows


def generate(build: str) -> list[Extent]:
    return target_rows(pinned_target(build), build)


def load_target(build: str) -> Elf:
    """Attach only thunk extents reproduced from the pinned original and recorded slot evidence."""
    target = pinned_target(build)
    rows = target_rows(target, build)
    if path_for(build).read_text() != render(rows):
        raise ValueError("stale/invalid thunk extents; run hv extents")
    target.thunks = frozenset((e.address, e.size) for e in rows)
    return target
