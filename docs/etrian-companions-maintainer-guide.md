# Etrian Odyssey HD companions: maintainer handoff

This is a practical handoff for continuing the three **experimental, read-only** Etrian Odyssey HD companions. It records the implementation and the evidence available when the EO III artwork reader was finished; code and later test results take precedence over this snapshot. See each companion's README for installation and current limitations, and [skill research](superpowers/specs/2026-11-20-etrian-skill-research.md) for the first game's skill leads.

## Ground rules

- Never write guest memory or modify the player's regular saves. Research in a separate emulator profile with an independently verified NAND/save path; back up research saves before experiments. A copied research config previously pointed to a *different isolated* NAND, so check the actual resolved paths rather than trusting the config filename. Never point research at the regular profile.
- The map is a **fixed view of recorded exploration**: no zoom, pan, follow, editing, undiscovered map reveal, future encounters, drops or AI intent. III's sea map is not supported.
- Obtain artwork and, where feasible, localized text from the installed game at runtime. Do not commit or package proprietary games, extracted game images/text, keys, firmware or saves. Small built-in fallback drawings and independently written synthetic fixtures are fine.
- Do not infer cross-title memory layouts or effects. Validate each updated build and each mechanic independently. Display unknown or unavailable rather than claiming absent or inventing durations. Preserve occupied roster-array order unless actual formation ordering is verified.
- Work on `main` as previously requested. Ask before committing or pushing new work; an earlier authorization to push is not a blanket authorization for future changes.

## Packages and entry points

| Game | Directory under `companions/` | Title ID | Updated build ID | Package version |
|---|---|---|---|---|
| I | `etrian-odyssey-hd` | `01008970176A6000` | `4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000` | 0.7.2 |
| II | `etrian-odyssey-ii-hd` | `0100B810176A8000` | `8883310FF46D85D17C09882B36A8205D00000000000000000000000000000000` | 0.1.0 |
| III | `etrian-odyssey-iii-hd` | `010084A0176AA000` | `3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000` | 0.1.1 |

All packages require runtime 19 and contain an Android ARM64 module only. Each title directory has a `<title-id>.cpp` ABI entry point and renderer; `etrian_map.h` map helpers; `etrian_battle.h` battle reader; `etrian_art.h` runtime artwork loader; a `dualscreen/manifest.json`; four corresponding `*_test.cpp` synthetic tests; a README; and an installable `<title-id>.dsmod.zip`. The code was copied into title-specific directories: changes in one do **not** automatically update the others. Shared DSMod ABI definitions are in `src/core/mods/dsmod_module_abi.h` and `dsmod_module_extensions.h`.

Important commits: `67d2982f2e` first-game battle layout; `0766954a8f` II/III ports; `fb7273cfb1` III UnityFS artwork. History and the `docs/superpowers/` design/plan files preserve earlier reasoning; do not mistake original plans for proof that every validation was completed.

## Runtime architecture

The module's `supports` checks the exact build ID. `sample()` reads the scene and battle state first; battle or loading produces a battle image in place of the map. In map mode it validates pointers, floor, coordinates, direction and array lengths before publishing `map.ready` and a generation-keyed `module:etrian:map/<generation>` image. The image callback draws a fixed RGBA map or the battle cards. Mutex-protected state and generation changes avoid serving the wrong image. Inspect the per-title code for exact bounds and behavior; do not assume unknown scene behavior is identical between titles.

Battle cards put enemies above allies, in centered rows of at most three and up to eight per side. Each occupied slot is kept in roster order (not proven visual formation order). Incomplete reads display unavailable; supported player TP is shown when readable. Non-ASCII names are currently replaced by `?`. I and II classify scene kind `2` as battle and other readable values as map; III recognizes dungeon `4` and battle `5`, and treats other/unreadable scenes as unavailable rather than showing a stale dungeon map. I/II's `unknown` fallback depends on the prior mode; verify transitions before changing it.

### Recorded map layout

The common model is 35 × 30 cells, `index = y*35+x`, rendered with `screen_y = 29-y`. `etrian_map.h` uses bit `8` as the recorded-floor flag, bits `64` and `128` for floor colors, and `2`/`4` for right/lower walls. These meanings and copied map/icon assumptions still require broader per-game visual validation. Map icon type → artwork mappings are in each `etrian_art.h`; player and placed icons have fallback drawings. They are not live FOE tracking.

All three current map bindings dereference the method slot at `main + slot`, then `+0x18`, `+0xc0`, `+0`, `+0xb8`, `+0` to reach the manager. The remaining current bindings are:

