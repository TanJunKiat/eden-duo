# Etrian Map Visuals Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement task-by-task.

**Goal:** Replace distracting zoom with a stationary, game-inspired map.

**Architecture:** Keep the existing read-only C++ map renderer and game-art loader. Simplify the manifest to a fixed image and update the renderer's frame, coordinate tabs, and palette based on the game's map.

**Tech Stack:** C++20, DSMod JSON manifest, Android NDK.

**Spec:** `docs/superpowers/specs/2026-11-20-etrian-map-visuals.md`

## Global Constraints

- No guest writes, unrevealed geometry, packaged game assets, keys or saves.
- Keep runtime 19 compatibility and artwork fallback.

## Review Focus

- Fixed map has no gesture zoom or action buttons: inspect manifest and test module actions.
- Invalid or absent map stays hidden: retain existing readiness tests.
- Saved icon and player artwork still render: retain asset and image tests.
- Floors and walls retain correct orientation: retain map tests.
- Narrow display behavior and device appearance require Android testing.

## Task 1: Fixed view and map styling

**Files:** `companions/etrian-odyssey-hd/01008970176A6000.cpp`, `dualscreen/manifest.json`, `etrian_module_test.cpp`, `README.md`.

- [x] Change synthetic tests to assert zoom actions are rejected and styling uses game-inspired tab colors; run and confirm failure.
- [x] Remove zoom state/actions/bindings and all gesture configuration. Style the grid and tabs with the observed palette while keeping map dimensions consistent.
- [x] Run all companion tests and inspect a synthetic rendered screenshot.
- [x] Update README, build Android package, check ZIP contents and report device validation limitations.
