# decomp.dev progress pipeline

`just progress-capture` compiles every recovered unit against the pinned original in the local
Docker toolchain, measures native objdiff similarity, and saves measurements under
`reference/1.18-linux-amd64/progress/`. Install the pinned scorer with `just objdiff-cli` first. Commit the
three generated files with the source change. `just progress` validates those measurements against
the current checkout and writes `build/progress/report.json` without needing originals or Docker.

The GitHub Actions workflow `.github/workflows/progress.yml` runs tests, checks measurement
freshness, validates the JSON with the pinned official objdiff v3.8.1 parser, and uploads
`1.18-linux-amd64_report/report.json`. This matches decomp.dev's artifact ingestion contract.
It runs on default-branch pushes, pull requests, manual dispatch, and the setup branch.

## Measurement contract

- **Denominator:** all 1,998,094 bytes in the original's allocated executable sections: `.init`,
  `.plt`, `.text`, and `.fini`. This deliberately includes linker/runtime code and padding.
- **Function inventory:** 5,240 nonoverlapping function extents totaling 1,948,609 bytes: 4,976
  `.eh_frame` FDE ranges (1,946,920 bytes) and 264 thunks (1,689 bytes) from `config/<build>/extents.tsv`,
  which GCC 4.4 emits without an FDE. The remaining 49,485 bytes are retained as unclaimed code, not
  invented functions or matches.
- **Matching credit:** full inventoried function bodies that the matcher found exact at their own
  target address: every byte and every resolved reference equal, and the extent equal to the
  function's inventoried extent (its FDE or thunk extent). A function earns credit even when its unit as a whole does not match yet (for
  example because GCC ordered the unit's functions differently); an exact unit must be exact in
  every section and function. No fuzzy/normalized similarity earns exact matching credit.
- **Deduplication:** each original address/range earns credit once. Shared inline/COMDAT bodies
  emitted by multiple recovered units do not inflate the numerator.
- **Fuzzy code:** pinned objdiff v3.8.1 compares fresh compiled objects with independently delinked
  target objects, using `functionRelocDiffs=data_value`. Each score is weighted by the original function
  extent, even when the candidate function has a different size. Duplicate target addresses use the
  best measured score once; unrecovered functions and unclaimed executable bytes contribute zero.
  Exact matcher proofs contribute 100; an objdiff score of 100 alone does not grant exact credit.
- **Data denominator:** all allocated non-executable ELF sections, including `.rodata`, `.data`,
  BSS, unwind/exception tables and linker metadata. The initial data inventory is 573,002 bytes.
  BSS contributes its zero-initialized memory extent, not file payload bytes. This does not include
  external assets or bundled shared libraries.
- **Matched data:** complete placed data sections with exact bytes and resolved references;
  correctly placed BSS extents; complete referenced merge elements/terminated strings; and verified
  read-only local copies. Target intervals are unioned so shared vtables, strings, suffixes and
  overlapping copies count once. Unplaced, skipped and inexact sections earn no section credit.
- **Fuzzy overall:** `(sum(original function size * score / 100) + matched data bytes) /
  (total code bytes + total data bytes) * 100`. Data receives exact-only credit; partial code scores
  come from native objdiff. This changes the meaning of the old fuzzy history, which was identical
  to exact matched-code percentage with a code-only denominator. Matched Code remains comparable.
- **Linked code/data:** `complete_code`, `complete_data` and `complete_units` remain zero. Capture
  records an explicit unavailable link status because there is no executable link step. Compiling
  an object or matching a function does not establish that it is included in a rebuilt executable.
- **Display:** one treemap unit per function extent, with readable Mac-derived names when available, plus four
  unclaimed-code units and matched/unclaimed data intervals. The **Allocated data** category isolates
  data progress. Leave the site's default category as **All** to retain the full denominator.
