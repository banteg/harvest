import copy
import json

import pytest

from hv import builds, progress, units


def sample():
    inv = {
        "total_code": 100,
        "total_data": 0,
        "data_sections": [],
        "sections": [
            {"name": ".text", "address": 0x1000, "size": 90},
            {"name": ".plt", "address": 0x2000, "size": 10},
        ],
    }
    functions = [
        {"address": 0x1000, "size": 20, "section": ".text"},
        {"address": 0x1020, "size": 40, "section": ".text"},
    ]
    function = {"symbol": "_Z3foov", "address": "0x1000", "size": 20, "fde": True, "exact": True}
    unit = {
        "unit": "a.cpp",
        "exact": True,
        "unplaced_sections": [],
        "sections": [{"name": ".text", "exact": True, "functions": [function]}],
    }
    return inv, functions, {"units": [unit]}


def test_full_denominator_keeps_gaps_and_stubs():
    inv, functions, evidence = sample()
    report = progress.make_report(inv, functions, evidence, {}, {"_Z3foov": "foo()"})
    assert report["version"] == 2
    m = report["measures"]
    assert m["total_code"] == "100" and m["matched_code"] == "20"
    assert m["matched_code_percent"] == 20
    assert m["total_functions"] == 2 and m["matched_functions"] == 1
    assert m["complete_code"] == "0" and m["complete_units"] == 0
    assert sum(int(u["measures"]["total_code"]) for u in report["units"]) == 100
    assert sum(int(c["measures"]["total_code"]) for c in report["categories"]) == 100
    assert report["units"][0]["name"] == "foo() @ 0x1000"
    assert report["units"][0]["functions"][0]["metadata"]["virtual_address"] == "4096"
    assert report["units"][0]["metadata"]["source_path"] == "src/a.cpp"
    assert report["units"][2]["measures"]["total_functions"] == 0


def test_shared_comdat_is_counted_once():
    inv, functions, evidence = sample()
    other = copy.deepcopy(evidence["units"][0])
    other["unit"] = "b.cpp"
    evidence["units"].append(other)
    report = progress.make_report(inv, functions, evidence, {}, {})
    assert report["measures"]["matched_functions"] == 1
    assert report["measures"]["matched_code"] == "20"


def test_partial_similarity_keeps_exact_credit_and_full_denominator():
    inv, functions, evidence = sample()
    first = evidence["units"][0]
    # Two objects emit the same target function. The better score wins once.
    first["fuzzy_functions"] = [{"address": "0x1020", "size": 40, "symbol": "bar", "percent": 70}]
    second = copy.deepcopy(first)
    second["unit"] = "b.cpp"
    second["fuzzy_functions"][0]["percent"] = 80
    evidence["units"].append(second)
    report = progress.make_report(inv, functions, evidence, {}, {})
    m = report["measures"]
    assert m["matched_code"] == "20" and m["matched_functions"] == 1
    assert m["fuzzy_match_percent"] == 52  # 20 exact + 40 * .8; 40 unclaimed bytes stay zero
    assert report["units"][1]["functions"][0]["fuzzy_match_percent"] == 80
    assert report["units"][1]["metadata"]["source_path"] == "src/b.cpp"


def test_fuzzy_100_does_not_grant_exact_or_linked_credit():
    inv, functions, evidence = sample()
    evidence["units"][0]["fuzzy_functions"] = [
        {"address": "0x1020", "size": 40, "symbol": "bar", "percent": 100}
    ]
    m = progress.make_report(inv, functions, evidence, {}, {})["measures"]
    assert m["fuzzy_match_percent"] == 60 and m["matched_code"] == "20"
    assert m["complete_code"] == m["complete_data"] == "0"


@pytest.mark.parametrize("percent", [-1, 101, float("nan"), float("inf")])
def test_invalid_fuzzy_score_rejected(percent):
    inv, functions, evidence = sample()
    evidence["units"][0]["fuzzy_functions"] = [
        {"address": "0x1020", "size": 40, "symbol": "bar", "percent": percent}
    ]
    with pytest.raises(ValueError, match="fuzzy"):
        progress.make_report(inv, functions, evidence, {}, {})


