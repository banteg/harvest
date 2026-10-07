import hashlib
import json
import subprocess
from types import SimpleNamespace

import pytest

from hv import builds, cli, search, toolchain
from hv.units import Unit


def blocks(source=b"head\nA\nfixed\nB\ntail\n"):
    spec = {
        "source_sha256": search.sha(source),
        "blocks": [
            {"name": "a", "start": 2, "end": 2},
            {"name": "b", "start": 4, "end": 4},
        ],
    }
    return search.Blocks.load(source, spec)


def result(exact=False, names=("kept",), misplaced=1, unknown=False, data=True):
    functions = [
        {"symbol": n, "address": hex(0x1000 + i * 8), "size": 8, "exact": True} for i, n in enumerate(names)
    ]
    if unknown:
        functions.append(
            {
                "symbol": "unresolved",
                "address": "0x2000",
                "size": 8,
                "exact": False,
                "exact_but_unknown": True,
            }
        )
    return {
        "exact": exact,
        "object_sha256": "obj",
        "unplaced_sections": [],
        "sections": [
            {
                "name": ".text",
                "size": 40,
                "exact": exact,
                "functions": functions,
                "misplaced_symbols": [{}] * misplaced,
            },
            {"name": ".rodata", "address": "0x3000", "size": 4, "exact": data},
        ],
    }


def choice(order=(0, 1, 2), **kwargs):
    return search.Choice(order, repr(order).encode(), result(**kwargs))


def test_blocks_preserve_fixed_bytes_and_original_identity():
    b = blocks()
    assert b.render((0, 1)) == b.source
    assert b.render((1, 0)) == b"head\nB\nfixed\nA\ntail\n"
    with pytest.raises(ValueError, match="every block"):
        b.render((1, 1))


@pytest.mark.parametrize("ranges", [((2, 3), (3, 4)), ((0, 1), (4, 4)), ((2, 2), (4, 8))])
def test_blocks_reject_invalid_ranges(ranges):
    spec = {
        "source_sha256": search.sha(b"A\nB\nC\nD\n"),
        "blocks": [{"name": str(i), "start": a, "end": b} for i, (a, b) in enumerate(ranges)],
    }
    with pytest.raises(ValueError, match="ranges"):
        search.Blocks.load(b"A\nB\nC\nD\n", spec)


def test_blocks_reject_stale_source():
    with pytest.raises(ValueError, match="stale"):
        search.Blocks.load(b"changed\n", {"source_sha256": "old", "blocks": []})


def test_neighbors_deduplicate_adjacent_swaps():
    orders = list(search.neighbors((0, 1, 2)))
    assert len(orders) == len(set(orders)) == 4
    assert (0, 1, 2) not in orders


def test_score_and_acceptance_protect_proven_functions_and_data():
    baseline = result(names=("kept",), misplaced=4)
    regressed = result(names=(), misplaced=0, unknown=True)
    assert not search.preserves(regressed, baseline)
    assert search.score(baseline) < search.score(regressed)
    assert not search.preserves(result(data=False), baseline)
    assert search.counts(regressed)["exact_functions"] == 0
    assert search.counts(regressed)["unknown_functions"] == 1


def test_zero_misplacements_does_not_end_body_search():
    initial = choice(misplaced=0)
    seen = []

    def evaluate(order):
        seen.append(order)
        return choice(order, exact=True, misplaced=0)

    best, reason = search.climb(evaluate, initial, 0, 0, 0, lambda c: None)
    assert seen and best.result["exact"] and reason == "exact"


def test_search_rejects_a_layout_gain_that_loses_an_exact_function():
    initial = choice(misplaced=4)
    best, reason = search.climb(
        lambda order: choice(order, names=(), misplaced=0), initial, 0, 0, 0, lambda c: None
    )
    assert best is initial and reason == "plateau"


def test_neutral_move_can_reach_an_improvement(monkeypatch):
    initial = choice()
    neutral, win = (1, 0, 2), (1, 2, 0)
    edges = {initial.order: [neutral], neutral: [win]}
    monkeypatch.setattr(search, "neighbors", lambda order: iter(edges.get(order, [])))

    def evaluate(order):
        return choice(order, exact=order == win, misplaced=0 if order == win else 1)

    best, _ = search.climb(evaluate, initial, 0, 0, 0, lambda c: None)
    assert best is initial
    best, reason = search.climb(evaluate, initial, 0, 1, 0, lambda c: None)
    assert best.order == win and reason == "exact"


