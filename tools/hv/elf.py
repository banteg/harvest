"""Small, fail-closed ELF64/x86-64 view of target images and compiled objects."""

import io
import struct
from pathlib import Path

from elftools.dwarf.callframe import FDE
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection


class Elf:
    def __init__(self, data: bytes, kind: str):
        self.data = data
        self.elf = ELFFile(io.BytesIO(data))
        if (
            self.elf.elfclass != 64
            or not self.elf.little_endian
            or self.elf["e_machine"] != "EM_X86_64"
            or self.elf["e_type"] != kind
        ):
            raise ValueError(f"expected little-endian x86-64 {kind}")
        # pyelftools re-parses a section header, and re-reads its bytes, on every access; the
        # matcher looks sections, symbols and relocations up repeatedly, so parse each once
        self.sections = list(self.elf.iter_sections())
        self.mapped = [
            (section, section.data())
            for section in self.sections
            if section["sh_flags"] & 2 and section["sh_type"] != "SHT_NOBITS"
        ]
        self._symbols = {}
        self._relocations = {}
        self._cache = {}
        # (address, size) of functions without an FDE, from config/<build>/extents.tsv
        self.thunks: frozenset[tuple[int, int]] = frozenset()

    @classmethod
    def load(cls, path: Path, kind: str):
        return cls(path.read_bytes(), kind)

    def read(self, address: int, size: int) -> bytes:
        if size <= 0:
            raise ValueError("range must be nonempty")
        for section, data in self.mapped:
            offset = address - section["sh_addr"]
            if 0 <= offset and offset + size <= section["sh_size"]:
                data = data[offset : offset + size]
                if len(data) == size:
                    return data
        raise ValueError(f"unmapped or truncated range: {address:#x}+{size:#x}")

    def word(self, address: int) -> int:
        return struct.unpack("<Q", self.read(address, 8))[0]

    def symbols(self, table_index: int) -> list:
        """The symbols of a symbol table section, by symbol index."""
        if table_index not in self._symbols:
            self._symbols[table_index] = list(self.sections[table_index].iter_symbols())
        return self._symbols[table_index]

    def symtab(self) -> list:
        for index, section in enumerate(self.sections):
            if section.name == ".symtab":
                return self.symbols(index)
        return []

    def relocations(self, section_index: int) -> tuple:
        """(relocation, symbol) pairs that apply to a section."""
        if section_index not in self._relocations:
            pairs = []
            for section in self.sections:
                if isinstance(section, RelocationSection) and section["sh_info"] == section_index:
                    if not section.is_RELA():
                        raise ValueError("REL relocations are not supported")
                    table = self.symbols(section["sh_link"])
                    pairs.extend((r, table[r["r_info_sym"]]) for r in section.iter_relocations())
            self._relocations[section_index] = tuple(pairs)
        return self._relocations[section_index]

    def section_at(self, address: int):
        for section in self.sections:
            if (
                section["sh_flags"] & 2
                and section["sh_addr"] <= address < section["sh_addr"] + section["sh_size"]
            ):
                return section
        return None

    def is_code(self, address: int) -> bool:
        section = self.section_at(address)
        return section is not None and bool(section["sh_flags"] & 4)

    def cstring(self, address: int) -> bytes:
        section = self.section_at(address)
        if section is None or section["sh_type"] == "SHT_NOBITS":
            raise ValueError(f"no string data at {address:#x}")
        data = next(data for mapped, data in self.mapped if mapped is section)
        start = address - section["sh_addr"]
        end = data.find(b"\0", start)
        if end < 0:
            raise ValueError(f"unterminated string at {address:#x}")
        return data[start:end]

    def copy_symbols(self) -> dict[str, int]:
        """Library data copied into the executable by R_X86_64_COPY, by symbol name."""
        if "copies" not in self._cache:
            self._cache["copies"] = self._copy_symbols()
        return self._cache["copies"]

    def _copy_symbols(self) -> dict[str, int]:
        rela = self.elf.get_section_by_name(".rela.dyn")
        if not isinstance(rela, RelocationSection):
            return {}
        table = self.elf.get_section(rela["sh_link"])
        return {
            table.get_symbol(r["r_info_sym"]).name: r["r_offset"]
            for r in rela.iter_relocations()
            if r["r_info_type"] == 5  # R_X86_64_COPY
        }

    def fdes(self) -> list[FDE]:
        if not hasattr(self, "_fdes"):
            dwarf = self.elf.get_dwarf_info()
            entries = dwarf.EH_CFI_entries() if dwarf.has_EH_CFI() else []
            self._fdes = [entry for entry in entries if isinstance(entry, FDE)]
        return self._fdes

    def fde_ranges(self) -> frozenset[tuple[int, int]]:
        if "fde_ranges" not in self._cache:
            self._cache["fde_ranges"] = frozenset(
                (entry["initial_location"], entry["address_range"]) for entry in self.fdes()
            )
        return self._cache["fde_ranges"]

    def function_extents(self) -> dict[int, tuple[int, str]]:
        """Each function's start -> (size, source): its FDE, or its thunk extent."""
        extents = {address: (size, "fde") for address, size in self.fde_ranges()}
        for address, size in self.thunks:
            if address in extents:
                raise ValueError(f"thunk extent at {address:#x} duplicates an FDE")
            extents[address] = (size, "thunk")
        return extents

    def fde_lsdas(self) -> dict[int, int]:
        """The exception table (LSDA) of each function that has one, by function address."""
        return {e["initial_location"]: e.lsda_pointer for e in self.fdes() if e.lsda_pointer is not None}

    def plt_symbols(self) -> dict[str, int]:
        """Resolve each legacy x86-64 PLT entry through its GOT relocation."""
        if "plt" not in self._cache:
            self._cache["plt"] = self._plt_symbols()
        return self._cache["plt"]

    def _plt_symbols(self) -> dict[str, int]:
        plt = self.elf.get_section_by_name(".plt")
        rela = self.elf.get_section_by_name(".rela.plt")
        if plt is None or not isinstance(rela, RelocationSection):
            return {}
        table = self.elf.get_section(rela["sh_link"])
        got = {}
        for relocation in rela.iter_relocations():
            if relocation["r_info_type"] != 7:  # R_X86_64_JUMP_SLOT
                raise ValueError("unsupported PLT relocation")
            got[relocation["r_offset"]] = table.get_symbol(relocation["r_info_sym"]).name
        result = {}
        data = plt.data()
        for offset in range(16, len(data) - 15, 16):
            # Verify the indirect jump, rather than inferring a PLT entry from order.
            if data[offset : offset + 2] != b"\xff\x25":
                raise ValueError("unsupported PLT encoding")
            address = plt["sh_addr"] + offset
            slot = address + 6 + struct.unpack_from("<i", data, offset + 2)[0]
            if slot not in got or got[slot] in result:
                raise ValueError("ambiguous PLT/GOT mapping")
            result[got[slot]] = address
        return result
