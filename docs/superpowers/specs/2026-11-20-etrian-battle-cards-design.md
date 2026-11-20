# Etrian battle cards

Replace the battle list with game-inspired per-combatant cards: name and HP/TP in the header, active skill names and remaining turns in stacked rows. Cover both sides without changing the fixed exploration map. Never present unknown effects as absent or invent durations. Prefer names loaded from the installed game's localized data; never package game data, artwork or assets. Unknown names and durations are explicit.

The user approved the card direction. A single-column list is easier to fit but unlike the reference; a two-column card layout better matches it, with explicit pagination when combatants exceed available space. Use the existing native module, read-only APIs, runtime 19 and supported build. Live validation of effect semantics is separate from synthetic tests.