def test_seeded_restart_can_leave_a_local_minimum(monkeypatch):
    initial = choice()
    monkeypatch.setattr(search, "neighbors", lambda order: iter(()))

    def evaluate(order):
        return choice(order, exact=True, misplaced=0)

    a, _ = search.climb(evaluate, initial, 1, 0, 1, lambda c: None)
    b, reason = search.climb(evaluate, initial, 1, 0, 1, lambda c: None)
    assert a.order == b.order != initial.order and reason == "exact"


def test_budget_keeps_a_winner_found_before_exhaustion():
    initial = choice(misplaced=4)
    calls = []

    def evaluate(order):
        calls.append(order)
        if len(calls) > 1:
            raise search.BudgetExhausted
        return choice(order, misplaced=2)

    saved = []
    best, reason = search.climb(evaluate, initial, 0, 0, 0, saved.append)
    assert best.result["sections"][0]["misplaced_symbols"] == [{}, {}]
    assert saved == [best] and reason == "budget"


class FakeCompiler:
    image_id = "pinned-image"
    version = "pinned-compiler"
    manifest_sha256 = "manifest"

    def __init__(self, out):
        self.out = out
        self.calls = 0

    def compile(self, source, name, *, source_override=None):
        self.calls += 1
        data = source_override.read_bytes() if source_override is not None else source.read_bytes()
        obj = self.out / (name + ".o")
        obj.parent.mkdir(parents=True, exist_ok=True)
        obj.write_bytes(b"compiled")  # distinct orders that compile to the same object
        return obj, {
            "object_sha256": search.sha(obj.read_bytes()),
            "inputs": {"src/unit.cpp": search.sha(data)},
        }

    def compile_many(self, source, items):
        self.batches = getattr(self, "batches", 0) + 1
        outcomes = []
        for name, replacement in items:
            obj = self.out / (name + ".o")
            obj.parent.mkdir(parents=True, exist_ok=True)
            obj.write_bytes(b"compiled")
            data = replacement.read_bytes()
            outcomes.append(
                (
                    obj,
                    {"object_sha256": search.sha(b"compiled"), "inputs": {"src/unit.cpp": search.sha(data)}},
                )
            )
        return outcomes


@pytest.fixture
def evaluator(tmp_path, monkeypatch):
    (tmp_path / "src").mkdir()
    unit = Unit("unit.cpp", {})
    b = blocks()
    (tmp_path / "src/unit.cpp").write_bytes(b.source)
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    compiler = FakeCompiler(tmp_path / "out")
    compiler.out.mkdir()
    run = tmp_path / "run"
    run.mkdir()
    context = {"inputs": {"src/unit.cpp": search.sha(b.source)}}
    monkeypatch.setattr(search.Elf, "load", lambda *args: None)
    monkeypatch.setattr(search.extents, "load_target", lambda build: None)
    calls = []

    def compare(*args):
        calls.append(True)
        r = result()
        r["object_sha256"] = search.sha(b"compiled")
        return r

    monkeypatch.setattr(search.match, "compare_object", compare)
    e = search.Evaluator(unit, b, None, {}, compiler, context, run, 10)
    return e, compiler, calls


def test_source_and_object_caches_avoid_repeated_work(evaluator):
    e, compiler, calls = evaluator
    a = e.evaluate((1, 0))
    assert e.evaluate((1, 0)) is a
    e.evaluate((0, 1))
    assert compiler.calls == 2 and len(calls) == 1 and e.object_hits == 1
    again = search.Evaluator(e.unit, e.blocks, None, {}, compiler, e.context, e.run, 10)
    again.evaluate((1, 0))
    assert compiler.calls == 2 and again.cache_hits == 1
    changed = search.Evaluator(
        e.unit, e.blocks, None, {}, compiler, {**e.context, "compiler": "different"}, e.run, 10
    )
    changed.evaluate((1, 0))
    assert compiler.calls == 3


def test_cache_recompiles_a_corrupted_object(evaluator):
    e, compiler, _ = evaluator
    e.evaluate((1, 0))
    obj = next(e.cache.glob("*.o"))
    obj.write_bytes(b"corrupted")
    again = search.Evaluator(e.unit, e.blocks, None, {}, compiler, e.context, e.run, 10)
    again.evaluate((1, 0))
    assert compiler.calls == 2 and again.cache_hits == 0


def test_evaluator_budget_counts_unique_sources(evaluator):
    e, compiler, _ = evaluator
    e.budget = 1
    e.evaluate((1, 0))
    e.evaluate((1, 0))
    with pytest.raises(search.BudgetExhausted):
        e.evaluate((0, 1))
    assert compiler.calls == 1


