"""Run the smoke scripts and build recipes against disposable data and stubbed external commands."""

import getpass
import os
import shutil
import subprocess
from pathlib import Path

import pytest

from hv import builds


@pytest.fixture
def workspace(tmp_path, monkeypatch):
    root = tmp_path / "player's checkout"
    shutil.copytree(builds.ROOT / "port/smoke", root / "port/smoke")
    shutil.copy2(builds.ROOT / "justfile", root)
    (root / "orig/1.18-linux-amd64/harvestClientData").mkdir(parents=True)
    (root / "bin").mkdir()
    monkeypatch.setenv("PATH", str(root / "bin") + os.pathsep + os.environ["PATH"])
    monkeypatch.delenv("HARVEST_DATA", raising=False)
    return root


def script(path, commands):
    path.write_text("#!/bin/sh\n" + commands + "\n")
    path.chmod(0o755)


def run(root, *args):
    return subprocess.run(args, cwd=root, text=True, capture_output=True)


def just():
    if binary := shutil.which("just"):
        return binary
    pytest.skip("just is not installed")


def test_windows_smoke_preserves_existing_wine_save(workspace, tmp_path, monkeypatch):
    prefix = tmp_path / "Wine for playing"
    save = prefix / "drive_c/users" / getpass.getuser() / "AppData/Roaming/Harvest/profiles/old.hsg"
    save.parent.mkdir(parents=True)
    save.write_bytes(b"player's save")
    (prefix / "system.reg").touch()
    monkeypatch.setenv("WINEPREFIX", str(prefix))
    wine = workspace / "bin/wine"
    monkeypatch.setenv("WINE", str(wine))
    script(wine, 'echo "$WINEPREFIX" >> prefixes; [ "$1" = wineboot ]')
    script(workspace / "bin/zig", "exit 0")
    script(workspace / "bin/timeout", 'shift; exec "$@"')

    result = run(workspace, "bash", "port/smoke/smoke-windows.sh")
    assert result.returncode != 0  # Simulated failure in the first Windows test.
    boot, game = (workspace / "prefixes").read_text().splitlines()
    assert boot == game and Path(boot) != prefix and not Path(boot).exists()
    assert save.read_bytes() == b"player's save"


@pytest.mark.parametrize("failure", ["zig", "docker"])
@pytest.mark.parametrize("args", [[], ["--matrix"]])
def test_linux_smoke_refuses_stale_builds(workspace, failure, args):
    script(workspace / "bin/zig", f'echo attempt >> "$PWD/../attempts"; exit {int(failure == "zig")}')
    script(
        workspace / "bin/docker",
        f'case "$1" in info) echo x86_64;; build) exit {int(failure == "docker")};; '
        "run) touch ran-stale-build;; esac",
    )
    result = run(workspace, "bash", "port/smoke/smoke-linux.sh", *args)
    assert result.returncode != 0
    assert not (workspace / "ran-stale-build").exists()
    assert len((workspace / "attempts").read_text().splitlines()) == (4 if args else 1)


def test_linux_smoke_rejects_paths_in_configuration(workspace):
    script(workspace / "bin/zig", "exit 0")
    script(workspace / "bin/docker", "exit 0")
    keep = workspace / "build/smoke/important.log"
    keep.parent.mkdir(parents=True)
    keep.write_text("previous result")
    result = run(workspace, "bash", "port/smoke/smoke-linux.sh", "--gl", "bad/../..")
    assert result.returncode == 2
    assert keep.read_text() == "previous result"


def test_linux_smoke_requires_the_loaded_save(workspace):
    profiles = workspace / "profiles"
    profiles.mkdir()
    save = profiles / "saveGame00.hsg"
    log = workspace / "game.log"
    check = ["sh", "port/smoke/check_save.sh", profiles, log]
    log.write_text("")
    save.write_bytes(b"save")
    assert run(workspace, *check).returncode != 0  # Save exists, stayed in menu or started a new game.
    log.write_text("Loaded save game: $HARVEST_USERDATA$/profiles/saveGame01.hsg\n")
    assert run(workspace, *check).returncode != 0
    log.write_text("Loaded save game: $HARVEST_USERDATA$/profiles/saveGame00.hsg\n")
    assert run(workspace, *check).returncode == 0
    original = workspace / "original"
    original.mkdir()
    (original / save.name).write_bytes(b"other save")
    assert run(workspace, *check, original).returncode != 0
    (original / save.name).write_bytes(save.read_bytes())
    assert run(workspace, *check, original).returncode == 0
    save.write_bytes(b"")
    assert run(workspace, *check).returncode != 0


@pytest.mark.parametrize("recipe", ["run", "play"])
def test_just_preserves_game_arguments(workspace, recipe):
    script(workspace / "bin/zig", "exit 0")
    game = workspace / "port/zig-out/bin/harvest"
    game.parent.mkdir(parents=True)
    script(game, 'printf "%s\\n" "$@"')
    args = ["--data", "/Games/Harvest Massive Encounter", "--input-script", "player's script"]
    result = run(workspace, just(), recipe, *args)
    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == ["--data", str(workspace / "orig/1.18-linux-amd64"), *args]


def test_toolchain_manifest_replaced_only_on_success(workspace):
    manifest = workspace / "toolchain/manifest.tsv"
    manifest.parent.mkdir()
    manifest.write_text("pinned\n")
    docker = workspace / "bin/docker"
    script(docker, "echo partial; exit 1")
    assert run(workspace, just(), "toolchain-manifest").returncode != 0
    assert manifest.read_text() == "pinned\n"
    script(docker, "exit 0")
    assert run(workspace, just(), "toolchain-manifest").returncode != 0
    assert manifest.read_text() == "pinned\n"
    script(docker, "echo complete")
    assert run(workspace, just(), "toolchain-manifest").returncode == 0
    assert manifest.read_text() == "complete\n"
    assert list(manifest.parent.iterdir()) == [manifest]