| Game | Slot | Floor | X / Y | Direction | Floor array |
|---|---|---|---|---|---|
| I | `0x53a70d0` | `+0xd0` | `+0x68 / +0x6c` | `+0x78` | `+0xd8` |
| II | `0x55d5ca0` | `+0xd0` | `+0x68 / +0x6c` | `+0x78` | `+0xd8` |
| III | `0x61a60b8` | `+0xdc` | `+0x70 / +0x74` | `+0x80` | `+0xe8` |

For each floor, the current code indexes array entries with stride `0x28`, follows entry `+0x20` to the tile list and entry `+0x08` to icons, checks lengths, then copies 1,050 entries. These are build-specific implementation bindings, not guarantees for other updates or game states.

### Battle bindings and evidence

The battle singleton helper follows `main + type-info slot → +0x58 → +0xb8 → +0`. Inspect `etrian_battle.h` for range checks and full pointer chains.

| Game | Scene slot / field | Battle slot | Notable implementation bindings |
|---|---|---|---|
| I | `0x5392908 / +0x1c` | `0x538f060` | manager `+0x88` → controller `+0x30` side arrays; character name `+0x28`, parameter `+0x60`, values `+0x10`, effects `+0x28` |
| II | `0x55c07f0 / +0x1c` | `0x55bcef0` | manager `+0x88` → controller `+0x38` side arrays; character name `+0x28`, parameter `+0x58`, values `+0x10` |
| III | `0x618e708 / +0x44` | `0x618ace8` | manager `+0xb8` → packet `+0x88` party array → party `+0x18` units → unit `+0x340` character data; data `+0x50/+0x52` are current HP/TP (`int16`), `+0x48` ability data (max HP/TP at `+0x10/+0x14`), name `+0x58` |

The code interprets side 0 as allies, side 1 as enemies. II's controller offset is **not** I's. III is a different battle engine; its observed enemy name was empty, so empty enemy names become `ENEMY <slot>` and an empty ally name makes battle data unavailable. Do not assume empty means the game has no name.

Observed, not exhaustively proven:

- **I:** player HP/TP candidates matched 26/7; one enemy HP changed 19/19 → 0/19 on defeat. War Cry had skill ID 9, efficacy ID 14 and observed countdown 4 → 3 → 2 → 1 → cleared. This is the **only** exposed effect/countdown. A static skill catalogue is not evidence of an active effect. General localization and timings remain unresolved.
- **II:** in one isolated dungeon/battle the position/floor matched the screen, a displayed ally had 45 HP / 20 TP, and two enemy records were found. No II effect duration is claimed.
- **III:** in one isolated dungeon/battle the position/floor matched the screen, a displayed ally had 44 HP / 18 TP, and two enemy records were found. Maxima, enemy HP after damage and general effect semantics remain insufficiently validated.
- The user reported the original fixed map floors/walls working and later reported the III artwork fix works; the latter report did not specify the device or validate every icon. Do not generalize these observations into comprehensive device/formation/transition coverage.

## Runtime artwork

I and II read expected Unity texture objects and `sharedassets1.assets.resS` from installed RomFS using `read_romfs`. They check names, sizes, formats and offsets; streamed 64×64 textures use the implemented Switch block-linear transform. II has its own offsets and accepts the observed `+0x30` update shift only when metadata validates. If data is missing or incompatible, the renderer uses fallback art. Never reuse I's object offsets for II without validation.

III uses `romfs:/Data/data.unity3d`, a UnityFS bundle, not the separate shared-assets layout. Its loader bounds and parses the bundle header, LZ4/raw block table and directory, reads only needed blocks, locates `resources.assets` and `resources.assets.resS`, validates title-specific texture object names/metadata and decodes inline RGBA or streamed block-linear textures. It has fallback art if any needed piece is unsupported or invalid. It is intentionally a bounded reader for the observed bundle, not a general-purpose Unity asset parser. Check its parsing limits and malformed-input tests before expanding it.

During the fix, the patched runtime bundle was compared with independently decoded textures: all 30 supported texture payloads matched byte-for-byte after accounting for vertical image orientation (18 regular inline icons, player, 10 streamed map/inactive icons, and the FOE marker). Synthetic tests cover inline/streamed extraction, LZ4, malformed metadata and truncated input; they do **not** prove the mapping of every saved symbol to its corresponding artwork. No extracted images are committed.

## Reproduce tests

From the repository root, using a C++20 compiler; none require game files:

```sh
set -e
for d in etrian-odyssey-hd etrian-odyssey-ii-hd etrian-odyssey-iii-hd; do
  for t in map art battle module; do
    clang++ -std=c++20 -I src "companions/$d/etrian_${t}_test.cpp" -o "/tmp/$d-$t-test"
    "/tmp/$d-$t-test"
  done
done
# Optional memory/undefined-behavior check for III's bundle parser:
clang++ -std=c++20 -fsanitize=address,undefined -fno-omit-frame-pointer \
  companions/etrian-odyssey-iii-hd/etrian_art_test.cpp -o /tmp/eo3-art-sanitize
/tmp/eo3-art-sanitize
```

