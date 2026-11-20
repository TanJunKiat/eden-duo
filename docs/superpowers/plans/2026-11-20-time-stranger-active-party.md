# Time Stranger Active Party Implementation Plan

> Execute inline with executing-plans and test-driven-development. Do not commit or push without permission.

**Goal:** Produce an installable Android ARM64 ZIP showing three active Digimon names.

**Architecture:** A read-only C ABI module follows the restart-validated v1.2.2 party vector. Native label widgets display names or empty/unavailable states; no heap scan, game assets, guest calls or memory writes.

**Tech Stack:** C++20, DSMod ABI 1, runtime 19, Android NDK r28c, existing package builder.

**Spec:** `docs/superpowers/specs/2026-11-20-time-stranger-companion-design.md`, narrowed by the user's active-slot milestone; bindings in the party research note.

## Constraints and review focus

- Exact title/build checks; unsupported builds never display guessed names.
- Checked pointer arithmetic and reads; unavailable data clears stale names.
- Preserve slot order, duplicates and null entries. Do not compact slots.
- Reject invalid vector lengths and unterminated names; unsupported name encodings display unavailable rather than fabricated ASCII.
- Re-read vector metadata and slot references after sampling to reject detected transitions.
- No icons, stats, other panels, keys, firmware or proprietary assets in the ZIP.

## Tasks

- [x] Add synthetic module tests for valid names, moved addresses, swap, duplicates, null and short parties, bad pointers/counts/names, mutation during sampling, and wrong title/build.
- [x] Implement the minimal read-only module until tests pass, then run with address/undefined-behavior sanitizers.
- [x] Create the native-label manifest and README; build host and Android ARM64 modules.
- [x] Package Android ARM64 with the exact build ID and inspect archive contents and metadata.
- [x] Install a host module only into the diagnostic profile; verify the rendered three names against the game, restart and recheck.
- [x] Self-review and rerun verification; report device validation limitations and provide the ZIP path.

## Execution record

- Worked inline on main, honoring project guidance; no commit/push. No external reviewer was authorized; performed self-review rather than launching a subagent.
- Tests were added first; the first compilation failed on the missing implementation. Implementation then passed the synthetic suite, including ASan/UBSan. This initial red was a compile failure, not a per-case assertion red; do not claim exhaustive test-first behavioral proof.
- Native Android build completed without compiler errors/warnings. ELF inspection: AArch64, stripped, only libc/libm/libdl dependencies, LOAD alignment 0x4000.
- Checked shared interface: manifest `bind_text` keys exactly match module `publish_text` keys. Package title/build match module; archive module hash verified.
- Host module loaded through the real DSMod loader. A temporary out-of-tree platform-selection patch enabled macOS testing; it does not ship. Rendered labels matched the initial active members and live Setup swaps, including duplicate Botamon slots. After restart, labels matched the save again using relocated addresses.
- Self-review covered checked address arithmetic, bounded name reads, count validation, exact build rejection, no write API use, stale-data clearing, slot identity and transition detection. No blocking findings; physical-device installation and battle transitions remain validation limitations, not claimed results.
- Ruling: ship the scoped names-only experimental package, not the broader artwork/stats design — only active-name bindings are established — richer panels remain future work.
- Ruling: use the existing external packaging tool rather than duplicate it in this repo — matches existing companions — rebuilding requires that tool alongside the NDK.
- Ran project-wide CTest: `cli_version` and `cli_help` passed; `tests`, `tsan_redraw_harness`, `dsmod_scroll_test`, and `dsmod_min_runtime_test` were Not Run because their executables are absent from the existing build. CTest therefore reports 4/6 failed (missing binaries), not a passing full regression suite. Standalone companion tests pass under ASan/UBSan, and real emulator integration passed. LSP confirmed one new C++ file clean, timed out on the other; both compiled with warnings-as-errors.
- Deliverable: `companions/time-stranger/dist/010062E01FE0C000.dsmod.zip` (0.1.0), Android ARM64/API28+, companion runtime19+.
