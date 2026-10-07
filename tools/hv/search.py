"""Bounded definition-order search, with canonical compilation and explicit source blocks."""

import difflib
import hashlib
import json
import multiprocessing
import os
import random
import re
import subprocess
import uuid
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass
from pathlib import Path

from hv import builds, extents, match, progress, symbols, toolchain, units
from hv.elf import Elf

SCHEMA = 1


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    temporary.write_text(json.dumps(data, indent=2) + "\n")
    temporary.replace(path)


@dataclass(frozen=True)
class Blocks:
    """Explicit inclusive line ranges. Unselected bytes stay in their original slots."""

    source: bytes
    names: tuple[str, ...]
    spans: tuple[tuple[int, int], ...]
    chunks: tuple[bytes, ...]

    @classmethod
    def load(cls, source: bytes, spec: dict):
        if not isinstance(spec, dict) or not isinstance(spec.get("blocks"), list):
            raise ValueError("block specification must contain a blocks list")
        if spec.get("source_sha256") != sha(source):
            raise ValueError("block specification has a stale source_sha256")
        lines = source.splitlines(keepends=True)
        spans, chunks, names = [], [], []
        previous = 0
        for block in spec["blocks"]:
            if not isinstance(block, dict):
                raise ValueError("each block must specify a name, start and end")
            start, end, name = block.get("start"), block.get("end"), block.get("name")
            if type(start) is not int or type(end) is not int or not previous < start <= end <= len(lines):
                raise ValueError("block ranges must be ordered, nonoverlapping, inclusive source lines")
            if not isinstance(name, str) or not name or name in names:
                raise ValueError("block names must be nonempty and unique")
            a, b = sum(map(len, lines[: start - 1])), sum(map(len, lines[:end]))
            spans.append((a, b))
            chunks.append(source[a:b])
            names.append(name)
            previous = end
        if len(names) < 2:
            raise ValueError("search needs at least two explicit blocks")
        blocks = cls(source, tuple(names), tuple(spans), tuple(chunks))
        if blocks.render(tuple(range(len(names)))) != source:
            raise ValueError("identity block order must reproduce the original source")
        return blocks

    def render(self, order: tuple[int, ...]) -> bytes:
        if sorted(order) != list(range(len(self.names))):
            raise ValueError("order must contain every block exactly once")
        parts, previous = [], 0
        for (start, end), index in zip(self.spans, order, strict=True):
            parts.extend((self.source[previous:start], self.chunks[index]))
            previous = end
        return b"".join(parts) + self.source[previous:]


def code_lines(text: str):
    """Each line with comments, string and character literals blanked, for brace counting."""
    out, state = [], None  # None, "block" comment, or the quote of an open literal
    for line in text.splitlines(keepends=True):
        code, i = [], 0
        while i < len(line):
            c, pair = line[i], line[i : i + 2]
            if state == "block":
                if pair == "*/":
                    state, i = None, i + 2
                    continue
            elif state in ('"', "'"):
                if c == "\\":
                    i += 2
                    continue
                if c == state:
                    state = None
            elif pair == "//":
                break
            elif pair == "/*":
                state, i = "block", i + 2
                continue
            elif c in ('"', "'"):
                state = c
            else:
                code.append(c)
            i += 1
        out.append("".join(code))
    return out


def auto_blocks(source: bytes) -> dict:
    """A block specification with one block per top-level function definition, with the comment
    lines directly above it. Namespaces are transparent; data, declarations and classes stay put."""
    lines = source.decode().splitlines(keepends=True)
    blocks, stack = [], []
    lead = start = None
    header = ""
    for number, (line, code) in enumerate(zip(lines, code_lines(source.decode()), strict=True), 1):
        top = all(kind == "namespace" for kind in stack)
        stripped = line.strip()
        if top and start is None:
            if not stripped:
                lead = None
            elif stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
                lead = lead or number
            elif not stripped.startswith("#") and code.strip() and code.strip() != "}":
                start, header = number, ""
        for c in code:
            if c == "{":
                opens_namespace = all(kind == "namespace" for kind in stack) and header.split()[:1] == [
                    "namespace"
                ]
                stack.append("namespace" if opens_namespace else "code")
                if opens_namespace:
                    lead = start = None
            elif c == "}":
                kind = stack.pop() if stack else None
                if kind == "code" and all(k == "namespace" for k in stack) and start is not None:
                    declaration = header.strip()
                    words = declaration.split()
                    if (
                        "(" in declaration
                        and "=" not in declaration.split("(")[0]
                        and not (words and words[0] in ("class", "struct", "union", "enum"))
                    ):
                        name = re.search(r"([\w:~]+)\s*\(", declaration).group(1)
                        taken = sum(b["name"] == name or b["name"].startswith(name + "#") for b in blocks)
                        blocks.append(
                            {
                                "name": f"{name}#{taken + 1}" if taken else name,
                                "start": lead or start,
                                "end": number,
                            }
                        )
                    lead = start = None
            elif start is not None and all(kind == "namespace" for kind in stack):
                header += c
        if start is not None and all(kind == "namespace" for kind in stack) and code.strip().endswith(";"):
            lead = start = None  # a declaration or data definition without a body
    return {"source_sha256": sha(source), "blocks": blocks}


