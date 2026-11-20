# Time Stranger: diagnostic scan method and command protocol

This preserves the useful original investigation procedure from the temporary emulator console fork. It is **not** a shipped feature or an instruction to enable write/cheat operations in the companion. The production module uses the fixed checked pointer chain and does not scan memory. Current results and obsolete leads are in the [party research note](../superpowers/specs/2026-11-20-time-stranger-party-research.md).

## Why the old scanner missed the records

The inherited Etrian scan used `HeapLow()`/`HeapHigh()` around a heap reservation beginning near `0x2104a00000`, while the observed Time Stranger records were in readable/writable `Normal` mappings near `0x115...`. Even a search for zero returned no hits in that old path. This diagnosed inadequate search coverage, not absent data or encryption. These addresses are historical session observations, not bindings.

The diagnostic implementation used `OwnerProcess()->GetPageTable().QueryInfo(...)`, inspected `GetSvcMemoryInfo()` state/permission, and read pages through `OwnerMemory().GetPointerSilent(page)`. The original enumeration stopped on query failure/zero-size/non-advancing regions, after 100,000 mappings, or at guest address `0x8000000000`. Those caps and mapping filters must be considered when interpreting negative results.

## Original diagnostic commands

The custom console polled a file selected by `EDEN_DSMOD_CMD`, consumed a command and truncated that file, and appended results to a separate output file. These commands are in the private research build, **not** a documented stock Eden interface.

| Command | Method and limits |
|---|---|
| `timemaps` | Enumerate nonzero-permission mappings; print guest address, size, state, permission and whether the page has a readable host pointer. |
| `timescan <decimal_u32>` | Search 4-byte-aligned u32 values in state `5` / permission `3` (`Normal`, read/write) pages. Stops after **40 hits**; not a complete candidate set. |
| `timecapture <decimal_hp> <decimal_sp>` | Search 2-byte-aligned u16 values matching either value across the same state-5/permission-3 pages. Write each candidate to `<command-file>.snapshot` as a packed 8-byte guest address followed by a 2-byte old value (10 bytes per record, little-endian in the original host). |
| `timecompare <old_hp> <new_hp> <old_sp> <new_sp>` | Re-read snapshot addresses as u16 and print exact old-to-new HP/SP transitions. Reads a changing live process; observations can include copies and unrelated values. |
| `timeptr <hex_target>` | Search all readable mappings for 8-byte-aligned u64 values in target ±`0x200`. Print the first **100** matches and total count. No guarantee that a match is a pointer or belongs to the party. |
| `timedump` | Search all readable pages, reading 8-byte-aligned values. Keep values in `[0x80000000, 0x8000000000)` divisible by 8. Write `(guest_address, value)` as two consecutive little-endian u64 values to `<command-file>.pointers`; these are heuristic pointer-like edges, not verified object references. |
| `timeparty` | Follow the final `main+0x146f500 → +0x10 → manager` chain, inspect vector bounds at manager `+8/+0x10`, then each slot holder and holder `+8` record. Print candidate name, level, EXP-like value, HP and SP. The final production reader has stronger checks and is the maintained implementation. |

No raw `.snapshot` or `.pointers` file is committed. They contain process/game data. The snapshot formats differ and must not be interchanged.

## Controlled-change procedure that produced the result

1. Use a copied isolated profile, independently check NAND/save paths, and back up its saves. Capture the displayed HP/SP and scene.
2. Capture all candidate values, make an ordinary game-controlled HP/SP change, then compare. In the original experiment, HP `466 → 450` and SP `305 → 250` narrowed 16,102 candidates to three exact matches, two adjacent. A value match alone was not treated as proof.
3. Compare adjacent record names and values for newly acquired Digimon; duplicates and reused copies must remain distinguishable. The observed allocation stride was `0x150`, but the final reader does not depend on it.
4. Change active/reserve membership through Setup and compare slot references, not just object data. The first UI-holder lead was transient and failed to establish a durable root.
5. Take a private pointer-edge dump and run the [offline reverse-root tracer](../../tools/research/README.md). The successful original dump contained 448,947 readable pages and 5,335,375 retained edges. There is no committed copy of it.
6. Validate a candidate chain by controlled swaps, menu close, save/load and fresh processes with relocated main/heap addresses. The manager's reusable-record branch at `+0x20` was rejected; its ordered party vector at `+8/+0x10` passed the documented tests.
7. Ship only bounded, checked reads tied to the exact title/build. Show unavailable on failed reads; do not ship the heavy scanner or use last-session heap addresses.

The temporary research console had inherited unrelated developer commands. Copying the entire fork would blur the read-only product boundary and carry machine-specific dependencies, so this archive preserves the protocol/method, original offline tracer, and the final maintained reader instead.
