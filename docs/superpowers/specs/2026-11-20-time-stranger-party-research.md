# Time Stranger v1.2.2 party memory research

## Current result: restart-validated party binding

**This section supersedes the earlier unresolved-root notes below.** The full reverse-reference search found a path to the live ordered party vector, not the reusable duplicate vector previously examined at manager `+0x20`.

For the updated main build identified below, let `read64` be a checked guest-memory read:

```text
root    = read64(main_base + 0x146f500)
manager = read64(root + 0x10)
begin   = read64(manager + 0x08)
end     = read64(manager + 0x10)
count   = (end - begin) / 8
holder  = read64(begin + slot * 8)
record  = read64(holder + 0x08)
```

The tested party has six entries. Entries 0–2 matched the three active battle members; entries 3–5 matched reserves in the observed setup. Do not deduplicate names or assume contiguous record allocation: use each vector entry. Check pointers, mapping, ordered bounds, 8-byte divisibility, and a maximum of six entries. Handle null/empty/unavailable data without serving a previous snapshot. Empty slots, fewer members, another profile, Android and other updates still require validation; this is not an exhaustive engine contract.

Fields relative to `record`:

| Offset | Read | Observation |
|---|---|---|
| `+0x08` | bounded inline name bytes | ASCII names matched all six records; full encoding and maximum length not established |
| `+0x68` | u32 | level matched displayed values |
| `+0x6c` | u32 | EXP-like value; **not** the displayed progress-within-level; semantics unverified |
| `+0x74` | u32 | current HP matched observations outside battle |
| `+0x78` | u32 | current SP matched observations outside battle |

### Verification performed

1. Reverse-reference snapshot of 448,947 readable pages produced 5,335,375 pointer-like edges. An offline breadth-first reverse walk identified the executable-relative path. This was research only, not a proposed per-frame scanner.
2. In one session, the vector initially held Patamon, Koromon, Botamon, Wanyamon, Botamon, Wanyamon. Swapping the first active member with the first reserve through the game's Setup screen changed vector entry 0 from Patamon's holder to Wanyamon's holder and entry 3 the opposite way. The underlying records did not move. Screenshots independently confirmed the active/reserve swap.
3. The path remained readable after exiting Setup to exploration. The swapped configuration was saved through the game into previously empty diagnostic slot 05; the pre-swap state was separately saved in diagnostic slot 04. Backups were taken before research and restart. These are isolated diagnostic saves, not the regular profile.
4. After a full process restart and load, the fixed-root `timeparty` diagnostic (no scan) returned active Wanyamon (level 2, HP 336, SP 296), Koromon (2, 523, 150), Botamon (2, 329, 216), followed by reserve Patamon, Botamon, Wanyamon. Screenshot corroboration covered exploration and the battle-members menu.
5. Another full process restart again resolved all six using exactly the same code and offsets, without scanning. Main base changed from `0x809a3000` to `0x8027b000`; manager changed from `0x115df0a000` to `0x115f5fe000`; first record changed from `0x115ec6a690` to `0x115ea05540`. Names, order and observed stats agreed. This is the required evidence against accidental absolute-address reuse.

**Result:** sufficient evidence to implement an experimental v1.2.2 active-name companion, with checked reads and fail-closed behavior. In-battle stat synchronization, maximum HP/SP, talent, guests, empty slots and device portability are not yet established. The initial research stopped at this point; a names-only companion has now been implemented under `companions/time-stranger/` (see validation below).

### Names-only companion validation

Implemented `companions/time-stranger/010062E01FE0C000.cpp`, a checked read-only module with exact title/full-build checks, ordered slots, bounded printable-ASCII names, and rejection of detected roster transitions. Its manifest uses native `bind_text` labels, not game assets. Synthetic tests cover normal and relocated roots, reserve swap, duplicate names, null/short rosters, malformed bounds, missing reads, unterminated/control-character names, mutation during sampling, and wrong title/build. Tests passed under AddressSanitizer and UndefinedBehaviorSanitizer.

The host module was loaded through the diagnostic emulator's actual DSMod loader using a **temporary macOS platform selection patch**; this patch is not in the repository or release. First screenshot showed Wanyamon / Koromon / Botamon, matching the game. Further Setup swaps produced Botamon / Patamon / Botamon in both the game and the companion, preserving duplicate slots. A full emulator restart reloaded diagnostic save 05 and the rendered companion again showed Wanyamon / Koromon / Botamon with relocated pointers. This validates the native reader and native label manifest locally, **not the Android binary on device**.