def neighbors(order: tuple[int, ...]):
    """Unique one-block moves (adjacent swaps otherwise occur twice)."""
    seen = {order}
    for i in range(len(order)):
        for j in range(len(order)):
            candidate = list(order)
            candidate.insert(j, candidate.pop(i))
            candidate = tuple(candidate)
            if candidate not in seen:
                seen.add(candidate)
                yield candidate


def exact_functions(result: dict) -> set[tuple]:
    return {
        (s["name"], f["symbol"], f["address"], f["size"])
        for s in result["sections"]
        for f in s.get("functions", [])
        if f["exact"]
    }


def exact_data(result: dict) -> set[tuple]:
    return {
        (s["name"], s.get("address"), s["size"])
        for s in result["sections"]
        if s["exact"] and not s.get("functions")
    }


def preserves(candidate: dict, baseline: dict) -> bool:
    return (
        exact_functions(baseline) <= exact_functions(candidate)
        and exact_data(baseline) <= exact_data(candidate)
        and (not baseline["exact"] or candidate["exact"])
    )


def score(result: dict) -> tuple[int, ...]:
    """Proven matches lead; unknown references never count as exact."""
    return (
        int(not result["exact"]),
        -len(exact_functions(result)),
        sum(len(s.get("misplaced_symbols", [])) for s in result["sections"]),
        sum(len(s.get("bad_references", [])) for s in result["sections"]),
        len(result["unplaced_sections"]),
        sum(not s["exact"] for s in result["sections"]),
    )


def counts(result: dict) -> dict:
    return {
        "exact": result["exact"],
        "exact_functions": len(exact_functions(result)),
        "unknown_functions": sum(
            bool(f.get("exact_but_unknown")) for s in result["sections"] for f in s.get("functions", [])
        ),
        "score": list(score(result)),
    }


class BudgetExhausted(Exception):
    pass


# (target, known, placements), inherited by forked comparison workers
_shared = None


def _compare_path(path: str) -> dict:
    target, known, placements = _shared
    return match.compare_object(Elf.load(Path(path), "ET_REL"), target, known, placements)


@dataclass
class Choice:
    order: tuple[int, ...]
    source: bytes
    result: dict


