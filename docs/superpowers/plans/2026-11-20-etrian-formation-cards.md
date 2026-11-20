# Etrian Formation Cards Implementation Plan

> Execute inline using TDD; do not commit unless requested.

**Goal:** Show enemies above allies in reference-inspired blue panels while preserving bounded roster order.

**Architecture:** Retain the battle snapshot reader and render its existing ordered vectors into centered rows of at most three cards per half. No new game-asset dependency. Do not infer a formation sort from unverified fields.

**Tech Stack:** C++20 companion renderer, Android ARM64, runtime 19.

**Spec:** `docs/superpowers/specs/2026-11-20-etrian-formation-cards.md`

## Global constraints

Read-only; no game content packaged; fixed map unchanged; unverifiable effects remain unknown; build ID and runtime unchanged.

## Review focus

- Up to eight actors on each side must remain visible.
- Empty roster entries must not reorder occupied entries.
- Long HP values and names must not overflow cards.
- War Cry and unknown effects must remain distinguishable.
- Formation order and transitions require real-game validation.

## Tasks

1. Add a synthetic reader test for occupied-slot order with gaps and duplicates, and run it.
2. Add failing renderer checks for three centered columns and all eight cards in both halves; implement layout and colors and rerun.
3. Document the unverified formation order, run all tests and rebuild and inspect the Android package.
