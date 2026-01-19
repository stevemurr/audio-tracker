# TODO

## Known Issues

### Bitwig Loop Playback Caching

**Status:** Unresolved

**Problem:** When playing audio in a loop in Bitwig, the plugin stops receiving `process()` calls for regions that have already been played. This appears to be a DAW-level optimization where Bitwig caches/freezes plugin output for already-processed audio regions.

**Observed behavior:**
- First playthrough: plugin receives audio and streams metrics correctly
- Loop restart: plugin stops receiving `process()` calls for previously-played regions
- Moving the loop to a new region: plugin works again until that region has been played once

**Attempted fixes:**
- Implemented CLAP tail extension with `UINT32_MAX` (infinite tail) - did not help
- Used independent timer thread for streaming (not dependent on `process()` being called) - metrics still stale because no new audio data
- Sample-based timing instead of wall clock - did not help

**Possible solutions to investigate:**
1. Research Bitwig-specific settings to disable audio region caching
2. Check if other CLAP analyzer plugins (MiniMeters, LSP Spectrum) have this issue
3. Look into adding imperceptible noise to output to defeat identity-based caching
4. Contact Bitwig support / CLAP developers about this behavior
5. Test in other CLAP-compatible DAWs to confirm if Bitwig-specific

**References:**
- CLAP tail extension: `clap/include/clap/ext/tail.h`
- CLAP transport flags: `CLAP_TRANSPORT_IS_LOOP_ACTIVE`
