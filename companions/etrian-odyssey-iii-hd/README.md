# Etrian Odyssey III HD experimental companion

Maintainers: see the [three-game implementation and research handoff](../../docs/etrian-companions-maintainer-guide.md).

Install `010084A0176AA000.dsmod.zip` through Eden Duo's **Add-ons → Install → Dual screen mods**. Requires companion runtime 19 and updated main build ID `3D0322EDA242DEC85321974C126F43AC00000000000000000000000000000000`.

Version **0.1.4 — Abyss Glass** adds localized enemy names to the battle panel. The three panels are:

- **Armoroad:** active party cards in party order with current HP/TP and a distinct KO state. Empty and unreadable parties get different messages. Guild-only members and battle-only maxima are not shown.
- **Dungeon:** the fixed recorded-exploration map retains the full grid, drawn walls, icon fallbacks, and player heading.
- **Battle:** enemies appear on the stage above the party HUD. Cards follow the readable roster order, not verified in-game positions, and keep readable KO members visible. There are no Front/Back labels. The panel reports effects as unavailable; once verified, effects will use skill names, not stat abbreviations.

The companion does not write to the game. The v0.1.4 ZIP shows recorded exploration only. III's sea maps are not supported; unknown scenes show an unavailable screen rather than a stale dungeon map. Drawing and memo text are unsupported.

## B1F exploration trial (v0.1.6 source)

The separate trial build starts on the full recorded map with hints off. Tap **Hints: Off** to show a faint full-floor outline on B1F. Recorded tiles, walls, icons and heading take priority; the hints include no reference treasure, shortcut, or event markers. Tap **Floor Guide** to see monsters from the standard B1F encounter tables, using names from the installed game's active language. **Back to Map** restores the previous hint setting. Town and battle dismiss the guide.

Tap a monster card for localized item drops and base chances per defeated monster, before modifiers. Conditional items are listed, but their requirements and chances are marked unverified. **Back to List** returns to the roster. The guide does not show special encounter variants or monster encounter odds. B1F is the only floor with verified hint and guide data. The map reference must match the live floor identity, dimensions, and stair coordinates; missing or mismatched data falls back to the recorded map and an unavailable guide. Map geometry and monster data are read from the installed game, not bundled. The checked-in v0.1.4 ZIP is unchanged.

## Quest Tracker trial (v0.1.7)

From the dungeon map, tap **Quests**. Main missions come first, then active side quests you can work on from your current floor. Tap a row for the objective, locations and sources, required items, and turn-in point where known. Only active and report-ready entries appear; completed and unaccepted quests stay hidden. **Back to List** and **Back to Map** return without changing the map or hints.

The installed HD game supplies journal state, localized names and fallback objective text. The companion adds English guide hints—even where the journal is vague—based on the [EO III HD Steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=2994339871) and [Final Weapon's HD quest guide](https://finalweapon.net/2023/06/05/etrian-odyssey-3-quests-list/), matched to the installed game's entries. Incomplete or conflicting information falls back to journal text or omits the unverified detail. **In bag** is carried inventory, not quest credit. The tracker shows the game's active/report-ready flags; it does not estimate kill, mapping, event or gathering counters. It does not open in town, battle or at sea.

The B1F main mission and touch navigation were checked in an isolated macOS session. Side-quest flags and inventory transitions have synthetic coverage; late-game hints were guide-checked, not individually replayed. Physical Android testing is still outstanding.

Map artwork is read from the installed game's compressed Unity bundle at runtime. Validated icons fall back to built-in approximations if the bundle or texture metadata cannot be read. The icon-to-map-symbol mapping still needs visual validation. Enemy names come from the game's active text table (English, German, French, Italian, or Spanish). If the lookup is unavailable and the unit has no readable name of its own, the card falls back to `ENEMY <slot>`. Effect identities and durations remain unknown, and the first game's War Cry countdown is not reused.

The Armoroad reader was verified on the updated game in an isolated macOS developer session at the town hub, Aman’s Inn, and the Explorers Guild. A guild formation change updated the panel from one active member (44 HP / 18 TP) to two (50 HP / 24 TP first, 44 HP / 18 TP second), even though the two names were identical. A synthetic test covers refresh after HP/TP changes. An in-game before/after rest and installation on physical Android hardware have not been verified.

An isolated macOS smoke test of Abyss Glass showed the Armoroad roster, the B1F map during exploration, and a two-enemy encounter with one ally whose 44 HP / 18 TP matched the game. The v0.1.4 battle cards matched the in-game target names for both enemies in the English encounter. Battle maxima, enemy HP after damage, exact formation positions, and the full icon mapping need further live validation. Enemy names support Western European accents; other player-name characters still render as `?`.

The package contains no game files, saves, or extracted artwork. The bundled Manrope glyph atlas is licensed under the [SIL Open Font License](dualscreen/licenses/Manrope-OFL.txt). See the [maintainer guide](../../docs/etrian-companions-maintainer-guide.md#build-and-inspect-an-android-package) for the test and build commands.
