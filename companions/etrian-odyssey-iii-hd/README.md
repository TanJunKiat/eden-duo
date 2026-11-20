# Etrian Odyssey III HD experimental companion

Maintainers: see the [three-game implementation and research handoff](../../docs/etrian-companions-maintainer-guide.md).

Install `010084A0176AA000.dsmod.zip` through Eden Duo's **Add-ons → Install → Dual screen mods**. Requires companion runtime 19 and updated main build ID `3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000`.

This read-only experimental port uses the first game's fixed recorded-dungeon-map presentation and battle-card layout. It does not write to the game or expose unexplored areas. III's sea maps are not supported; unknown scenes show an unavailable screen rather than a stale dungeon map. Drawing and memo text are unsupported.

Map artwork is read from the installed game's compressed Unity bundle at runtime. Validated icons fall back to built-in approximations if the bundle or texture metadata cannot be read. The icon-to-map-symbol mapping still needs visual validation. In battle, III's enemy names are not yet resolved: cards show `ENEMY 1`, etc. Effect identities and durations remain unknown, and the first game's War Cry countdown is not reused.

In an isolated III session the map position/floor matched the dungeon view; the battle roster contained the displayed ally and two enemies, and the ally's 44 HP / 18 TP matched the screen. HP maxima, enemy HP, formation order, full parties, transitions, icon mappings and Android behavior need further validation. Non-ASCII combatant names render as `?`.

The package contains no game files or extracted artwork. See the first-game `../etrian-odyssey-hd/README.md` for the test and build approach; use this title ID, source file and build ID instead.
