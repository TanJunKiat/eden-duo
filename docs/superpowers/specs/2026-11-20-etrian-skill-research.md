# Etrian skill and effect research

Research only; no game text or assets are included here. Findings are from the base title's installed RomFS and the updated executable's IL2CPP metadata, unless noted. Metadata is a lead, not evidence of live-state behavior.

## Skill catalogue

- Base `SkillMasterData` contains 189 player and 235 enemy skill records, with IDs 1–189 and 190–424 respectively. Parameters describe master definitions, not necessarily active status or duration.
- The base Unity asset bundle `tempcommon_assets_textlabel_7402ad57e6ae0368417241d1c9846a01.bundle` contains `tree1_text_label_skill_EFIGS`, a UTF-8 tab-separated table. It has `skillname_1` through `skillname_424` and columns for English, German, French, Italian and Spanish. Skill 9 matches the observed War Cry. Enemy skill names are therefore present in the same numbered table.
- The updated executable metadata identifies `Common.TextLabelDataManager` (type info at `main+0x53932f8`) with a dictionary at instance offset `0x18`, `GetTextSkillName(int)`, and `TextLabelData` fields for those languages. This is a promising runtime lookup, not yet verified as a readable live dictionary or as the currently selected locale. The base asset's applicability to the update has not been confirmed.
- Neither the bundle nor extracted names are packaged; current UI uses only the one observed War Cry name.

## Effect leads

The updated metadata's `BstSealData.eEfficacy` lists death, stone, sleep, confusion, terror, poison, blind, curse, paralysis, head/arm/leg binds, stun, and upper/lower categories for attack, speed, avoidance, style, defense, recovery, affinity, max HP, hit and squeeze. These names alone do not prove how these effects are stored or timed in live battle, or that the list covers every combat mechanic.

The only effect followed through live turns is War Cry: source ID 9, efficacy ID 14, observed countdown 4, 3, 2, 1, then cleared. The current reader accepts only that combination and range. Further controlled tests are needed to determine whether other entries' skill IDs, ownership, overlapping effects, and numeric timers have the same meaning; ailments and binds may have different storage.
