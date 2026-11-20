# Etrian Odyssey II HD experimental companion

Maintainers: see the [three-game implementation and research handoff](../../docs/etrian-companions-maintainer-guide.md).

Install `0100B810176A8000.dsmod.zip` through Eden Duo's **Add-ons → Install → Dual screen mods**. Requires companion runtime 19 and updated main build ID `8883310FF46D85D17C09882B36A8205D00000000000000000000000000000000`.

This read-only experimental port uses the first game's fixed recorded-map presentation and battle-card layout. It does not write to the game or expose unexplored areas. Game map artwork is read from the installed title at runtime, with fallbacks when the validated asset layout is absent. Sea maps, drawing and memo text are unsupported.

In an isolated II session the map position/floor matched the dungeon view; the battle roster contained the displayed ally and two enemies, and the ally's 45 HP / 20 TP matched the screen. Other battle cases, enemy HP after damage, formation order, transitions, icon mappings and Android behavior still need validation. Effect identities and durations have not been verified for II, so they are shown as unknown; the first game's War Cry countdown is deliberately not reused. Non-ASCII combatant names render as `?`.

The package contains no game files or extracted artwork. See the first-game `../etrian-odyssey-hd/README.md` for the test and build approach; use this title ID, source file and build ID instead.
