import os
import re
import struct
import subprocess
from pathlib import Path

import pytest

from hv import builds, extents, toolchain


@pytest.mark.originals
@pytest.mark.toolchain
@pytest.mark.parametrize("mutation", [False, True], ids=["native", "unordered-enters"])
def test_shuttle_race_loop_entry_matches_original(tmp_path: Path, mutation: bool) -> None:
    if os.environ.get("HARVEST_TEST_TOOLCHAIN") != "1":
        pytest.skip("set HARVEST_TEST_TOOLCHAIN=1 to compile with Docker")
    build = "1.18-linux-amd64"
    (image,) = builds.load_builds()[build].images.values()
    if builds.check_image(image) is not None:
        pytest.skip("pinned Linux image not present under orig/")

    # The original updateState's loop exit: ucomiss xmm0, xmm1; jbe 0x4b8afa.
    # Unordered sets CF and ZF, so NaN leaves the loop just like zero/negative deltas.
    original = extents.load_target(build).read(0x4B8D70, 9)
    assert original == bytes.fromhex("0f2ec10f8681fdffff")
    # Wrap those instructions as int(float): zero xmm1, preserve the compare/jbe,
    # relocate only its displacement to the false return, return true on fall-through.
    code = bytes.fromhex("0f57c9") + original[:5] + struct.pack("<i", 6)
    code += bytes.fromhex("b801000000c331c0c3")
    (tmp_path / "target-loop.h").write_text(
        "static const unsigned char targetCode[] = {" + ",".join(map(str, code)) + "};\n"
    )
    source = (builds.ROOT / "src/HarvestFull/harvest/states/CShuttleRaceState.cpp").read_text()
    body = source.split("int CShuttleRaceState::updateState(float time)", 1)[1].split(
        "CShuttleRaceState::~CShuttleRaceState()", 1
    )[0]
    (condition,) = re.findall(r"while \(([^\n]+)\)\n", body)
    if mutation:
        condition = "!(frameDelta <= 0)"
    (tmp_path / "source-loop.h").write_text(
        "static bool sourceEnters(float frameDelta) { return " + condition + "; }\n"
    )
    compiler = toolchain.Compiler(toolchain.load_flags(build), tmp_path)
    compiler.command(
        "g++", "-O2", "-Isrc", "-I/out", "tests/native/shuttle_race_loop_smoke.cpp",
        "-o", "/out/shuttle-race-loop-smoke",
    )  # fmt: skip
    if mutation:
        with pytest.raises(subprocess.CalledProcessError) as rejected:
            compiler.command("/out/shuttle-race-loop-smoke")
        assert "Shuttle-race loop check failed:" in rejected.value.stderr
    else:
        assert "Shuttle-race loop smoke passed:" in compiler.command("/out/shuttle-race-loop-smoke")