def test_fuzzy_target_extent_mismatch_rejected():
    inv, functions, evidence = sample()
    evidence["units"][0]["fuzzy_functions"] = [
        {"address": "0x1020", "size": 41, "symbol": "bar", "percent": 80}
    ]
    with pytest.raises(ValueError, match="target extent"):
        progress.make_report(inv, functions, evidence, {}, {})


def test_data_union_preserves_unclaimed_bytes_and_blends_fuzzy_by_size():
    inv, functions, evidence = sample()
    inv["data_sections"] = [{"name": ".rodata", "address": 0x3000, "size": 100}]
    inv["total_data"] = 100
    section = evidence["units"][0]["sections"][0]
    section["data_ranges"] = [
        {"address": "0x3010", "size": 20, "kind": "merged string"},
        {"address": "0x3020", "size": 20, "kind": "merged string"},
        {"address": "0x3010", "size": 20, "kind": "local copy"},
    ]
    report = progress.make_report(inv, functions, evidence, {}, {})
    m = report["measures"]
    assert m["total_data"] == "100" and m["matched_data"] == "36"
    assert m["matched_data_percent"] == 36 and m["fuzzy_match_percent"] == 28
    assert sum(int(u["measures"]["total_data"]) for u in report["units"]) == 100
    assert sum(int(u["measures"]["matched_data"]) for u in report["units"]) == 36
    assert sum(int(c["measures"]["total_data"]) for c in report["categories"]) == 100
    assert m["complete_data"] == "0"


@pytest.mark.parametrize("address,size", [("0x2fff", 2), ("0x3060", 8), ("0x3010", 0)])
def test_out_of_inventory_data_proof_rejected(address, size):
    inv, functions, evidence = sample()
    inv["data_sections"] = [{"name": ".rodata", "address": 0x3000, "size": 100}]
    evidence["units"][0]["sections"][0]["data_ranges"] = [
        {"address": address, "size": size, "kind": "merged constant"}
    ]
    with pytest.raises(ValueError, match="outside"):
        progress.make_report(inv, functions, evidence, {}, {})


def test_bss_counts_memory_extent_and_requires_exact_placement():
    inv, functions, evidence = sample()
    inv["data_sections"] = [{"name": ".bss", "address": 0x3000, "size": 12, "type": "SHT_NOBITS"}]
    inv["total_data"] = 12
    section = {
        "name": ".bss",
        "address": "0x3000",
        "size": 12,
        "exact": True,
        "data_ranges": [{"address": "0x3000", "size": 12, "kind": "bss"}],
    }
    evidence["units"][0]["sections"].append(section)
    assert progress.make_report(inv, functions, evidence, {}, {})["measures"]["matched_data"] == "12"
    section["exact"] = False
    evidence["units"][0]["exact"] = False
    with pytest.raises(ValueError, match="inexact evidence"):
        progress.make_report(inv, functions, evidence, {}, {})


def test_fuzzy_capture_rejects_unpinned_binary_before_execution(tmp_path, monkeypatch):
    tool = tmp_path / "objdiff-cli"
    tool.write_text("unknown scorer")
    monkeypatch.setattr(progress.metrics.objdiff, "tool", lambda: tool)
    with pytest.raises(ValueError, match="pinned"):
        progress.metrics.fuzzy_capture(None, [], [], [], tmp_path)


def test_inexact_unit_credits_only_its_exact_functions():
    inv, functions, evidence = sample()
    unit = evidence["units"][0]
    unit["exact"] = False
    unit["sections"][0]["exact"] = False
    other = {"symbol": "_Z3barv", "address": "0x1020", "size": 40, "fde": True, "exact": False}
    unit["sections"][0]["functions"].append(other)
    report = progress.make_report(inv, functions, evidence, {}, {})
    assert report["measures"]["matched_code"] == "20"
    assert report["measures"]["matched_functions"] == 1


def test_exact_function_in_inexact_unit_still_needs_its_fde_extent():
    inv, functions, evidence = sample()
    unit = evidence["units"][0]
    unit["exact"] = False
    unit["sections"][0]["functions"][0]["size"] += 1
    with pytest.raises(ValueError):
        progress.make_report(inv, functions, evidence, {}, {})


