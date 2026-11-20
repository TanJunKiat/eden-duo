# Etrian Odyssey II and III companion ports

Create separate Android ARM64 runtime-19 packages for Etrian Odyssey II HD (`0100B810176A8000`) and III HD (`010084A0176AA000`), matching the existing first-game companion's fixed, read-only recorded map and battle-card presentation. Keep the first game's companion working unchanged. Restrict packages to identified updated builds; never copy unverified guest offsets or effect semantics as facts. Load game artwork at runtime with safe fallbacks. Unknown battle information must be unavailable, not presented as absent. Preserve unexplored maps and never expose future encounters.

Use isolated research profiles and save backups for runtime observations. Do not include game files, extracted artwork, keys, firmware, or saves in source control or the packages. Document which functionality was actually verified and what remains experimental. Do not commit or push without a separate request.
