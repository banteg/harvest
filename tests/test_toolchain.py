"""Build-recipe compatibility across Docker and Podman's security configurations."""

import json
import os
import shutil
import subprocess
import sys

import pytest

from hv import builds


@pytest.mark.parametrize(
    ("security", "status", "flags"),
    [
        ("", 1, []),  # Docker rejects Podman's info fields.
        ("false false", 0, []),
        ("true false", 0, ["--cap-add=SYS_ADMIN"]),
        ("false true", 0, ["--security-opt=label=disable"]),
        ("true true", 0, ["--cap-add=SYS_ADMIN", "--security-opt=label=disable"]),
    ],
)
def test_toolchain_build_selects_runtime_security_flags(tmp_path, monkeypatch, security, status, flags):
    just = shutil.which("just")
    if just is None:
        pytest.skip("just is not installed")
    docker = tmp_path / "docker"
    docker.write_text(
        f"#!{sys.executable}\n"
        "import json, os, sys\n"
        "if sys.argv[1] == 'info':\n"
        "    print(os.environ['HARVEST_TEST_SECURITY'])\n"
        "    sys.exit(int(os.environ['HARVEST_TEST_INFO_STATUS']))\n"
        "with open(os.environ['HARVEST_TEST_BUILD_ARGS'], 'w') as f:\n"
        "    json.dump(sys.argv[1:], f)\n"
    )
    docker.chmod(0o755)
    output = tmp_path / "args.json"
    monkeypatch.setenv("PATH", f"{tmp_path}{os.pathsep}{os.environ['PATH']}")
    monkeypatch.setenv("HARVEST_TEST_SECURITY", security)
    monkeypatch.setenv("HARVEST_TEST_INFO_STATUS", str(status))
    monkeypatch.setenv("HARVEST_TEST_BUILD_ARGS", str(output))
    subprocess.run(
        [just, "--justfile", str(builds.ROOT / "justfile"), "toolchain"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert json.loads(output.read_text()) == [
        "build",
        *flags,
        "--platform",
        "linux/amd64",
        "-t",
        "harvest-toolchain",
        "toolchain",
    ]


def test_compile_each_runs_concurrently_but_yields_in_source_order(monkeypatch):
    from pathlib import Path
    from threading import Barrier, Event

    from hv import toolchain

    compiler = toolchain.Compiler.__new__(toolchain.Compiler)
    together = Barrier(3, timeout=5)
    completed = [Event() for _ in range(3)]

    def compile(source, name):
        index = int(name)
        together.wait()
        if index < 2:
            assert completed[index + 1].wait(5)
        completed[index].set()
        return Path(f"{name}.o"), {"source": str(source)}

    monkeypatch.setattr(compiler, "compile", compile)
    items = [(Path(f"{i}.cpp"), str(i)) for i in range(3)]
    assert list(compiler.compile_each(items, jobs=3)) == [
        (Path(f"{i}.o"), {"source": f"{i}.cpp"}) for i in range(3)
    ]


def test_compile_each_preserves_prior_result_and_compiler_error(monkeypatch):
    from pathlib import Path
    from threading import Event

    from hv import toolchain

    compiler = toolchain.Compiler.__new__(toolchain.Compiler)
    failed = Event()
    error = subprocess.CalledProcessError(7, ["g++", "bad.cpp"], stderr="compile failed")

    def compile(source, name):
        if name == "bad":
            failed.set()
            raise error
        assert failed.wait(5)
        return Path("good.o"), {"source": str(source)}

    monkeypatch.setattr(compiler, "compile", compile)
    results = compiler.compile_each([(Path("good.cpp"), "good"), (Path("bad.cpp"), "bad")], jobs=2)
    assert next(results) == (Path("good.o"), {"source": "good.cpp"})
    with pytest.raises(subprocess.CalledProcessError) as caught:
        next(results)
    assert caught.value is error
    assert caught.value.stderr == "compile failed"
