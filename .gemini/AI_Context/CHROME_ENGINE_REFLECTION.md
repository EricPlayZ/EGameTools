# Chrome Engine Dynamic Reflection (CRTTI) Guide

This document provides the technical specifications for the internal reflection system used in Techland's Chrome Engine (specifically Dying Light 2). Use this information to maintain and expand the Dynamic SDK.

---

## 1. Core Architecture
The engine maintains a global registry of all reflected classes (`CRTTI`) and their members (`CRTTIField`). Unlike standard C++ RTTI, this system provides exact memory offsets for fields at runtime.

### The Manager
- **Static Pointer**: `engine_x64_rwdi.dll + 0x1FFC9D0`
- **Primary Export**: `GetRTTIManager()` (Returns the `CRTTIManager*` instance).

### Key Engine APIs (DL2 Mangled Names)
- **Find Class**: `?FindClass@CRTTIManager@@UEBAPEBVCRTTI@@V?$string_const@D@ttl@@W4TYPE@FFindClass@1@_N@Z`
  - *Args*: `RCX` (Manager), `RDX` (ttl::string_const), `R8` (Type Mask, use 3), `R9` (bool, use false).
- **Find Field**: `?FindField@CRTTI@@QEBAPEBVCRTTIField@@PEBD@Z`
  - *Args*: `RCX` (CRTTI*), `RDX` (const char* name).

---

## 2. The String Table Breakthrough
Techland uses a deduplicated string table for all reflected names. A `ttl::string_const` or a name pointer in a struct is **never** a raw `char*`.

### Indirection Logic:
1.  **Indirection 1**: The structure (e.g., `CRTTI + 0x10`) holds a pointer to a **String Table Entry**.
2.  **Indirection 2**: The String Table Entry contains a **Tagged Pointer**.
3.  **Tagging**: The top 3 bits of the pointer are used for flags (e.g., `0x8` prefix).
4.  **Buffer**: After masking with `0x1FFFFFFFFFFFFFFF`, you get the actual heap address of the null-terminated string.

**C++ Implementation:**
```cpp
const char* GetName() const {
    uintptr_t* pEntry = UntagPointer(m_pNamePtr);
    uintptr_t taggedAddr = *pEntry;
    return reinterpret_cast<const char*>(taggedAddr & 0x1FFFFFFFFFFFFFFF);
}
```

---

## 3. Data Structure Layouts (DL2)

### `CRTTI` Structure
| Offset | Type | Description |
|--------|------|-------------|
| 0x00 | void* | VTable |
| 0x10 | uintptr_t* | **Name Entry Pointer** (see String Table Logic) |
| 0x18 | uint32_t | Class Byte Size |
| 0x28 | CRTTIField** | **Fields Array** (Tagged Pointer) |
| 0x30 | uint32_t | Field Count |
| 0xE8 | CRTTI* | **Base Class Pointer** (Tagged) |

### `CRTTIField` Structure
| Offset | Type | Description |
|--------|------|-------------|
| 0x08 | uintptr_t* | **Field Name Entry Pointer** |
| 0x50 | uint32_t | **Byte Offset** within the class instance |

---

## 4. Usage Strategy ("RED4ext" Approach)
To keep the SDK resilient to game updates:
1.  **Don't define hardcoded structs** for complex classes.
2.  **Use the `DynamicObject` wrapper**:
    - Wrap a game pointer (`void*`).
    - Use `RTTIManager::GetClass("ClassName")` to get the metadata.
    - Use `pCRTTI->FindField("m_MemberName")` to get the offset live.
    - Read/Write memory at `base_addr + offset`.

This ensures that even if a game patch moves `m_Fov` from `0x120` to `0x138`, your mod will still find it instantly.