These tests use synthetic data, not Android or a live game. The image comparison above additionally used an isolated patched RomFS and UnityPy; both the installed content and temporary extracted evidence must remain private and untracked.

## Build and inspect an Android package

Requires Android NDK r28c and the [Eden Duo Companions](https://github.com/igawa6/eden-duo-companions) packaging tool (I's README notes the tool commit used). Set `NDK_HOST` to `toolchains/llvm/prebuilt/<host>` within the NDK and `COMPANIONS` to that checkout. Example for III; substitute the exact directory, title ID, build ID and appropriate new package version for I/II:

```sh
D=companions/etrian-odyssey-iii-hd
T=010084A0176AA000
BUILD=3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000
VERSION=0.1.1
"$NDK_HOST/bin/aarch64-linux-android24-clang++" -std=c++20 -O2 -shared -fPIC -static-libstdc++ \
  -I src "$D/$T.cpp" -o "/tmp/$T.so"
"$NDK_HOST/bin/llvm-strip" --strip-all "/tmp/$T.so"
python3 "$COMPANIONS/tools/build_dualscreen_package.py" --package "$D" \
  --output /tmp/etrian-dist --version "$VERSION" \
  --module "android-arm64-v8a=/tmp/$T.so" --build-id "$BUILD" --min-runtime 19
unzip -t "/tmp/etrian-dist/$T.dsmod.zip"
unzip -l "/tmp/etrian-dist/$T.dsmod.zip"
# After inspection, copy the ZIP to the title directory if it is intended for distribution.
```

Inspect `package.json` and `dualscreen/manifest.json` in the ZIP for title, version, runtime, correct full build ID and `android-arm64-v8a` module. The expected archive contains manifest, module and package metadata only—not proprietary data. Install via Eden Duo's game **Add-ons → Install → Dual screen mods**. A valid ZIP and a successful cross-compile do not replace on-device testing.

## Continuing research safely

1. Record the title, full running build ID, scene and visible screen before reading memory. Set up a new isolated emulator profile, confirm its resolved NAND/save paths, make a backup, and never use the regular save for experiments. Use the installed patched runtime RomFS when testing an update: a standalone update may be a delta and can fail to extract independently.
2. Use updated IL2CPP metadata to identify type/field/method candidates; distinguish metadata from live observations. Cross-check pointer chains with bounded, read-only memory reads and controlled before/after screen changes. A single matching value is a lead, not proof. Validate unrelated states and transitions, including a failed read.
3. For effects, observe activation, multiple turns, expiry, stacking and a second skill. Check identity, owner, duration and turn boundary before displaying a name or number. For enemy HP, compare non-lethal damage, defeat and different enemies. For ordering, test gaps, deaths, summons and full parties.
4. For artwork, compare validated decoded payloads against the installed title in private, then test each saved map-icon type visually. Do not commit texture bytes, localized game tables, screenshots containing proprietary data, saves or dumps. Treat Unity offsets as update-specific and fail closed on metadata mismatches.
5. Add synthetic regression tests and verify existing titles before packaging. Document precisely which states were live-tested, which were only synthetic, and what remains unknown. Ask the user for an Android check rather than claiming one.

Earlier research used temporary files under `/tmp/eden-etrian-research`, `/tmp/eden-eoII-research` and `/tmp/eden-eoIII-research`, including isolated backups, `il2cpp/dump.cs`, `script.json` and custom research executables. These are **volatile local artifacts**, not repository dependencies or a reproducible setup; they may no longer exist and can contain sensitive/proprietary data. Do not rely on their absolute paths or commit them. IL2CPP dumps generated from the patched runtime metadata and UnityPy inspection were used as research aids, not bundled into the companion.

## Priority follow-ups

- Verify II/III floor flags and individual icon semantics, floor changes, reloads and scene transitions; test full parties, gaps, bosses and summons.
- Verify battle HP/TP and maxima, especially non-lethal enemy damage; resolve III enemy display names from the running game without packaging text.
- Extend active effects only after title-specific observation; research I's runtime localized skill lookup described in the linked skill-research note. Never carry I's War Cry assumptions into II/III.
- Check non-ASCII names and language selection; validate actual card formation order rather than sorting by guesswork.
- Device-test Android install, update/build gating, startup, rotation, map/battle switching, memory/performance and fallback behavior. III's sea map, map editing, memo text and live FOE tracking are not implemented.