- **Port-relevance layers:** `config/<build>/layers.toml` puts every function into one more category, so
  the code a modern port keeps can be followed apart from code it would replace. **Game logic**
  is the `harvest` objects; **Engine core** is the `ox` library plus the daisy pieces that define
  data formats or game behaviour (sprite and particle packages, fonts, the sound logic in
  `CAudioDriver`); **GUI toolkit** is the other `daisy::gui` widgets, whose interfaces the game uses
  but whose drawing a port redoes; **Platform layer** is the rest of daisy (OpenGL and software
  renderers, scene graph and mesh loaders, device, input, file system, network and OpenAL backends).
  A rule matches a symbol name; unnamed functions take the layer of the closest earlier named function
  in their address range. Template and inline copies a unit emits beside its own code (STL
  instantiations, `ox` interface inlines) take the layer of the code around them unless a rule names
  one (`ox` code is engine wherever it is emitted), and unnamed functions never inherit from a copy.
  The layers do not change the whole-binary measures.

The initial capture at the `CMemReadFile` + `CMemWriteFile` stage has 35 unique matched functions
and 995 matched bytes: **0.04980%**. These two objects emit 37 function records, including two shared
bodies. The snapshot is metadata only; proprietary executable and compiled-object bytes are not
committed or uploaded by the workflow.

## Freshness and proof limits

`inventory.json` pins the target image, section sizes/hashes and function-table hash.
`functions.tsv` contains every function extent and its source (`fde` or `thunk`); the **Thunks**
category reports the thunks apart. `evidence.json` contains the whole-object matcher
results, object hashes, compiler/image identity, dependency hashes from GCC's depfile, toolchain
manifest hash, and hashes of all measurement code/configuration inputs. Schema 2 also records native
per-function scores, the scorer binary/configuration, delinked object hashes, exact data ranges and
link status. Capture uses its own local objdiff project; it does not change the root `objdiff.json`
or reuse stale interactive objects. Capture checks for source and measurement changes during compilation.
Schema 3 adds each function's extent source to `functions.tsv` and to the matcher's function rows.

CI verifies these identities, score extents, data bounds and the evidence's internal consistency.
**It does not recompile or recheck the proprietary original.** It publishes the recorded local measurement
only when its
source/header, compiler configuration, matcher, inventory, and unit list still agree with the
checkout. This is not a signed attestation and should be reviewed like other generated evidence.

The workflow's report self-comparison only exercises the official objdiff parser;
it is not a before/after measurement or a recompilation proof. The bot compares
the reports associated with its displayed base and head commits. Its "new
matches" count includes both function rows and matched-data intervals, so it can
exceed the number of new code functions. Compare code/data byte deltas and the
reported commit range, not an older PR body's pre-integration project totals.

After editing recovered code, headers, `units.toml`, `symbols.tsv`, compiler flags, dependencies,
or measurement code, run:

```sh
just progress-capture
just progress
uv run pytest -q
```

Then commit the updated snapshot with the change. A stale snapshot fails CI; it never silently
reuses old matching credit or shrinks the denominator to the recovered subset.

## Site registration

The public project is registered at [decomp.dev/banteg/harvest](https://decomp.dev/banteg/harvest).
Its default version is `1.18-linux-amd64`, workflow is `progress.yml`, and category is **All**.
The site currently offers Windows as its only PC platform category; the report version identifies
the Linux target explicitly. Directory listing requires at least 0.5% matched code.

The existing decomp.dev GitHub App installation includes `banteg/harvest` using selected-repository
access, alongside the existing Crimsonland and Snail Mail repositories. Workflow completion events
trigger report ingestion. Automatic pull-request comments are enabled to show progress changes on each PR.

Registration imported the successful `master` push report at `910cad47a09e029936700d51b416214af1926e96`:
995 / 1,998,094 matched code bytes (0.04980%). Subsequent matching changes must refresh the snapshot
as described above before pushing. A successful default-branch push publishes the next measurement.

Upstream contracts checked against:

- [decomp.dev ingestion and workflow discovery](https://github.com/encounter/decomp.dev/blob/e9c086adb74d2fe569541cd715cd9312d7641313/crates/github/src/lib.rs#L460)
- [Registration UI](https://github.com/encounter/decomp.dev/blob/e9c086adb74d2fe569541cd715cd9312d7641313/crates/web/src/handlers/manage.rs#L194)
- [objdiff report schema](https://github.com/encounter/objdiff/blob/fba10a617154f19b3fc25c8817dc81f81d8489b5/objdiff-core/protos/report.proto)

The workflow's Actions are pinned by commit. Its objdiff executable is pinned by release and
SHA-256, and parsed with `objdiff-cli report changes report.json report.json` before upload.
