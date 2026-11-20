# Etrian Odyssey HD: original combat observations

Recovered from the authored local note `combat-evidence-20261120.md`, dated 2026-11-20. Research used an isolated profile with a save backup taken before the combat experiment. This document preserves observations, including limitations and unintended research-profile changes; it is not a new round of validation. The current [maintainer guide](../etrian-companions-maintainer-guide.md) describes the shipped reader and takes precedence where implementation evolved.

**Updated build:** `4CBA9B7CE88603D196F4C63639586B8F00000000000000000000000000000000`.

## Encounters and transition leads

Two encounters were observed. That is preliminary evidence, not proof across all transitions, party compositions or game modes.

- Candidate battle manager: dereference `main+0x538f060`, then `+0x58`, `+0xb8`, `+0`. In the first encounter, its `+0x88` followed by `+0x30` held an IL2CPP array of two side arrays with lengths 6 and 8. One side contained one character and the other two. The field reset to null after returning to the dungeon, and a new battle repopulated the chain.
- The manager remained non-null on the result screen. **Non-null alone is not a complete battle-state signal.**
- Candidate scene manager: dereference `main+0x5392908`, then `+0x58`, `+0xb8`, `+0`. Its instance fields `+0x18 / +0x1c / +0x20` were `4 / 4 / 2` in the dungeon and `4 / 2 / 4` during the second battle. Metadata labels scene 2 Battle and scene 4 Dungeon. Exact field semantics and transition timing were not established by that experiment.

## HP, TP and names

- In the first battle, ally character `+0x60` led to parameter `+0x10`, then an IL2CPP array. Element 0's pair at `+0x10` held `26 / 26`, matching displayed HP; element 1 held `7 / 7`, matching displayed TP.
- The first enemy's corresponding HP pair was `19 / 19`; after defeat it became `0 / 19` and the enemy disappeared on screen. A separate second enemy record existed.
- Character `+0x28` held UTF-16 IL2CPP names for the two observed enemies.
- This did not verify enemy HP after partial damage, general roster order, different party compositions, locales, identity across encounters, or complete transition safety.

## War Cry experiment

The user activated War Cry in the isolated research battle. The ally parameter `+0x28` efficacy array, slot 0, held:

- Source skill ID: `9`.
- Efficacy ID: `14` (metadata labels attack up).
- Float at `+0x1c`: `4.0`.

After successive defend turns, the float became `3.0`, `2.0`, `1.0`, then skill/effect/float fields became zero. This supports a countdown for **this effect in this encounter**. Other effects, overlapping effects, ownership and exact turn-phase boundaries remain unverified. Static skill definitions are not evidence of active effect state.

## Research-profile side effect

Automated confirmation in the isolated skill-customization screen accidentally invested one research character skill point in Axes. The contemporaneous note states that regular user saves were not accessed by this experiment. This is recorded because input automation can change an isolated save even when the memory reader itself never writes guest memory. Preserve backups and check resolved profile/NAND paths before repeating experiments.
