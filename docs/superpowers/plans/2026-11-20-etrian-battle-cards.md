# Etrian Battle Cards Implementation Plan

> Execute inline with TDD; do not commit or push unless asked.

**Goal:** Show a readable card for each current combatant with available HP/TP and verified active skill names and timers.

**Architecture:** Keep the bounded battle snapshot reader and fixed map. Investigate the installed game's text table and live effect slots; add only evidence-supported reads and resolve names from the installed game at runtime if feasible. Render two-column cards on the existing surface and make overflow explicit.

**Tech Stack:** C++20 DSMod module, runtime 19, Android ARM64, Unity IL2CPP and installed RomFS.

**Spec:** `docs/superpowers/specs/2026-11-20-etrian-battle-cards-design.md`

## Global constraints
- Preserve recorded map and no zoom/follow. Never write guest memory or saves.
- Never redistribute game files, text tables, artwork or saves. No future encounter data.
- Unsupported or unverified effect names/timing must be labelled unavailable, not reported absent or guessed.
- Supported executable build ID and runtime 19 remain unchanged.

## Review focus
- Invalid or partial guest data must clear stale battle info.
- Duplicate enemies and multiple effects need separate records.
- Unknown/unreadable effects cannot be rendered as none.
- Locale, asset layout changes and out-of-range names must fail closed.
- Cards cannot silently clip combatants or effects.

## Tasks
1. Investigate the skill-name tables and effect records; record exact evidence, including update-vs-base uncertainty. Do not infer unrelated timer semantics from War Cry.
2. Add failing reader tests for effect identities and unknown durations, then implement only verified fields.
3. Add failing card rendering/overflow tests and implement the card UI, retaining unknown-state labels and the fixed map.
4. Run companion tests, build and inspect Android ARM64 ZIP and document remaining evidence and device-validation gaps.
