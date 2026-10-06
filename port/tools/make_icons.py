"""Render the port's icon (port/packaging/icons/harvest.svg) into the files packaging uses.

Writes, next to the SVG:

- harvest.png: 256x256, for the Linux .desktop entry;
- harvest.ico: 16 to 256 pixels, PNG-compressed entries, for the Windows resource;
- harvest.icns: 16 to 1024 pixels, PNG entries, for the macOS bundle.

The outputs are committed, so builds need neither this script nor rsvg-convert. Run it after
editing the SVG:

    uv run python port/tools/make_icons.py
"""

import shutil
import struct
import subprocess
from pathlib import Path

ICONS = Path(__file__).resolve().parent.parent / "packaging" / "icons"
SVG = ICONS / "harvest.svg"

ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]
# icns element types holding PNG data, by pixel size (the @2x types reuse the larger renders).
ICNS_TYPES = [
    (b"icp4", 16),
    (b"icp5", 32),
    (b"ic11", 32),
    (b"ic12", 64),
    (b"ic07", 128),
    (b"ic08", 256),
    (b"ic13", 256),
    (b"ic09", 512),
    (b"ic14", 512),
    (b"ic10", 1024),
]


def render(size: int) -> bytes:
    return subprocess.run(
        ["rsvg-convert", "-w", str(size), "-h", str(size), str(SVG)], check=True, capture_output=True
    ).stdout


def ico(pngs: dict[int, bytes]) -> bytes:
    header = struct.pack("<HHH", 0, 1, len(ICO_SIZES))
    offset = len(header) + 16 * len(ICO_SIZES)
    entries, images = b"", b""
    for size in ICO_SIZES:
        data = pngs[size]
        # width and height 0 mean 256
        entries += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
        images += data
        offset += len(data)
    return header + entries + images


def icns(pngs: dict[int, bytes]) -> bytes:
    body = b"".join(kind + struct.pack(">I", 8 + len(pngs[size])) + pngs[size] for kind, size in ICNS_TYPES)
    return b"icns" + struct.pack(">I", 8 + len(body)) + body


def main() -> None:
    if not shutil.which("rsvg-convert"):
        raise SystemExit("rsvg-convert is needed (librsvg)")
    sizes = sorted(set(ICO_SIZES) | {size for _, size in ICNS_TYPES})
    pngs = {size: render(size) for size in sizes}
    (ICONS / "harvest.png").write_bytes(pngs[256])
    (ICONS / "harvest.ico").write_bytes(ico(pngs))
    (ICONS / "harvest.icns").write_bytes(icns(pngs))
    for name in ("harvest.png", "harvest.ico", "harvest.icns"):
        print(ICONS / name)


if __name__ == "__main__":
    main()