Release `companions/time-stranger/dist/010062E01FE0C000.dsmod.zip` targets Android ARM64, API 28+, runtime 19+. It contains only package metadata, dualscreen manifest and stripped module. Cross-compiled using NDK r28c with static libc++ and 16-KiB load alignment. The full exact build ID is present in both package and module. Archive CRC and module SHA-256 checks passed. The Android installer/device has not been exercised; battle transitions remain unverified. No commit or push was made.

### Local reproduction artifacts

Private, untracked diagnostic data lives under `~/Documents/Eden/time-stranger-diagnostics/`: `cmd.pointers`, `cmd.out`, `restart-proof-1-console.txt`, `restart-proof-1.log`, `run.log` and screenshots. Temporary source `/tmp/eo-console/mod_console_time.cpp` adds `timeparty`; executable `/tmp/eo-console/eden-time` consumes a command file selected by `EDEN_DSMOD_CMD`. It contains other research operations and is not a distributable read-only product. The command above and offsets here are the durable recipe; do not rely on temporary executables continuing to exist. No proprietary extracted data should be committed.

## Scope and evidence

Switch title `010062E01FE0C000`, update file labeled v1.2.2 (package version 262144); emulator reports updated main build ID `135C83CDEA06AEC2D858E1BC7D53D72F7257166C`. Research used a separate diagnostic copy of the player's save and read-only guest-memory inspection. Addresses below are guest virtual addresses from one emulator session, not stable bindings.

A diagnostic scanner enumerated readable writable state-5 mappings and captured all 16-bit-aligned matches for HP 466 and SP 305. It inspected 491,251 readable pages and recorded 16,102 candidates. After the player reported HP 450 and SP 250, three candidates changed as expected:

- `0x115F3FD5B4`: 466 → 450 (HP candidate).
- `0x115F3FD5B8`: 305 → 250 (SP candidate).
- `0x116959A2E0`: 305 → 250 (additional SP candidate).

A subsequent 32-bit read starting at `0x115F3FD5A8` returned `1, 6, 0, 450, 250, 0, 0, 0`. The player reported level 1 and EXP 6 at the same time, so the adjacent first two fields are level and EXP candidates; whether 6 represents accumulated or remaining EXP is unverified. HP and SP also appear consistent with 32-bit values in this snapshot, though width and meaning require further testing. These observations support a likely live party-related structure, not a proven durable pointer chain or complete party layout.

The earlier Etrian scanner reported zero hits even when searching for zero; its bounds did not establish coverage for this game. Its negative searches must not be treated as evidence of absence. The diagnostic scanner was a temporary research build and snapshots only the writable memory mappings it enumerated; other regions and encodings are outside its coverage.

## Second Digimon observation

After the player acquired Koromon, the same session contained two records 0x150 bytes apart. At guest address `0x115F3FD548`, the inline bytes spell `Patamon`; at `0x115F3FD698`, they spell `Koromon`. Relative to each name's first byte, 32-bit reads at offsets `+0x60`, `+0x64`, `+0x6c`, and `+0x70` were respectively:

| Name | +0x60 | +0x64 | +0x6c | +0x70 |
|---|---:|---:|---:|---:|
| Patamon | 3 | 170 | 538 | 400 |
| Koromon | 1 | 0 | 499 | 136 |

The player independently reported Koromon HP 499, SP 136 and EXP 0; the earlier Patamon HP and SP at these offsets changed together with the player's reports. This supports candidate level, EXP, current HP and current SP fields, and a 0x150-byte spacing in this particular pair. Patamon's later values, general record stride, name encoding, other party members, duplicates, level semantics and the distinction between current/remaining EXP remain unverified. Another identical Koromon HP/SP pair was observed at `0x115F3FDD94` and `0x115F3FDD98`; its role is unknown. Do not mistake a session address or a duplicate for a stable binding.

## Third Digimon observation

The next 0x150-byte-spaced name location, `0x115F3FD7E8`, contains the inline bytes `Botamon`. At the same offsets `+0x60`, `+0x64`, `+0x6c`, and `+0x70`, 32-bit reads returned `1, 0, 308, 200`. The player reported EXP 0/8, HP 308/308, and SP 200/200, independently corroborating the repeated layout for a third adjacent record. This does not establish the maximum values, talent, a stable pointer, or how party membership is represented; Botamon's level was not separately reported.

## Pointer investigation after restart

After restarting and loading the diagnostic save, a fresh mapped-memory scan found three matching inline Botamon name/HP/SP records; the record adjacent to Patamon and Koromon began at `0x115F202698`, and Patamon's name began at `0x115F2023F8`. This confirms the prior absolute addresses did not survive restart, while the observed record spacing did.