class Evaluator:
    """Persistent source/compilation cache and object-hash match cache, scoped to all inputs."""

    def __init__(self, unit, blocks, target, known, compiler, context, run, budget):
        self.unit, self.blocks, self.target, self.known = unit, blocks, target, known
        self.compiler, self.context, self.run, self.budget = compiler, context, run, budget
        self.fingerprint = sha(json.dumps(context, sort_keys=True).encode())
        self.cache = compiler.out / "cache" / self.fingerprint
        self.cache.mkdir(parents=True, exist_ok=True)
        self.sources = {}
        self.objects = {}
        self.pending = {}  # digest -> batch compilation outcome, not yet evaluated
        self.ahead = {}  # object digest -> comparison result or error, not yet evaluated
        self.stats = {}  # input name -> (size, mtime, inode) when its hash was last verified
        self.paths = {name: str(builds.ROOT / name) for name in context["inputs"]}
        self.evaluated = self.compiled = self.cache_hits = self.object_hits = 0

    def fresh(self):
        """Fail when a repository input changed. Files whose size, mtime and inode are unchanged since
        their last verified hash are not hashed again."""
        expected = self.context["inputs"]
        stale = []
        for name in expected:
            try:
                stat = os.stat(self.paths[name])
            except FileNotFoundError:
                raise ValueError(
                    "repository inputs changed during search; candidate was not applied"
                ) from None
            key = (stat.st_size, stat.st_mtime_ns, stat.st_ino)
            if self.stats.get(name) != key:
                stale.append((name, key))
        if stale and progress.input_hashes([n for n, _ in stale]) != {n: expected[n] for n, _ in stale}:
            raise ValueError("repository inputs changed during search; candidate was not applied")
        self.stats.update(stale)

    def compare(self, obj: Path):
        digest = builds.sha256_file(obj)
        if digest in self.objects:
            self.object_hits += 1
            return self.objects[digest]
        result = self.ahead.pop(digest, None)
        if isinstance(result, Exception):
            raise result
        if result is None:
            compiled = Elf.load(obj, "ET_REL")
            result = match.compare_object(compiled, self.target, self.known, self.unit.placements)
        self.objects[digest] = result
        return result

    def compare_ahead(self, objects: list[Path]):
        """Compare a batch's new objects in parallel; compare and evaluate consume them in order."""
        todo = {}
        for obj in objects:
            digest = builds.sha256_file(obj)
            if digest not in self.objects and digest not in self.ahead:
                todo[digest] = obj
        if len(todo) < 2:
            return
        global _shared
        _shared = (self.target, self.known, self.unit.placements)
        workers = min(len(todo), os.cpu_count() or 1)
        with ProcessPoolExecutor(workers, mp_context=multiprocessing.get_context("fork")) as pool:
            futures = {digest: pool.submit(_compare_path, str(obj)) for digest, obj in todo.items()}
            for digest, future in futures.items():
                self.ahead[digest] = future.exception() or future.result()

    def validate_compilation(self, metadata: dict, digest: str):
        expected = {**self.context["inputs"], f"src/{self.unit.source}": digest}
        # GCC preserves relative spellings such as src/ox/net/../core/CString.h in its depfile.
        inputs = {
            str((builds.ROOT / name).resolve().relative_to(builds.ROOT)): value
            for name, value in metadata["inputs"].items()
        }
        if inputs.get(f"src/{self.unit.source}") != digest or any(
            expected.get(name) != value for name, value in inputs.items()
        ):
            raise ValueError("compilation dependencies differ from the search snapshot")

    def cached(self, digest: str) -> dict | None:
        entry = self.cache / (digest + ".json")
        obj = self.cache / (digest + ".o")
        cached = json.loads(entry.read_text()) if entry.exists() else None
        if cached and obj.exists() and builds.sha256_file(obj) == cached["compilation"]["object_sha256"]:
            return cached
        return None

    def prefetch(self, orders):
        """Compile the uncached candidates among orders in one container, within the budget left.
        Their results are evaluated, logged and counted only when evaluate reaches them."""
        todo = {}
        for order in orders:
            source = self.blocks.render(order)
            digest = sha(source)
            if digest in self.sources or digest in self.pending or digest in todo or self.cached(digest):
                continue
            if self.evaluated + len(self.pending) + len(todo) >= self.budget:
                break
            todo[digest] = source
        if len(todo) < 2:
            return
        self.fresh()
        items = []
        for digest, source in todo.items():
            candidate = self.cache / (digest + ".cpp")
            candidate.write_bytes(source)
            items.append((str((self.cache / digest).relative_to(self.compiler.out)), candidate))
        outcomes = self.compiler.compile_many(self.unit.path, items)
        self.compiled += len(items)
        self.pending.update(zip(todo, outcomes, strict=True))
        self.compare_ahead([o[0] for o in outcomes if not isinstance(o, subprocess.CalledProcessError)])
        self.fresh()

    def evaluate(self, order):
        source = self.blocks.render(order)
        digest = sha(source)
        if digest in self.sources:
            return self.sources[digest]
        if self.evaluated >= self.budget:
            raise BudgetExhausted
        self.evaluated += 1
        self.fresh()
        entry = self.cache / (digest + ".json")
        cached = self.cached(digest)
        if cached:
            self.cache_hits += 1
            result, metadata = cached["result"], cached["compilation"]
            self.validate_compilation(metadata, digest)
            if result["object_sha256"] != metadata["object_sha256"]:
                raise ValueError("cached match object identity mismatch")
            self.objects[result["object_sha256"]] = result
        else:
            try:
                if digest in self.pending:
                    outcome = self.pending.pop(digest)
                    if isinstance(outcome, subprocess.CalledProcessError):
                        raise outcome
                    obj, metadata = outcome
                else:
                    candidate = self.cache / (digest + ".cpp")
                    candidate.write_bytes(source)
                    name = str((self.cache / digest).relative_to(self.compiler.out))
                    self.compiled += 1
                    obj, metadata = self.compiler.compile(self.unit.path, name, source_override=candidate)
            except subprocess.CalledProcessError as error:
                if error.returncode != 1:
                    raise
                self.fresh()
                self.sources[digest] = None
                self.log(order, digest, {"compile_error": (error.stderr or str(error))[-2000:]})
                return None
            result = self.compare(obj)
            self.validate_compilation(metadata, digest)
            write_json(entry, {"result": result, "compilation": metadata})
        self.fresh()
        choice = Choice(order, source, result)
        self.sources[digest] = choice
        self.log(order, digest, {**counts(result), "object_sha256": result["object_sha256"]})
        return choice

    def log(self, order, digest, details):
        row = {"order": [self.blocks.names[i] for i in order], "source_sha256": digest, **details}
        with (self.run / "trials.jsonl").open("a") as stream:
            stream.write(json.dumps(row) + "\n")


