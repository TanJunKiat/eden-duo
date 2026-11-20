# Time Stranger — Active Digimon (experimental)

Read-only Eden Duo companion showing the **three active party slots by name**. Reserves remain distinct even when names/species repeat. Missing slots show `Empty`; unreadable data shows `Unavailable`. Before the module supplies text, labels show `-`. Unsupported builds are rejected by the module loader.

## Install

1. Use an Eden Duo build with companion runtime **19 or newer**, on **Android ARM64** (Android API 28 / Android 9 minimum).
2. Install your own legally obtained Digimon Story: Time Stranger and the tested **v1.2.2 update**. Title: `010062E01FE0C000`.
3. In Eden Duo, open the game's **Add-ons → Install → Dual screen mods**, then select `010062E01FE0C000.dsmod.zip`. Enable the installed companion if needed and restart the game.
4. Load your save. The companion should list the three active Digimon. Move a reserve into an active slot through the game's Setup screen to check that the name changes.

The exact supported main build is:

```text
135C83CDEA06AEC2D858E1BC7D53D72F7257166C000000000000000000000000
```

Package filenames are title IDs, not update title IDs. Do not extract the `.so` manually or install it as a cheat.

## Scope and limitations

- Names only: no portraits, HP/SP, map, farm, evolution or quest panel yet.
- Non-ASCII/custom name encodings are not established; this version conservatively displays `Unavailable` for a name outside printable ASCII or without a terminator in 64 bytes. It does not invent or transliterate names.
- Pointer resolution was validated in isolated macOS emulator research across reserve swapping, menu/exploration and two process restarts. The native module/labels are tested separately in the diagnostic emulator. **The Android build has not been tested on a physical device.**
- In-battle changes, guests and partially filled parties need live validation. Synthetic tests cover empty/short rosters but are not proof of all game layouts.
- No writes to guest memory, guest calls, save reads, cheats, game assets, keys or firmware. Memory reads are bounded; detected roster transitions clear the whole displayed snapshot. These checks cannot guarantee an atomic snapshot of an asynchronously changing guest.
- `Unavailable` during loading is expected. If it persists, check the enabled update and build ID; do not try to fix it by modifying a save.

## Build and test

From the repository root (C++20 compiler required):

```sh
clang++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src companions/time-stranger/party_test.cpp -o /tmp/time-party-test
/tmp/time-party-test
```

Build/package with NDK r28c and the existing Eden Duo companions packaging tool:

```sh
ANDROID_NDK_HOME=/path/to/android-ndk-r28c \
  bash companions/time-stranger/build.sh \
  /path/to/eden-duo-companions/tools/build_dualscreen_package.py
```

The script outputs the ZIP in `companions/time-stranger/dist/`; source, tests and private research artifacts are not bundled. The ELF uses static libc++ and 16-KiB-compatible load-segment alignment. See the research note under `docs/superpowers/specs/2026-11-20-time-stranger-party-research.md` for the build-specific pointer chain.
