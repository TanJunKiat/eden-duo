# Etrian Odyssey HD combat dashboard

## Intent

During combat, replace the companion's map with a useful battle dashboard; restore the fixed full map outside combat. Start broad: the player explicitly wants to explore hidden enemy information, then decide later what to remove. Candidate information includes exact enemy HP, combatants' HP/TP where applicable, active buffs, debuffs, ailments and binds, remaining turns and, if verifiable, weaknesses or resistances. None of these is promised until supported by evidence. Do not show unknown values as facts.

## Presentation

Keep the companion's Etrian-inspired navy, cyan and green styling. Prioritize combatant identity, HP and active effects; distinguish allies and enemies. Use the game's own icons at runtime when feasible, retaining legible text fallback. Fit the existing bottom-screen surface; select or paginate combatants only if necessary for legibility. Never reintroduce automatic zoom, pan or follow. The interface is informational only.

## Safety and behavior

The module is strictly read-only. Do not reveal future encounters, unseen maps, drops or AI intent: the user approved hidden data for combatants in the current battle, not future spoilers. When a validated battle state is absent, return to the existing map. During transitions or uncertain reads, do not show stale battle values; show a neutral loading state or unavailable markers, then recover. Suppress unsupported or unverified data rather than inventing it.

## Evidence and compatibility

The updated IL2CPP metadata suggests `BattleManager` has `battleCtrl` at +0x88; `BattleCtrl.BATTLE_CTRL` has arrays of battle characters at +0x30; each character has side, a string and a parameter reference at +0x14, +0x28 and +0x60, respectively. `BTL_CHARACTER_PARAM` refers to now/max values, bad status and efficacy arrays. `BstSealData.eEfficacy` names ailments, binds, upper and downer effects. These are static leads, not validated pointer chains, names, field semantics or timing. Confirm independently in the isolated research profile before implementation.

Maintain Android ARM64, companion runtime 19, the supported executable build ID and the existing map. No game files, firmware, keys, saves, or extracted proprietary assets in the repository or ZIP.
