"""Compile recovered sources in the pinned toolchain container."""

import json
import os
import shlex
import subprocess
import uuid
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from hv import builds

MANIFEST = builds.ROOT / "toolchain" / "manifest.tsv"


def run(*args: str) -> str:
    return subprocess.run(args, cwd=builds.ROOT, check=True, text=True, capture_output=True).stdout


def load_flags(build: str) -> dict:
    return json.loads((builds.ROOT / "config" / build / "flags.json").read_text())


class Compiler:
    """Runs the container by image ID, offline, with the repository read-only."""

    def __init__(self, flags: dict, out: Path):
        self.flags = flags
        self.out = out.resolve()
        self.out.mkdir(parents=True, exist_ok=True)
        self.image_id = run("docker", "image", "inspect", flags["image"], "--format", "{{.Id}}").strip()
        self.version = self.command(flags["compiler"], "--version").splitlines()[0]
        if self.version != flags["version"]:
            raise ValueError(f"compiler version mismatch: {self.version}")
        manifest = self.command("dpkg-query", "-W", "-f", "${Package}\t${Version}\n")
        if manifest != MANIFEST.read_text():
            raise ValueError("installed packages differ from toolchain/manifest.tsv")
        self.manifest_sha256 = builds.sha256_file(MANIFEST)

    def command(self, *args: str, source_override: tuple[Path, Path] | None = None) -> str:
        # SELinux hosts label the bind mounts as host files that container_t cannot read; the
        # container is already offline with a read-only repository, so skip labeling, not relabel
        mounts = []
        if source_override is not None:
            source, replacement = (p.resolve() for p in source_override)
            if not source.is_relative_to(builds.ROOT) or not replacement.is_relative_to(self.out):
                raise ValueError("source override must map a repository file to a compiler output file")
            mounts = ["-v", f"{replacement}:/work/{source.relative_to(builds.ROOT)}:ro"]
        return run(
            "docker", "run", "--rm", "--network=none", "--platform", "linux/amd64",
            "--security-opt=label=disable",
            "-v", f"{builds.ROOT}:/work:ro", "-v", f"{self.out}:/out", "-w", "/work",
            *mounts, self.image_id, *args,
        )  # fmt: skip

    def container_path(self, path: Path) -> str:
        path = path.resolve()
        if path.is_relative_to(self.out):
            return str(Path("/out") / path.relative_to(self.out))
        return str(path.relative_to(builds.ROOT))

    def host_path(self, path: str) -> Path | None:
        """Host path of a container path, or None for toolchain files covered by the manifest."""
        if path.startswith("/out/"):
            return self.out / path.removeprefix("/out/")
        if path.startswith("/"):
            return None
        return builds.ROOT / path

    def compile_command(self, source: Path, name: str) -> list[str]:
        return [
            self.flags["compiler"],
            *self.flags["flags"],
            "-MD",
            "-MF",
            f"/out/{name}.d",
            "-c",
            self.container_path(source),
            "-o",
            f"/out/{name}.o",
        ]

    def compile(self, source: Path, name: str, *, source_override: Path | None = None) -> tuple[Path, dict]:
        (self.out / f"{name}.o").parent.mkdir(parents=True, exist_ok=True)
        command = self.compile_command(source, name)
        override = (source, source_override) if source_override is not None else None
        self.command(*command, source_override=override)
        return self.out / f"{name}.o", self.metadata(source, name, command, source_override)

    def compile_each(self, items: list[tuple[Path, str]], jobs: int | None = None):
        """Compile several sources, each as compile() does and in its own container, up to `jobs`
        at a time (8, or the processor count if smaller), and yield (object, metadata) in the
        order given. A failed compilation is raised when its item is reached, as a loop over
        compile() would raise it, and the compilations not yet started are dropped."""
        pool = ThreadPoolExecutor(jobs or min(8, os.cpu_count() or 1))
        try:
            yield from pool.map(lambda item: self.compile(*item), items)
        finally:
            pool.shutdown(cancel_futures=True)

    def compile_many(self, source: Path, items: list[tuple[str, Path]], lanes: int | None = None) -> list:
        """Compile replacements of one repository source in a single container. Items run in parallel
        lanes; each lane copies the source and include roots into a private directory and puts its
        replacement at the source's own relative path, so command lines, __FILE__ and depfile spellings
        match a canonical compile (nothing records the working directory without -g). Returns, per
        item, (object, metadata) or the CalledProcessError of a failed compilation."""
        source = source.resolve()
        relative = source.relative_to(builds.ROOT)
        roots = {relative.parts[0]} | {
            Path(flag[2:]).parts[0]
            for flag in self.flags["flags"]
            if flag.startswith("-I") and not Path(flag[2:]).is_absolute()
        }
        lanes = max(1, min(len(items), lanes or os.cpu_count() or 1))
        work = [[] for _ in range(lanes)]
        commands = []
        for index, (name, replacement) in enumerate(items):
            replacement = replacement.resolve()
            if not replacement.is_relative_to(self.out):
                raise ValueError("batch replacements must be compiler output files")
            (self.out / f"{name}.o").parent.mkdir(parents=True, exist_ok=True)
            command = self.compile_command(source, name)
            commands.append(command)
            work[index % lanes].append(
                f"if cp {shlex.quote(self.container_path(replacement))} {shlex.quote(str(relative))}; "
                f"then {shlex.join(command)} 2> /out/{name}.err; echo $? > /out/{name}.status; "
                f"else echo 'cp failed' > /out/{name}.err; echo 125 > /out/{name}.status; fi"
            )
        lines = ["set -u"]
        for lane, steps in enumerate(work):
            copies = [f"cp -a /repo/{shlex.quote(r)} {shlex.quote(r)}" for r in sorted(roots)]
            lines.append(f"(mkdir /work/{lane} && cd /work/{lane} && " + " && ".join(copies) + "\n")
            lines.extend(steps)
            lines.append(") &")
        lines.append("wait")
        script = self.out / f"batch-{uuid.uuid4().hex}.sh"
        script.write_text("\n".join(lines) + "\n")
        try:
            run(
                "docker", "run", "--rm", "--network=none", "--platform", "linux/amd64",
                "--security-opt=label=disable",
                "-v", f"{builds.ROOT}:/repo:ro", "-v", f"{self.out}:/out", "--tmpfs", "/work:exec",
                "-w", "/work", self.image_id, "sh", self.container_path(script),
            )  # fmt: skip
        finally:
            script.unlink()
        outcomes = []
        for (name, replacement), command in zip(items, commands, strict=True):
            status_path = self.out / f"{name}.status"
            err_path = self.out / f"{name}.err"
            status = int(status_path.read_text()) if status_path.exists() else 125
            if status:
                error = err_path.read_text() if err_path.exists() else "batch compilation lane failed"
                outcomes.append(subprocess.CalledProcessError(status, command, stderr=error))
            else:
                outcomes.append((self.out / f"{name}.o", self.metadata(source, name, command, replacement)))
        return outcomes

    def metadata(self, source: Path, name: str, command: list[str], source_override: Path | None) -> dict:
        inputs, system = {}, []
        for dependency in parse_depfile((self.out / f"{name}.d").read_text()):
            host = self.host_path(dependency)
            if host is None:
                system.append(dependency)
            else:
                key = (
                    str(host.relative_to(builds.ROOT))
                    if host.resolve().is_relative_to(builds.ROOT)
                    else str(host.resolve())
                )
                actual = (
                    source_override
                    if source_override is not None and host.resolve() == source.resolve()
                    else host
                )
                inputs[key] = builds.sha256_file(actual)
        return {
            "container_image_id": self.image_id,
            "compiler_version": self.version,
            "package_manifest_sha256": self.manifest_sha256,
            "command": command,
            "inputs": inputs,
            "system_headers": len(system),
            "object_sha256": builds.sha256_file(self.out / f"{name}.o"),
        }


def parse_depfile(text: str) -> list[str]:
    body = text.replace("\\\n", " ").split(":", 1)[1]
    return body.split()
