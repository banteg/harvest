import os
import subprocess
from pathlib import Path

import pytest

from hv import builds, extents, rtti, toolchain


@pytest.mark.originals
def test_fade_affector_original_base_type():
    build = "1.18-linux-amd64"
    (image,) = builds.load_builds()[build].images.values()
    if builds.check_image(image) is not None:
        pytest.skip("pinned Linux image not present under orig/")
    target = extents.load_target(build)
    classes = rtti.discover(target)
    affector = classes["N5daisy5scene24CParticleFadeOutAffectorE"]
    assert affector.kind == "si"
    assert target.word(affector.typeinfo + 16) == classes["N2ox5scene17IParticleAffectorE"].typeinfo


@pytest.mark.toolchain
@pytest.mark.parametrize("mutation", [False, True], ids=["native", "wrong-endpoint"])
def test_fade_affector_native_interface_and_behavior(tmp_path: Path, mutation: bool):
    if os.environ.get("HARVEST_TEST_TOOLCHAIN") != "1":
        pytest.skip("set HARVEST_TEST_TOOLCHAIN=1 to compile with Docker")
    compiler = toolchain.Compiler(toolchain.load_flags("1.18-linux-amd64"), tmp_path)
    source = builds.ROOT / "src/daisy/scene/CParticleFadeOutAffector.cpp"
    replacement = None
    if mutation:
        text = source.read_text()
        before = "particlearray[i].endTime - now < FadeOutTime"
        assert text.count(before) == 1
        replacement = tmp_path / "mutated.cpp"
        replacement.write_text(text.replace(before, "particlearray[i].endTime - now <= FadeOutTime"))
    obj, _ = compiler.compile(source, "affector", source_override=replacement)
    compiler.command(
        "g++", "-O2", "-Isrc", "tests/native/particle_affector_smoke.cpp",
        compiler.container_path(obj), "-o", "/out/particle-affector-smoke",
    )  # fmt: skip
    if mutation:
        with pytest.raises(subprocess.CalledProcessError) as rejected:
            compiler.command("/out/particle-affector-smoke")
        assert "Particle affector check failed:" in rejected.value.stderr
    else:
        assert "Particle affector smoke passed:" in compiler.command("/out/particle-affector-smoke")
