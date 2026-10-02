import os
import subprocess
from pathlib import Path
from typing import Literal, assert_never

import pytest

from hv import builds, toolchain


@pytest.mark.toolchain
@pytest.mark.parametrize(
    "mutation", [None, "alpha", "channel"], ids=["native", "missing-alpha", "wrong-channel"]
)
def test_color_converter_native_behavior_and_mutation_rejection(
    tmp_path: Path, mutation: Literal["alpha", "channel"] | None
) -> None:
    if os.environ.get("HARVEST_TEST_TOOLCHAIN") != "1":
        pytest.skip("set HARVEST_TEST_TOOLCHAIN=1 to compile with Docker")

    compiler = toolchain.Compiler(toolchain.load_flags("1.18-linux-amd64"), tmp_path)
    source = builds.ROOT / "src/daisy/video/Null/CColorConverter.cpp"
    overlay: tuple[Path, Path] | None = None
    match mutation:
        case None:
            pass
        case "alpha":
            header = builds.ROOT / "src/ox/video/ColorPacking.h"
            original = header.read_text()
            before = "return 0x8000 |"
            assert original.count(before) == 1
            replacement = tmp_path / "ColorPacking.h"
            _ = replacement.write_text(original.replace(before, "return"))
            overlay = (header, replacement)
        case "channel":
            original = source.read_text()
            before = "((color << inR) & 0xff000000) >> outR"
            assert original.count(before) == 1
            replacement = tmp_path / "mutated.cpp"
            _ = replacement.write_text(original.replace(before, "((color << inB) & 0xff000000) >> outR"))
            overlay = (source, replacement)
        case unreachable:
            assert_never(unreachable)
    command = compiler.compile_command(source, "color-converter")
    _ = compiler.command(*command, source_override=overlay)
    _ = compiler.command(
        "g++",
        "-O2",
        "-Isrc",
        "tests/native/color_converter_smoke.cpp",
        "/out/color-converter.o",
        "-o",
        "/out/color-converter-smoke",
    )
    if mutation is None:
        assert "Color-converter smoke passed:" in compiler.command("/out/color-converter-smoke")
    else:
        with pytest.raises(subprocess.CalledProcessError) as rejected:
            compiler.command("/out/color-converter-smoke")
        assert "Color-converter check failed:" in rejected.value.stderr
