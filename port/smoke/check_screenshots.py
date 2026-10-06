"""Checks smoke-test screenshots: each must exist and must not be blank (one flat colour).

uv run --no-project --with pillow python port/smoke/check_screenshots.py <image>...
"""

import sys
from pathlib import Path

from PIL import Image, ImageStat

# a blank frame (a cleared screen, a stalled renderer) is one colour up to JPEG noise
MIN_DEVIATION = 6.0
MAX_DOMINANT_SHARE = 0.97


def problem(path: Path) -> str | None:
    if not path.is_file():
        return "missing"
    image = Image.open(path).convert("RGB")
    deviation = max(ImageStat.Stat(image).stddev)
    if deviation < MIN_DEVIATION:
        return f"blank (standard deviation {deviation:.1f})"
    # reduced to 256 colours, so JPEG noise around one colour counts as that colour
    small = image.resize((256, 192)).quantize(colors=256, method=Image.Quantize.FASTOCTREE)
    count, _ = max(small.getcolors(256 * 256))
    share = count / (256 * 192)
    if share > MAX_DOMINANT_SHARE:
        return f"blank ({share:.0%} one colour)"
    return None


def main() -> int:
    failures = 0
    for name in sys.argv[1:]:
        path = Path(name)
        result = problem(path)
        print(f"{'FAIL' if result else 'ok  '}  {path.name}{': ' + result if result else ''}")
        failures += result is not None
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
