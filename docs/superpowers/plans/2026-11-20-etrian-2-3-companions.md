# Etrian II and III Companions Implementation Plan

> Execute inline using TDD; do not commit or push without a request.

**Goal:** Produce a separately installable, read-only companion for each updated game.

**Architecture:** Reuse proven UI/map code where safe, with title-specific build gates, memory-layout constants and validated runtime artwork. Determine offsets independently from each installed update and verify critical behavior with isolated runtime observations. Fail closed on unverified data.

**Tech Stack:** C++20 DSMod, IL2CPP metadata and Unity assets, Android ARM64 NDK r28c, companion runtime 19.

**Spec:** `docs/superpowers/specs/2026-11-20-etrian-2-3-companions.md`

## Global constraints

No game/asset/key/save redistribution; no guest writes; fixed recorded map only; only current-battle information; no guessed timers; separate isolated profiles and backups; no commits or pushes.

## Review focus

- Mismatched game/update must fail closed.
- Map geometry and recorded/unexplored flags must be independently validated.
- Scene transitions must not display stale battle/map state.
- Unknown artwork and effect timing must not appear valid.
- Full party/enemy counts and duplicate actors must not be silently lost.

## Tasks

1. Identify each updated executable build and inspect its map/battle metadata and asset layouts; record candidates and limitations.
2. Create synthetic tests proving each title's bindings and fail-closed behavior. Implement each title's independently supported readers and shared UI, only where evidence permits.
3. Validate on isolated live sessions where possible; document missing evidence without calling estimates verified.
4. Run first-game and new tests, build separate ARM64 packages, inspect ZIPs and report exactly what remains unverified.
