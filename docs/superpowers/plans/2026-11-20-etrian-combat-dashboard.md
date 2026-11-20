# Etrian Odyssey HD Combat Dashboard Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (or superpowers:subagent-driven-development only when the user explicitly authorizes delegation). Track each step below. This is a plan only; implementation requires the user's go-ahead.

**Goal:** Show a read-only dashboard of current-battle allies, enemies, active effects and validated hidden information, automatically returning to the map outside combat.

**Architecture:** Extend the existing title-specific module with a bounded battle snapshot reader and a separate image renderer; keep the current map and runtime artwork loader intact. Identify the correct pointers, states and units through controlled live observations before fixing the reader's schema. Expose one stable module image whose content switches between the existing map and combat dashboard; use the existing manifest bindings unless discovery shows a safer alternative.

**Tech Stack:** Existing C++20 DSMod module, Android ARM64, Unity IL2CPP metadata, existing synthetic C++ tests and isolated Eden research profile.

**Spec:** `docs/superpowers/specs/2026-11-20-etrian-combat-dashboard.md`

## Global Constraints

- Read-only guest access; no game, save, keys, firmware, extracted artwork or other proprietary files in Git or the package.
- No future/spoiler data: hidden information is allowed only for current combatants; do not expose unseen encounters, drops or AI intent.
- Preserve map behavior, supported executable build ID and companion runtime 19. No zoom, pan or follow.
- Research only in `/tmp/eden-etrian-research/user` or an isolated copy; create a fresh backup of research-profile saves before the first interaction. Never modify the user's actual profile or saves.
- Prefer runtime-loaded game icons with legible non-proprietary fallback. Do not commit research captures containing game assets.
- Never treat static metadata names as proof of live semantics. Unknown, unreadable or unverified data is omitted or explicitly unavailable.

## Review Focus

- A battle manager may remain allocated outside battle: verify transitions and only switch after a reliable battle-active signal; never rely solely on non-null pointers.
- Data can change between reads: bounds-check arrays, pointers, string lengths, IDs and numeric fields; handle incomplete or inconsistent snapshots without showing stale data.
- Effects may end, be replaced, or decrement at different turn boundaries: verify before showing numeric durations and hide them when uncertain.
- Enemies may die, spawn, repeat names or be replaced: represent current instances separately; bound counts and clear obsolete entries.
- A UI crowded by many combatants or effects must remain readable and accessible; validate on device and provide explicit overflow handling, not clipped values.

## Task 1: Discover and validate battle data

**Inputs:** The game's `dump.cs` and executable under `/tmp/eden-etrian-research/il2cpp/` and `/tmp/eden-etrian-research/update/`; existing research profile and current save backup.

**Outputs:** A local, non-proprietary evidence table of confirmed field meanings, pointer chains, value ranges, transition signals, supported build and observation scenarios. Do not write game data into the repository.

- [ ] Make a fresh backup of isolated-profile saves; verify its location and profile separation.
- [ ] Trace the `BattleManager` / `battleCtrl` / battle-character candidates and a reliable battle-active signal. Record what persists after battle, in menus and during scene changes. Validate in at least two encounters and after returning to exploration.
- [ ] Observe ally and enemy identity, side, HP/TP, party changes, identical enemies, enemy HP changes and death; distinguish current/max values and unavailable TP.
- [ ] Apply at least one buff, one debuff, one ailment and one bind; track application, replacement, expiry and turn-boundary updates. Determine duration semantics if possible.
- [ ] Investigate resistance/weakness values and other useful current-battle fields only where reproducibly verifiable. Distinguish intrinsic data from temporary modifiers and unknown values.
- [ ] Summarize confirmed, plausible and unavailable fields with exact evidence and unresolved risks. **Gate:** do not implement fields or battle detection that are not validated. If no reliable battle signal exists, stop and report the blocker instead of guessing.

## Task 2: Snapshot reader and transitions

**Files:** Modify `companions/etrian-odyssey-hd/01008970176A6000.cpp`; create a small `etrian_battle.h` only if discovery warrants it; extend `companions/etrian-odyssey-hd/etrian_module_test.cpp` and add a battle-specific test if needed.

**Interfaces:** Define a bounded internal snapshot for confirmed combatants and confirmed fields after Task 1; the existing module `sample` publishes readiness and image generation. Keep the existing supported-build check and read-only host API.

- [ ] Write synthetic tests for exploration → battle → exploration, transition states, invalid pointers and stale snapshot clearing; confirm the tests fail before implementing.
- [ ] Implement battle detection and safe snapshot acquisition using only Task 1's verified fields. Bound all guest reads and counts; invalidate unavailable fields instead of treating them as zero. Preserve current map polling.
- [ ] Write/run tests for duplicate enemy names, death, spawning/replacement, bad lengths, partial reads, and a battle manager retained outside combat.
- [ ] Confirm no guest-write host API is used and all existing map and asset tests pass.

## Task 3: Dashboard and asset fallback

**Files:** Modify `companions/etrian-odyssey-hd/01008970176A6000.cpp` and `dualscreen/manifest.json` only as required; add targeted renderer tests.

**Interfaces:** Render the validated snapshot into the current 592 × 528 surface. In normal exploration, preserve the existing `module:etrian:map/...` image behavior. Present neutral loading/unavailable state for uncertain transitions rather than stale values.

- [ ] Create failing synthetic rendering tests for battle and map selection, HP/TP availability, multiple effects and overflow. Test effect expiration and source-data changes trigger refresh.
- [ ] Render a readable ally/enemy layout using the existing color theme. Include exact enemy HP only if validated. Use status icons from the installed game where their mappings and formats are verified; otherwise use text.
- [ ] Handle more combatants/effects than fit without silently clipping: choose a small, explicitly tested manual selection or pagination interaction if needed. Do not add automatic zoom or movement.
- [ ] Generate a synthetic preview using fake data and inspect it; do not store artwork, game data, or screenshots of proprietary assets in the repository.

## Task 4: Verify and package

**Files:** Update `companions/etrian-odyssey-hd/README.md` and regenerate `01008970176A6000.dsmod.zip`.

- [ ] Run existing `etrian_map_test`, `etrian_art_test`, `etrian_module_test` and any new battle tests using C++20; validate the manifest and verify no guest writes.
- [ ] Test the state changes and several effects in the isolated profile; compare observed values to the game across turns, enemies and battle exit. Document which hidden fields and mappings are actually verified.
- [ ] Build the Android ARM64 module, create the package with runtime minimum 19, check ZIP integrity and list contents to confirm no proprietary assets.
- [ ] Ask for Android/device verification of readability, battle transitions and effect accuracy. Report unsupported or unverified fields explicitly; do not claim success based only on synthetic tests.
- [ ] Do not commit or push unless separately requested.