@pytest.mark.parametrize("change", ["extent", "fde", "function", "section", "unplaced"])
def test_inconsistent_exact_evidence_fails(change):
    inv, functions, evidence = sample()
    unit = evidence["units"][0]
    section = unit["sections"][0]
    function = section["functions"][0]
    if change == "extent":
        function["size"] += 1
    if change == "fde":
        function["fde"] = False
    if change == "function":
        function["exact"] = False
    if change == "section":
        section["exact"] = False
    if change == "unplaced":
        unit["unplaced_sections"].append(".rodata")
    with pytest.raises(ValueError):
        progress.make_report(inv, functions, evidence, {}, {})


@pytest.fixture
def captured(tmp_path, monkeypatch):
    monkeypatch.setattr(builds, "ROOT", tmp_path)
    monkeypatch.setattr(builds, "REFERENCE", tmp_path / "reference")
    image = builds.Image("test", "Game", 1, "a" * 64)
    monkeypatch.setattr(
        builds,
        "load_builds",
        lambda: {"test": builds.Build("test", "1", "linux", "amd64", "target", "gcc", {"Game": image})},
    )
    monkeypatch.setattr(units, "load", lambda build: [units.Unit("a.cpp", {})])
    for name in progress.measurement_paths("test") + ["src/a.cpp", "src/a.h"]:
        p = tmp_path / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("input")
    (tmp_path / "config/test/flags.json").write_text('{"version":"gcc"}')
    inv, functions, evidence = sample()
    table = "address\tsize\tsection\n0x1000\t20\t.text\n0x1020\t40\t.text\n"
    inv.update(
        schema=progress.SCHEMA,
        build="test",
        image="Game",
        image_sha256=image.sha256,
        functions_sha256=progress.sha(table.encode()),
        total_functions=2,
    )
    dest = progress.paths("test")
    dest.mkdir(parents=True)
    (dest / "functions.tsv").write_text(table)
    progress.write_json(dest / "inventory.json", inv)
    inputs = progress.input_hashes(progress.measurement_paths("test"))
    evidence.update(
        schema=progress.SCHEMA,
        build="test",
        image_sha256=image.sha256,
        inventory_sha256=builds.sha256_file(dest / "inventory.json"),
        measurement_inputs=inputs,
        fuzzy={
            "version": "3.8.1",
            "sha256": sorted(progress.metrics.OBJDIFF_HASHES)[0],
            "config": {"functionRelocDiffs": "data_value"},
        },
        link={"status": "unavailable", "reason": "no executable link step; compiled objects only"},
    )
    unit = evidence["units"][0]
    unit["object_sha256"] = "b" * 64
    unit["fuzzy_target_sha256"] = "c" * 64
    unit["fuzzy_functions"] = [{"address": "0x1000", "size": 20, "symbol": "_Z3foov", "percent": 100}]
    unit["compilation"] = {
        "object_sha256": unit["object_sha256"],
        "compiler_version": "gcc",
        "package_manifest_sha256": inputs["toolchain/manifest.tsv"],
        "inputs": progress.input_hashes(["src/a.cpp", "src/a.h"]),
    }
    progress.write_json(dest / "evidence.json", evidence)
    return tmp_path, dest


def test_snapshot_validates_without_originals_or_objects(captured):
    inv, functions, evidence = progress.validate("test")
    assert inv["total_code"] == 100 and len(functions) == 2 and len(evidence["units"]) == 1


@pytest.mark.parametrize(
    "path",
    [
        "src/a.cpp",
        "src/a.h",
        "tools/hv/match.py",
        "config/test/units.toml",
        "config/test/flags.json",
        "uv.lock",
    ],
)
def test_changed_source_header_or_measurement_tool_rejected(captured, path):
    root, dest = captured
    (root / path).write_text("changed")
    with pytest.raises(ValueError, match="stale"):
        progress.validate("test")


def test_missing_dependency_identity_rejected(captured):
    root, dest = captured
    evidence = json.loads((dest / "evidence.json").read_text())
    del evidence["measurement_inputs"]["tools/hv/match.py"]
    progress.write_json(dest / "evidence.json", evidence)
    with pytest.raises(ValueError, match="stale"):
        progress.validate("test")


