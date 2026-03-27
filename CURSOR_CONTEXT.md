# EGameSDK / EGameTools Development Context

## Project Overview

Building a C++ SDK and Mod Menu for **Dying Light 2**.
**Core Goal**: Move away from hardcoded, version-specific memory addresses and offsets, and build a highly resilient, dynamic system capable of surviving game updates.

## The Problem: Dynamic Instance & Offset Discovery

We attempted to build a RED4ext-like RTTI (Run-Time Type Information) system to dynamically find class instances and property offsets.

### What We Tried (RTTI Approach)

1. **CRTTI Parsing**: Successfully parsed Techland's RTTI structure, handling 48-bit pointer tagging (`0x0000FFFFFFFFFFFF`) and inline storage heuristics.
2. **Instance Discovery (`GetFirstInstance`)**: Attempted to dynamically find the `CGame` and `PlayerDI_PH` instances by scanning:
    - The `IFactory` global list (only contained 160 objects, missing core singletons).
    - The `CRTTI` object's internal instance list at offset `+0x38` (empty/sentinel for `CGame`).
    - `CSerializableObject::GetLoadedGSObjectsList` (returned null).
3. **The `CGame` Singleton**: Discovered that `CGame` is not tracked in standard RTTI lists. It is retrieved via a static pointer to `CLobbySteam` (`0x2778058`), with `CGame` located at `[CLobbySteam + 0xF8]`.
4. **Missing Properties**: Tried to use RTTI to dynamically find the offsets for `maxImmunity` and `nightrunnerTimer` inside `PlayerInfectionModule`.
    - **Discovery**: These variables do _not_ exist in the standard RTTI registry. They are internal C++ logic variables not exposed to the game's reflection system.
    - We found some of them exposed via the `ChromeSpy::SEventPlayer` debugging system (`sub_14D2720`), but relying on RTTI for internal logic variables proved to be a dead end.

### The Tools We Used (And Abandoned)

- **Cheat Engine `scan_all`**: Abandoned. Caused severe RAM spikes and system hangs.
- **Cheat Engine `evaluate_lua`**: Abandoned. Caused game crashes when executing engine vtable functions (like `GetRTTI`) mid-execution.

---

## The Pivot: Instruction-Based Pattern Scanning

Because RTTI does not track internal logic variables (`maxImmunity`, `nightrunnerTimer`, etc.) or robustly track singleton instances, **we are abandoning RTTI for offset discovery.**

**New Strategy**: Surgical Assembly Pattern Scanning.
Instead of scanning for RTTI names, we will use Cheat Engine to find the exact assembly instructions that read/write to the desired class offsets (e.g., `movss xmm0, [rcx + 2Ch]`). We will signature-scan these specific functions and extract the offset (`0x2C`) dynamically from the instruction bytes.

---

## Architectural Review: `OffsetManager` Class

The current `OffsetManager` handles patterns via macros (`AddPattern`, `AddDynamicPattern`, `AddStaticOffset`), but requires refactoring to support the new strategy efficiently.

### Identified Flaws in Current Implementation:

1. **Header Bloat**: `AddPattern` places raw byte patterns directly in `Offsets.h`. Changing a pattern forces a recompilation of every file that includes the header.
    - _Fix_: Move all pattern strings to `Offsets.cpp` and only expose the getter in the header.
