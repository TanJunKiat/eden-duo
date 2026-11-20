# Etrian Odyssey HD experimental companion

This directory contains the experimental read-only map companion and its Android ARM64 package, `01008970176A6000.dsmod.zip`. Install it in Eden Duo via the game's **Add-ons → Install → Dual screen mods**. It requires companion runtime 19 or later and the updated game executable with build ID `4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000`.

The companion reads the current floor's saved map and does not write to the game. It shows coordinate labels, the floor number, a facing arrow, and saved map icons. It attempts to read the player artwork and artwork for all 28 known saved map icon types from the installed game's RomFS at runtime, including streamed Switch textures; unavailable artwork uses built-in approximations. The icon reader checks the expected texture names and layout and fails closed if they differ. The first cut was observed by a player showing floors and walls; the updated visuals, icon meanings, facing, floor changes and reloads still need device validation. Live moving FOEs are not yet displayed (a placed FOE marker uses game artwork when available). Drawing and memo text are not implemented.

The complete recorded map stays fixed in view, with game-inspired coordinate tabs and floor indicator. There are no zoom, pan or follow controls.

During a detected battle the experimental dashboard replaces the map with blue combatant cards, with enemies above allies. Cards show candidate current/max HP, ally TP when available, and the observed War Cry countdown. Cards preserve the order of occupied roster-array entries, but whether this matches the game's visual formation order has not been verified, particularly for gaps, defeated combatants and summons. Other effects and durations are not yet supported and are explicitly labelled unknown; multiple stacked skills and localized skill-name lookup are still being researched. Battle detection, enemy HP, combatant identity and countdown handling need Android testing, particularly for full parties, bosses, summons, different languages and battle transitions. Unreadable or incomplete battle data shows an unavailable screen rather than stale values. Non-ASCII characters in names currently appear as `?`.

## Tests

From the repository root, with a C++20 compiler:

```sh
clang++ -std=c++20 companions/etrian-odyssey-hd/etrian_map_test.cpp -o /tmp/etrian-map-test
/tmp/etrian-map-test
clang++ -std=c++20 -I src companions/etrian-odyssey-hd/etrian_art_test.cpp -o /tmp/etrian-art-test
/tmp/etrian-art-test
clang++ -std=c++20 -I src companions/etrian-odyssey-hd/etrian_module_test.cpp -o /tmp/etrian-module-test
/tmp/etrian-module-test
clang++ -std=c++20 companions/etrian-odyssey-hd/etrian_battle_test.cpp -o /tmp/etrian-battle-test
/tmp/etrian-battle-test
```

The module test uses synthetic guest memory, not the game or an Android device.

## Rebuild the package

Use Android NDK r28c and the public [Eden Duo Companions](https://github.com/igawa6/eden-duo-companions) packaging tool (the saved package was built with commit `2169928d2f301aa7bb3b13777ca8a94d0e73961d`). Set `NDK_HOST` to the NDK's `toolchains/llvm/prebuilt/<host>` directory and `COMPANIONS` to a checkout of that repository. From this repository's root:

```sh
"$NDK_HOST/bin/aarch64-linux-android24-clang++" -std=c++20 -O2 -shared -fPIC -static-libstdc++ \
  -I src companions/etrian-odyssey-hd/01008970176A6000.cpp -o /tmp/01008970176A6000.so
"$NDK_HOST/bin/llvm-strip" --strip-all /tmp/01008970176A6000.so
python3 "$COMPANIONS/tools/build_dualscreen_package.py" \
  --package companions/etrian-odyssey-hd --output /tmp/etrian-dist --version 0.7.2 \
  --module android-arm64-v8a=/tmp/01008970176A6000.so \
  --build-id 4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000 \
  --min-runtime 19
```

No game files, keys, firmware, extracted assets, or saves are needed in this directory or included in the package.