def test_tampered_inventory_rejected(captured):
    root, dest = captured
    (dest / "functions.tsv").write_text("shrunk denominator")
    with pytest.raises(ValueError, match="function inventory hash"):
        progress.validate("test")


def test_added_unit_requires_capture(captured, monkeypatch):
    monkeypatch.setattr(units, "load", lambda build: [units.Unit("a.cpp", {}), units.Unit("b.cpp", {})])
    with pytest.raises(ValueError, match="captured units differ"):
        progress.validate("test")


def test_missing_native_score_rejected(captured):
    root, dest = captured
    evidence = json.loads((dest / "evidence.json").read_text())
    evidence["units"][0]["fuzzy_functions"] = []
    progress.write_json(dest / "evidence.json", evidence)
    with pytest.raises(ValueError, match="fuzzy score inventory"):
        progress.validate("test")


def test_false_linked_claim_rejected(captured):
    root, dest = captured
    evidence = json.loads((dest / "evidence.json").read_text())
    evidence["link"]["status"] = "linked"
    progress.write_json(dest / "evidence.json", evidence)
    with pytest.raises(ValueError, match="linked evidence"):
        progress.validate("test")


def layer_config():
    return {
        "layers": {"game": {"name": "Game"}, "engine": {"name": "Engine"}, "platform": {"name": "Platform"}},
        "range": [
            {"layer": "game", "start": 0x1000, "end": 0x1020},
            {"layer": "platform", "start": 0x1020, "end": 0x1100},
        ],
        "rule": [
            {"layer": "engine", "pattern": r"5daisy5video\d+CSpritePackage"},
            {"layer": "engine", "pattern": r"^_ZNK?2ox"},
        ],
        "copies": [r"^_ZN?K?(St|9__gnu_cxx)", r"^_ZNK?2ox"],
    }


def test_layers_follow_rules_then_the_previous_named_function_then_the_range():
    functions = [
        {"address": 0x1000, "size": 8, "section": ".text"},
        {"address": 0x1020, "size": 8, "section": ".text"},
        {"address": 0x1030, "size": 8, "section": ".text"},
        {"address": 0x1040, "size": 8, "section": ".text"},
        {"address": 0x1050, "size": 8, "section": ".text"},
    ]
    names = {0x1030: "_ZN5daisy5video14CSpritePackage4loadEv", 0x1050: "_ZN5daisy5video10CVideoNull5clearEv"}
    layers = progress.assign_layers(functions, names, layer_config())
    assert layers == {
        0x1000: "game",
        0x1020: "platform",
        0x1030: "engine",
        0x1040: "engine",
        0x1050: "platform",
    }


def test_layer_categories_measure_their_functions():
    inv, functions, evidence = sample()
    report = progress.make_report(inv, functions, evidence, {}, {}, layer_config())
    categories = {c["id"]: c["measures"] for c in report["categories"]}
    assert categories["game"]["total_code"] == "20" and categories["game"]["matched_code"] == "20"
    assert categories["platform"]["total_code"] == "40" and categories["platform"]["matched_code"] == "0"
    assert categories["engine"]["total_functions"] == 0
    assert report["units"][0]["metadata"]["progress_categories"] == ["functions", "game"]


def test_functions_outside_every_layer_range_are_rejected():
    config = layer_config()
    config["range"] = config["range"][:1]
    with pytest.raises(ValueError, match="outside every layer range"):
        progress.assign_layers([{"address": 0x1020, "size": 8, "section": ".text"}], {}, config)


def test_copies_take_the_surrounding_layer_and_are_never_inherited():
    addresses = range(0x1000, 0x1060, 0x10)
    functions = [{"address": address, "size": 8, "section": ".text"} for address in addresses]
    names = {
        0x1000: "_ZN7harvest4game6CWorld6updateEv",
        0x1010: "_ZN2ox4core7CStringIcEaSERKS2_",
        0x1030: "_ZN5daisy5video14CSpritePackage4loadEv",
        0x1040: "_ZNSt6vectorIiSaIiEE13_M_insert_auxEv",
    }
    layers = progress.assign_layers(functions, names, layer_config())
    assert layers == {
        0x1000: "game",
        0x1010: "engine",
        0x1020: "platform",
        0x1030: "engine",
        0x1040: "engine",
        0x1050: "engine",
    }