def climb(
    evaluate,
    initial: Choice,
    restarts: int,
    sideways: int,
    seed: int,
    checkpoint,
    restart_budget=None,
    prefetch=None,
    batch=16,
):
    """Greedy one-block moves, bounded neutral steps, then seeded restarts. With prefetch, each run
    of up to batch neighbours is compiled together before they are evaluated in order."""
    rng = random.Random(seed)
    best = current = initial
    visited = {initial.order}
    reason = "plateau"
    try:
        for restart in range(restarts + 1):
            attempts = 0
            if restart:
                order = list(initial.order)
                for _ in range(8):
                    rng.shuffle(order)
                    if tuple(order) not in visited:
                        break
                else:
                    continue
                order = tuple(order)
                visited.add(order)
                attempts += 1
                current = evaluate(order)
                if current is None or not preserves(current.result, best.result):
                    continue
                if score(current.result) < score(best.result):
                    best = current
                    checkpoint(best)
            neutral = 0
            while not best.result["exact"]:
                if restart_budget is not None and attempts >= restart_budget:
                    reason = "budget"
                    break
                options = []
                orders = list(neighbors(current.order))
                rng.shuffle(orders)
                orders = [o for o in orders if o not in visited]
                for k, order in enumerate(orders):
                    if restart_budget is not None and attempts >= restart_budget:
                        reason = "budget"
                        break
                    if prefetch is not None and k % batch == 0:
                        room = batch if restart_budget is None else min(batch, restart_budget - attempts)
                        prefetch(orders[k : k + room])
                    visited.add(order)
                    attempts += 1
                    candidate = evaluate(order)
                    if candidate is None or not preserves(candidate.result, best.result):
                        continue
                    if score(candidate.result) < score(best.result):
                        best = candidate
                        checkpoint(best)
                    options.append(candidate)
                    if best.result["exact"]:
                        break
                if best.result["exact"]:
                    break
                options = [c for c in options if preserves(c.result, best.result)]
                if not options:
                    break
                next_score = min(score(c.result) for c in options)
                choices = [c for c in options if score(c.result) == next_score]
                if next_score < score(current.result):
                    current, neutral = rng.choice(choices), 0
                elif next_score == score(current.result) and neutral < sideways:
                    current = rng.choice(choices)
                    neutral += 1
                else:
                    break
            if best.result["exact"]:
                reason = "exact"
                break
    except BudgetExhausted:
        reason = "budget"
    if best.result["exact"]:
        reason = "exact"
    return best, reason


def save_candidate(run: Path, unit, blocks: Blocks, choice: Choice):
    (run / "candidate.cpp").write_bytes(choice.source)
    patch = "".join(
        difflib.unified_diff(
            blocks.source.decode().splitlines(keepends=True),
            choice.source.decode().splitlines(keepends=True),
            fromfile=f"a/src/{unit.source}",
            tofile=f"b/src/{unit.source}",
        )
    )
    (run / "candidate.patch").write_text(patch)
    write_json(
        run / "best.json",
        {
            "order": [blocks.names[i] for i in choice.order],
            "source_sha256": sha(choice.source),
            "result": choice.result,
        },
    )