def test_repository_changes_abort_the_search(evaluator):
    e, compiler, _ = evaluator
    e.unit.path.write_bytes(b"concurrent edit")
    with pytest.raises(ValueError, match="changed"):
        e.evaluate((1, 0))
    assert compiler.calls == 0


def test_invalid_candidate_does_not_abort_the_neighborhood(evaluator, monkeypatch):
    e, compiler, _ = evaluator

    def compile(*args, **kwargs):
        raise subprocess.CalledProcessError(1, ["g++"], stderr="not declared")

    monkeypatch.setattr(compiler, "compile", compile)
    assert e.evaluate((1, 0)) is None
    assert "not declared" in (e.run / "trials.jsonl").read_text()


def test_cli_rejects_unknown_unit_before_starting_docker(capsys):
    assert cli.main(["search", "no/such/File.cpp", "--blocks", "absent.json"]) == 2
    assert "not in units.toml" in capsys.readouterr().err


def test_compiler_overlay_uses_canonical_path_and_hashes_actual_source(tmp_path, monkeypatch):
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    source = tmp_path / "src/unit.cpp"
    source.parent.mkdir()
    source.write_bytes(b"original")
    compiler = toolchain.Compiler.__new__(toolchain.Compiler)
    compiler.out = tmp_path / "out"
    compiler.out.mkdir()
    compiler.flags = {"compiler": "g++", "flags": ["-O2", "-Isrc"]}
    compiler.image_id, compiler.version, compiler.manifest_sha256 = "image", "version", "manifest"
    candidate = compiler.out / "candidate.cpp"
    candidate.write_bytes(b"candidate")
    commands = []

    def run(*args):
        commands.append(args)
        (compiler.out / "candidate.o").write_bytes(b"object")
        (compiler.out / "candidate.d").write_text("/out/candidate.o: src/unit.cpp\n")
        return ""

    monkeypatch.setattr(toolchain, "run", run)
    _, metadata = compiler.compile(source, "candidate", source_override=candidate)
    assert "--security-opt=label=disable" in commands[0]
    assert f"{candidate}:/work/src/unit.cpp:ro" in commands[0]
    assert metadata["inputs"] == {"src/unit.cpp": hashlib.sha256(b"candidate").hexdigest()}
    assert metadata["command"][-3] == "src/unit.cpp"
    assert source.read_bytes() == b"original"


@pytest.fixture
def search_project(tmp_path, monkeypatch):
    source = blocks().source
    path = tmp_path / "src/unit.cpp"
    path.parent.mkdir()
    path.write_bytes(source)
    script = tmp_path / "tools/hv/search.py"
    script.parent.mkdir(parents=True)
    script.write_text("search context")
    spec = tmp_path / "blocks.json"
    spec.write_text(
        json.dumps(
            {
                "source_sha256": search.sha(source),
                "blocks": [
                    {"name": "a", "start": 2, "end": 2},
                    {"name": "b", "start": 4, "end": 4},
                ],
            }
        )
    )
    image = SimpleNamespace(path=tmp_path / "target", sha256="pin")
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    monkeypatch.setattr(builds, "load_builds", lambda: {"b": SimpleNamespace(images={"image": image})})
    monkeypatch.setattr(builds, "check_image", lambda image: None)
    monkeypatch.setattr(search.units, "load", lambda b: [Unit("unit.cpp", {})])
    monkeypatch.setattr(search.symbols, "load", lambda b: [])
    monkeypatch.setattr(search.toolchain, "load_flags", lambda b: {})
    monkeypatch.setattr(search.progress, "measurement_paths", lambda b: ["src/unit.cpp"])
    state = SimpleNamespace(calls=0, verification_change=False, concurrent_edit=False)

    class Compiler(FakeCompiler):
        def compile(self, source, name, *, source_override=None):
            state.calls += 1
            data = source_override.read_bytes() if source_override else source.read_bytes()
            if "verified" in name:
                if state.verification_change:
                    data = b"changed compiler result"
                if state.concurrent_edit:
                    source.write_bytes(b"another worker edited this")
            obj = self.out / (name + ".o")
            obj.parent.mkdir(parents=True, exist_ok=True)
            obj.write_bytes(data)
            digest = search.sha(source_override.read_bytes() if source_override else source.read_bytes())
            return obj, {"object_sha256": search.sha(data), "inputs": {"src/unit.cpp": digest}}

    def compiler(flags, out):
        out.mkdir(parents=True, exist_ok=True)
        return Compiler(out)

    monkeypatch.setattr(search.toolchain, "Compiler", compiler)
    monkeypatch.setattr(search.Elf, "load", lambda p, kind: p.read_bytes() if kind == "ET_REL" else None)
    monkeypatch.setattr(search.extents, "load_target", lambda build: None)

    def compare(obj, *args):
        winning = obj == b"head\nB\nfixed\nA\ntail\n"
        r = result(exact=winning, misplaced=0 if winning else 2)
        r["object_sha256"] = search.sha(obj)
        return r

    monkeypatch.setattr(search.match, "compare_object", compare)
    return path, spec, state


