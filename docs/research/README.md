# Game research archive

This index consolidates the original research results currently recovered from this workspace and its isolated local research directories. It covers **Etrian Odyssey HD, Etrian Odyssey II HD, Etrian Odyssey III HD, and Digimon Story: Time Stranger**. It does not claim to archive work for other games merely because their ROMs or companion packages exist locally. No new gameplay experiments were performed for this archival pass.

## Start here

| Game | Findings and implementation | Evidence boundary / remaining work |
|---|---|---|
| Etrian Odyssey HD | [Maintainer guide](../etrian-companions-maintainer-guide.md), [skill research](../superpowers/specs/2026-11-20-etrian-skill-research.md), [original combat observations](etrian-odyssey-hd-combat-observations.md), [source and package](../../companions/etrian-odyssey-hd/) | Recorded dungeon map, runtime artwork and battle reader; observed HP/TP and one enemy defeat. Only War Cry's effect/countdown was followed through turns. General effects, localization, formation order and device/transition coverage are incomplete. |
| Etrian Odyssey II HD | [Maintainer guide](../etrian-companions-maintainer-guide.md), [II/III design](../superpowers/specs/2026-11-20-etrian-2-3-companions.md), [source and package](../../companions/etrian-odyssey-ii-hd/) | Title-specific map/art/battle bindings. One dungeon/battle comparison included ally 45 HP / 20 TP and two enemy records. General effect duration, full parties and broader transitions are unverified. |
| Etrian Odyssey III HD | [Maintainer guide](../etrian-companions-maintainer-guide.md), [II/III design](../superpowers/specs/2026-11-20-etrian-2-3-companions.md), [source and package](../../companions/etrian-odyssey-iii-hd/) | Different battle engine and UnityFS artwork layout. One dungeon/battle comparison included ally 44 HP / 18 TP and two enemy records. Thirty decoded texture payloads matched independent private decoding. Enemy names/maxima, broader icon semantics and sea map remain unresolved/unsupported. |
| Digimon Story: Time Stranger | [Chronological memory research and current binding](../superpowers/specs/2026-11-20-time-stranger-party-research.md), [design](../superpowers/specs/2026-11-20-time-stranger-companion-design.md), [implementation/verification record](../superpowers/plans/2026-11-20-time-stranger-active-party.md), [source and package](../../companions/time-stranger/) | Ordered party root validated across restarts, active/reserve swaps and duplicate names. Native labels matched live Setup changes and a restarted save. Names-only release; battle transitions, portraits, other panels and physical Android installation unverified. |

The linked maintainer/research documents contain exact title/build IDs, pointer chains, field offsets, runtime asset paths, observed values, rejected leads and reproduction commands. Use their **current-result** sections before older chronological notes: earlier unresolved hypotheses are deliberately preserved, not silently promoted to facts. Plans describe intent; they are not independent evidence of completion.

## Additional recovered material

- [Original Etrian I combat observations](etrian-odyssey-hd-combat-observations.md): preserves extra transition observations, two encounters, scene-field values, and the isolated-profile accidental skill-point expenditure. The main maintainer guide already contained the useful bindings and War Cry countdown; this is provenance, not a new discovery.
- [Time Stranger diagnostic method and command protocol](time-stranger-diagnostic-method.md): preserves mapped-memory search filters, capture formats, command caps, the controlled-change sequence and rejected transient-copy leads without committing snapshots or the whole developer-console fork.
- [Offline pointer-root tracer](../../tools/research/README.md): original authored C++ tool used on a private pointer-edge snapshot to identify Time Stranger's durable module-relative party chain. Its limitations are explicit; a reported chain is a candidate until validated in the live game.
- The temporary Etrian II artwork checker selected an assets file from the first game's research directory while taking `.resS` from II's base extraction. That mixed-input helper is **not reliable standalone validation of II's updated artwork** and is not copied as a reproducible tool. Use the title-specific reader/tests and the documented live evidence instead.
- The temporary Etrian III artwork checker loaded the private patched UnityFS bundle with the production reader and wrote decoded RGBA payloads for comparison. Its useful results and bounds are already recorded in the maintainer guide; the extracted bundle and output images are intentionally not archived.

## Reproduction and verification

From the repository root, synthetic tests require no game content:

```sh
set -e
for d in etrian-odyssey-hd etrian-odyssey-ii-hd etrian-odyssey-iii-hd; do
  for t in map art battle module; do
    clang++ -std=c++20 -I src "companions/$d/etrian_${t}_test.cpp" -o "/tmp/$d-$t-test"
    "/tmp/$d-$t-test"
  done
done
clang++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src companions/time-stranger/party_test.cpp -o /tmp/time-party-test
/tmp/time-party-test
```

During this archival pass, all twelve Etrian map/art/battle/module test executables passed, the Time Stranger test passed under ASan/UBSan, and the archived tracer compiled with warnings-as-errors and passed its synthetic path-convention smoke check.

These checks exercise implementation logic, not the Android installer or all game states. Live research requires the exact supported build, a separate profile with independently checked save paths, backups, read-only guest-memory access, and screen corroboration. See each companion README for packaging and install requirements.

The Time Stranger implementation pass also attempted the existing emulator CTest suite: `cli_version` and `cli_help` passed; `tests`, `tsan_redraw_harness`, `dsmod_scroll_test`, and `dsmod_min_runtime_test` could not run because their executables were absent. This is not a passing full emulator regression suite.

## What is intentionally not committed

ROMs/NSPs, firmware, keys, regular or research saves, raw memory/pointer snapshots, IL2CPP dumps, extracted executables/metadata, localized game tables, textures, gameplay screenshots and private emulator logs. They can contain proprietary content or private state; committing a directory wholesale is not a safe archival method. The original research code, independently written tests, factual findings and reproducible procedures are the shareable record.

Temporary emulator console forks and machine-specific build/link scripts remain private scratch artifacts. The stable party reader is preserved in the production Time Stranger module; the memory-scan methods and false-negative diagnosis are recorded in its research note. The full production Etrian readers preserve their respective validated decoding logic. Local `/tmp` paths in older notes are provenance only and must not become runtime dependencies.
