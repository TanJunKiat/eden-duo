# Digimon Story: Time Stranger companion design

## Intent and scope

Create an Eden Duo second-screen companion for the Switch game, targeting update v1.2.2 first. The intended panels are map, party, evolution tracker, DigiFarm and quest tracker. Begin with the party panel, then expand in later milestones. The party panel should eventually show as much information about each Digimon as is practical and verified; exact available fields are an investigation outcome, not a promise.

## Approaches considered

- Live memory: reflects current game state but requires build-specific investigation and validation. **Selected by the user.**
- Save-file reading: can provide saved progress but not necessarily current state and requires understanding the save format.
- Manual entry: enables a quick interface but requires the player to maintain the data.

## First milestone

Investigate live memory for party data, starting with a visible Digimon's identity and statistics. Establish the exact executable build identity and verify candidate values against controlled in-game changes, transitions and reloads. Expand to the party and additional fields where evidence supports it. Record addresses, pointer chains, data types, observations and limitations, distinguishing candidates from verified findings. Do not assume any particular field or structure exists or is discoverable until observed.

Build a party panel for verified fields when enough evidence exists. Handle unsupported builds, invalid pointers, incomplete reads and game transitions safely; display unavailable or unknown rather than stale or invented values. Do not write guest memory or modify normal saves. Use an isolated research profile and independently verify its save paths and backups before experiments.

## Artwork

Investigate loading icons and other useful artwork from the installed game at runtime, following the existing companions where appropriate. Do not commit or distribute game assets, game images, keys, firmware or saves. Use simple fallbacks when artwork is unavailable. Asset formats and feasibility remain unverified.

## Validation

Use synthetic tests for memory readers and rendering logic where practical. Separately validate live values against the game on v1.2.2, including changes and reloads; synthetic tests do not prove the memory bindings. Report device validation and unverified fields explicitly. Other panels and support for other game versions are outside the first milestone.