@pytest.mark.parametrize("apply", [False, True])
def test_search_saves_patch_and_verifies_before_optional_application(search_project, apply):
    path, spec, state = search_project
    original = path.read_bytes()
    summary = search.search("unit.cpp", spec, "b", budget=2, restarts=0, apply=apply)
    assert summary["improved"] and summary["best"]["exact"]
    assert summary["applied"] == apply
    assert state.calls == 3  # canonical baseline, candidate, independent verification
    assert path.read_bytes() == (b"head\nB\nfixed\nA\ntail\n" if apply else original)
    run = next((builds.ROOT / "build/search/b/unit/runs").iterdir())
    assert "+B" in (run / "candidate.patch").read_text()
    assert (run / "verification.json").exists()
    assert json.loads((run / "summary.json").read_text()) == summary


@pytest.mark.parametrize("failure", ["verification_change", "concurrent_edit"])
def test_failed_verification_never_applies_a_candidate(search_project, failure):
    path, spec, state = search_project
    original = path.read_bytes()
    setattr(state, failure, True)
    with pytest.raises(ValueError, match="disagrees|changed"):
        search.search("unit.cpp", spec, "b", budget=2, restarts=0, apply=True)
    assert path.read_bytes() == (b"another worker edited this" if state.concurrent_edit else original)
    run = next((builds.ROOT / "build/search/b/unit/runs").iterdir())
    assert (run / "candidate.patch").exists()
    assert not (run / "verification.json").exists()


def test_dependency_validation_normalizes_gcc_relative_include_paths(evaluator):
    e, _, _ = evaluator
    digest = search.sha(e.blocks.source)
    e.validate_compilation({"inputs": {"src/dir/../unit.cpp": digest}}, digest)
    with pytest.raises(ValueError, match="dependencies"):
        e.validate_compilation({"inputs": {"src/unit.cpp": "wrong hash"}}, digest)


def test_restart_quota_reserves_budget_for_a_new_start(monkeypatch):
    initial = choice()
    calls = []

    def evaluate(order):
        calls.append(order)
        return choice(order)

    monkeypatch.setattr(search, "neighbors", lambda order: iter([(1, 0, 2), (2, 0, 1)]))
    search.climb(evaluate, initial, 1, 1, 1, lambda c: None, restart_budget=1)
    assert len(calls) == 2


def test_prefetch_compiles_a_neighbourhood_in_one_batch_and_evaluate_uses_it(evaluator):
    e, compiler, _ = evaluator
    e.prefetch([(1, 0), (0, 1), (1, 0)])
    assert compiler.batches == 1 and compiler.calls == 0 and e.evaluated == 0 and e.compiled == 2
    a, b = e.evaluate((1, 0)), e.evaluate((0, 1))
    assert a and b and compiler.calls == 0 and e.evaluated == 2 and not e.pending
    assert len((e.run / "trials.jsonl").read_text().splitlines()) == 2


def test_prefetch_stays_within_the_budget(evaluator):
    e, compiler, _ = evaluator
    e.budget = 1
    e.prefetch([(1, 0), (0, 1)])
    assert not getattr(compiler, "batches", 0) and not e.pending


def test_parallel_comparisons_reuse_objects_and_defer_candidate_errors(evaluator, monkeypatch):
    e, compiler, _ = evaluator
    good, duplicate, bad = [compiler.out / name for name in ("good.o", "duplicate.o", "bad.o")]
    good.write_bytes(b"good")
    duplicate.write_bytes(b"good")
    bad.write_bytes(b"bad")
    monkeypatch.setattr(search.Elf, "load", lambda path, kind: path.read_bytes())
    monkeypatch.setattr(search.extents, "load_target", lambda build: None)

    def compare(obj, *args):
        if obj == b"bad":
            raise ValueError("invalid candidate object")
        return {**result(), "object_sha256": search.sha(obj)}

    monkeypatch.setattr(search.match, "compare_object", compare)
    e.compare_ahead([good, duplicate, bad])
    assert len(e.ahead) == 2
    assert not e.objects and e.evaluated == 0 and e.object_hits == 0
    assert not (e.run / "trials.jsonl").exists()
    expected = compare(b"good")
    assert e.compare(good) == expected
    assert e.compare(duplicate) == expected and e.object_hits == 1
    with pytest.raises(ValueError, match="invalid candidate object"):
        e.compare(bad)
    assert not e.ahead and search.sha(b"bad") not in e.objects