A full readable-mapping scan found a pointer at `0x114B1BD418` to `0x115F2023F0` (eight bytes before Patamon's name), and `0x114B1BD798` to `0x115F202540` (eight bytes before Koromon's name). A possible holder array at `0x1149A65810` pointed to `0x114B1BD380` and `0x114B1BD700`, each with the corresponding record pointer at `+0x98`. These could be transient UI or other objects; their meaning, third entry, lifetimes, and party membership are not verified. Scanning for references near the holder array found several heap references, but no stable module-relative root was established. A scan for references around one possible higher-level holder, `0x1166636170`, returned no hits; this is not proof that no usable root exists. None of these absolute addresses are safe companion bindings.

A subsequent read confirmed that the third candidate holder at `0x114B1BDA80 + 0x98` points to `0x115F202690`, eight bytes before Botamon's name, while the fourth holder's corresponding pointer was null at that moment. This is consistent with three populated UI-related slots but does not establish battle-active versus reserve status. A scan around `0x114966E678`, another pointer present in the holder structure, found 12,867 references and is not a useful discriminating root without further evidence.

## Active slot swap

The player added Wanyamon (HP 315, SP 280, EXP 0), whose name and values appeared at the next 0x150-spaced record. With Patamon, Koromon and Botamon reported active, the first three candidate holder pointers referenced those records. After the player swapped Wanyamon into Patamon's active slot and reported Wanyamon, Koromon and Botamon active, the first candidate holder's record pointer changed from `0x115F2023F0` (Patamon) to `0x115F2027E0` (Wanyamon). The underlying Patamon and Wanyamon records remained at their original session addresses. This supports the holder array as an active-slot or UI representation in this scenario, not a proven persistent battle-party model. A later reference search found a circular-looking set of heap pointers and did not produce a stable module-relative root. Scene transitions, reserves, reloads and battle behavior still require validation.

## Duplicate record reuse

In the restarted session, `0x115F202BD8` initially held a Botamon record, but a later read at that same address held Wanyamon with matching HP/SP. Therefore at least this duplicate is reused; it must not be treated as a durable Botamon record or stable party slot. Nearby heap entries contain indices and pointers to the duplicate area, but their purpose is unknown. The update's extracted ExeFS is an NSO executable; the inspected update RomFS did not contain `global-metadata.dat`, so the Etrian Unity IL2CPP route is not established for this title. Extracted game material remains private and untracked.

## Candidate root tested across another restart

A readable-module pointer at `main+0x146f500` was followed through `+0x10` after dereferencing; this resolved to a heap object in two emulator sessions. In both sessions, that object at `+0xb0` contained a plausible Digimon record. However, the root also led to a reusable duplicate record whose identity changed from Wanyamon to Botamon between sessions. After leaving the menu and entering battle, the root's embedded Patamon record had candidate EXP 184 while the previously observed Patamon record had candidate EXP 185. Those readings demonstrate disagreement; neither value's exact semantics were independently established. This root is therefore only a lead and must not be shipped as live party data without establishing its lifecycle and proving correct behavior across transitions.

The old `HeapLow` helper logged a heap reservation beginning at `0x2104a00000`, while the observed party records were in a readable writable `Normal` mapping at `0x115db1f000..0x115fa8e000` in that session. This explains why limiting the earlier scanner to the reported heap reservation missed these records; a module cannot assume those records are within that reservation. The bounds, addresses and mappings are session-specific.

## Six occupied party records

In the later diagnostic session the user reported filling all six slots. Six consecutive 0x150-spaced records beginning at `0x115EC67698` contained Patamon, Koromon, Botamon, Wanyamon, Botamon, and Wanyamon. The first three names, levels and current HP/SP matched the displayed battle-members screen: Patamon level 3, 538 HP and 345 SP; Koromon level 2, 523 HP and 150 SP; Botamon level 2, 329 HP and 216 SP. The other records were not independently compared with the reserve screen. Duplicated names occur, so deduplicating by name would be incorrect. The mapping between this record order and battle-active order after additional switches remains unverified. The executable-relative root is still unresolved.

## Next checks

- Observe the same structure through another HP/SP change, and validate level and EXP against a controlled change.
- Trace references to the candidate structure from readable mappings or game code; revalidate across scene transitions, reloads and a new emulator process before creating any build-specific binding.
- Investigate party slots, maximum values and identity independently; do not infer them from adjacency alone.
- Preserve read-only behavior and display unknown when a binding fails validation.
