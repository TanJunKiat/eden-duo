# Archived research tools

These are offline investigation aids, **not** production companion readers. No guest-memory snapshots, extracted assets or game binaries are included.

## `trace_roots.cpp`

Original authored pointer-graph search used during Time Stranger's party investigation. Preserved from the temporary research workspace without algorithm changes. The live validated result is documented in [the party research note](../../docs/superpowers/specs/2026-11-20-time-stranger-party-research.md); do not use arbitrary discovered chains as production bindings.

```sh
clang++ -std=c++20 -O2 tools/research/trace_roots.cpp -o /tmp/trace-roots
/tmp/trace-roots PRIVATE_POINTER_FILE TARGET_HEX MAIN_BASE_HEX MAX_OFFSET_HEX
```

Input consists of consecutive native little-endian pairs of 64-bit integers `(address, value)`, 16 bytes per edge, from the private diagnostic `timedump` command. Values and arguments are guest addresses, not host-process pointers. The original capture enumerated readable mappings and wrote aligned pointer-like entries; this tool does not attach to the emulator or read/write guest memory. Use a little-endian host with the expected 64-bit layout.

For each target node it finds incoming edges whose stored values are at most `MAX_OFFSET_HEX` below that node, then walks reverse references breadth-first. Printed output `root main+X -> +Y -> +Z` means dereference `[main+X]`, then dereference at `+Y`, then at `+Z` to reach the original target. It is a candidate path, not proof of object ownership or lifetime.

### Important original-tool limitations

- Research-grade input handling: no argument validation, no clear missing-file error, trailing partial records are ignored. An empty/missing input can exit successfully with no roots.
- Reads the full edge file into RAM and sorts it; there is no input-file size cap. Use only trusted, bounded local files, never uploads or a service endpoint.
- Candidate roots are addresses in the heuristic interval `[main, main+0x2000000)`, **not** a verified executable mapping. This can miss roots in larger modules or produce false positives.
- Unsigned address arithmetic is not hardened: keep the maximum offset below all target/node addresses and keep main far below the address-space limit.
- At most 10 reverse levels and 40 roots are reported. The 3-million-node limit is checked only after a level, not before individual allocations. Deduplication keeps only the first path to an address.
- Only nonnegative offsets are searched. Cycles are pruned; relevant alternative routes can be lost. No-result does not establish absence.
- A snapshot can contain stale copies, unrelated objects and transitional mixtures. Verify candidates against screenshots, controlled changes and fresh process restarts. Time Stranger's first plausible manager branch reached reusable copies; only the ordered party-vector branch was later validated.

### Synthetic smoke check

Create the fixture in `/tmp`, not in the repository. It contains no game data:

```sh
python3 - <<'PY'
import struct
with open('/tmp/root-tracer-synthetic.bin', 'wb') as f:
    f.write(struct.pack('<QQ', 0x100120, 0x200000))
    f.write(struct.pack('<QQ', 0x200010, 0x600000))
PY
/tmp/trace-roots /tmp/root-tracer-synthetic.bin 600000 100000 100
```

Expected output contains `root main+120 -> +10 -> +0`. This checks the archived search's path/offset convention only, not malformed-input behavior or the actual game root. Delete the fixture when finished if desired.