def search(
    unit_name: str,
    spec_path: Path | None,
    build: str,
    *,
    budget=100,
    restarts=2,
    sideways=3,
    seed=0,
    apply=False,
    batch=16,
):
    """Search definition orders of a unit. Without spec_path, every top-level function definition
    is a block (auto_blocks)."""
    if budget < 1 or restarts < 0 or sideways < 0 or batch < 1:
        raise ValueError("budget and batch must be positive; restarts and sideways must be nonnegative")
    selected = [u for u in units.load(build) if u.source == unit_name]
    if not selected:
        raise ValueError(f"not in units.toml: {unit_name}")
    (unit,) = selected
    original = unit.path.read_bytes()
    spec = auto_blocks(original) if spec_path is None else json.loads(spec_path.read_text())
    blocks = Blocks.load(original, spec)
    (image,) = builds.load_builds()[build].images.values()
    if problem := builds.check_image(image):
        raise ValueError(f"{image.path}: {problem}")
    target = extents.load_target(build)
    inputs = sorted(
        set(
            progress.measurement_paths(build)
            + ["tools/hv/search.py"]
            + [
                str(p.relative_to(builds.ROOT))
                for base in ("src", "third_party")
                for p in (builds.ROOT / base).rglob("*")
                if p.is_file()
            ]
        )
    )
    hashes = progress.input_hashes(inputs)
    out = builds.ROOT / "build" / "search" / build / unit.slug
    compiler = toolchain.Compiler(toolchain.load_flags(build), out)
    context = {
        "schema": SCHEMA,
        "build": build,
        "unit": unit.source,
        "placements": unit.placements,
        "image_sha256": image.sha256,
        "container_image_id": compiler.image_id,
        "compiler_version": compiler.version,
        "inputs": hashes,
    }
    run = out / "runs" / uuid.uuid4().hex
    run.mkdir(parents=True)
    write_json(
        run / "context.json",
        {
            **context,
            "blocks": spec,
            "budget": budget,
            "restarts": restarts,
            "sideways": sideways,
            "seed": seed,
            "batch": batch,
        },
    )
    evaluator = Evaluator(
        unit, blocks, target, symbols.by_name(symbols.load(build)), compiler, context, run, budget
    )
    evaluator.fresh()
    obj, metadata = compiler.compile(unit.path, str((run / "baseline").relative_to(out)))
    evaluator.validate_compilation(metadata, sha(original))
    baseline = evaluator.compare(obj)
    evaluator.fresh()
    order = tuple(range(len(blocks.names)))
    initial = Choice(order, original, baseline)
    evaluator.sources[sha(original)] = initial
    write_json(run / "baseline.json", {"result": baseline, "compilation": metadata})
    print(f"baseline   {counts(baseline)}", flush=True)

    def checkpoint(choice):
        save_candidate(run, unit, blocks, choice)
        print(f"best       {counts(choice.result)}", flush=True)

    checkpoint(initial)
    best, reason = climb(
        evaluator.evaluate,
        initial,
        restarts,
        sideways,
        seed,
        checkpoint,
        restart_budget=max(1, budget // (restarts + 1)),
        prefetch=evaluator.prefetch,
        batch=batch,
    )
    improved = score(best.result) < score(baseline)
    applied = False
    if improved:
        evaluator.fresh()
        obj, metadata = compiler.compile(
            unit.path, str((run / "verified").relative_to(out)), source_override=run / "candidate.cpp"
        )
        verified = match.compare_object(Elf.load(obj, "ET_REL"), target, evaluator.known, unit.placements)
        evaluator.validate_compilation(metadata, sha(best.source))
        evaluator.fresh()
        if verified != best.result or not preserves(verified, baseline):
            raise ValueError("fresh canonical comparison disagrees with the winning candidate")
        write_json(run / "verification.json", {"result": verified, "compilation": metadata})
        if apply:
            if unit.path.read_bytes() != original:
                raise ValueError("source changed during search; candidate was not applied")
            unit.path.write_bytes(best.source)
            applied = True
    else:
        evaluator.fresh()
    summary = {
        "reason": reason,
        "improved": improved,
        "applied": applied,
        "baseline": counts(baseline),
        "best": counts(best.result),
        "evaluated": evaluator.evaluated,
        "compiled": evaluator.compiled,
        "cache_hits": evaluator.cache_hits,
        "object_hits": evaluator.object_hits,
    }
    write_json(run / "summary.json", summary)
    print(f"{reason:10} {json.dumps(summary)}\nevidence   {run}", flush=True)
    return summary