def test_batched_compile_failures_are_logged_like_single_ones(evaluator, monkeypatch):
    e, compiler, _ = evaluator
    error = subprocess.CalledProcessError(1, ["g++"], stderr="not declared")
    monkeypatch.setattr(compiler, "compile_many", lambda source, items: [error] * len(items))
    e.prefetch([(1, 0), (0, 1)])
    assert e.evaluate((1, 0)) is None
    assert "not declared" in (e.run / "trials.jsonl").read_text()


def test_climb_prefetches_neighbours_in_batches():
    initial = choice(order=(0, 1, 2, 3))
    batches = []

    def evaluate(order):
        return choice(order=order)

    search.climb(evaluate, initial, 0, 0, 0, lambda c: None, prefetch=batches.append, batch=4)
    assert batches and all(len(b) <= 4 for b in batches)
    assert len({o for b in batches for o in b}) == sum(map(len, batches))


def test_compile_many_uses_canonical_paths_in_a_private_copy(tmp_path, monkeypatch):
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    source = tmp_path / "src/unit.cpp"
    source.parent.mkdir()
    source.write_bytes(b"original")
    compiler = toolchain.Compiler.__new__(toolchain.Compiler)
    compiler.out = tmp_path / "out"
    compiler.out.mkdir()
    compiler.flags = {"compiler": "g++", "flags": ["-O2", "-Isrc", "-Ithird_party/lua/include"]}
    compiler.image_id, compiler.version, compiler.manifest_sha256 = "image", "version", "manifest"
    good, bad = compiler.out / "good.cpp", compiler.out / "bad.cpp"
    good.write_bytes(b"good")
    bad.write_bytes(b"bad")
    scripts = []

    def run(*args):
        scripts.append((args, (compiler.out / args[-1].removeprefix("/out/")).read_text()))
        (compiler.out / "good.status").write_text("0\n")
        (compiler.out / "good.o").write_bytes(b"object")
        (compiler.out / "good.d").write_text("/out/good.o: src/unit.cpp\n")
        (compiler.out / "bad.status").write_text("1\n")
        (compiler.out / "bad.err").write_text("error: not declared\n")
        return ""

    monkeypatch.setattr(toolchain, "run", run)
    (obj, metadata), failure = compiler.compile_many(source, [("good", good), ("bad", bad)], lanes=2)
    args, script = scripts[0]
    assert "--security-opt=label=disable" in args
    assert f"{tmp_path}:/repo:ro" in args and "/work:exec" in args
    assert script.count("cp -a /repo/src src") == 2 and "cp -a /repo/third_party third_party" in script
    assert "cd /work/0" in script and "cd /work/1" in script and script.rstrip().endswith("wait")
    assert "cp /out/good.cpp src/unit.cpp" in script and " -c src/unit.cpp " in script
    assert metadata["inputs"] == {"src/unit.cpp": hashlib.sha256(b"good").hexdigest()}
    assert metadata["command"] == compiler.compile_command(source, "good")
    assert isinstance(failure, subprocess.CalledProcessError) and "not declared" in failure.stderr
    assert not list(compiler.out.glob("batch-*.sh")) and source.read_bytes() == b"original"


def test_auto_blocks_select_top_level_function_definitions():
    source = b"""// header
#include "x.h"

namespace a {
namespace b {

static const int TABLE[] = { 1, 2 };

//! A functor.
class F
{
    bool test() { return true; }
};

int X::y = 0;

//! First.
X::X(int v)
    : Y(v)
{
    const char* s = "}{";  // }
}

void X::f()
{
    if (true) { }
}

void X::f(int)
{
}

} // end namespace b
} // end namespace a
"""
    spec = search.auto_blocks(source)
    assert [(b["name"], b["start"], b["end"]) for b in spec["blocks"]] == [
        ("X::X", 17, 22),
        ("X::f", 24, 27),
        ("X::f#2", 29, 31),
    ]
    blocks = search.Blocks.load(source, spec)
    swapped = blocks.render((1, 0, 2)).decode()
    assert swapped.index("void X::f()") < swapped.index("//! First.") < swapped.index("void X::f(int)")
