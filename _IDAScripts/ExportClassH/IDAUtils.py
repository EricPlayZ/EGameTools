from functools import cache
import idc
import idaapi
import idautils
import ida_bytes
import ida_nalt

IDA_NALT_ENCODING = ida_nalt.get_default_encoding_idx(ida_nalt.BPU_1B)

idaRTTIStrings: dict[bytes, dict[str, int]] = {}

# def GetDemangledExportedSigs() -> list[str]:
#     """
#     Generate a list of demangled function signatures from IDA's database.
#     Uses a set to avoid duplicate entries.
#     """
#     sigs_set = set()
#     entry_qty = idc.get_entry_qty()
#     for i in range(entry_qty):
#         ea: int = idc.get_entry(i)
#         exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
#         if not exportedSig:
#             continue
#         demangledExportedSig: str = Utils.DemangleSig(exportedSig)
#         if demangledExportedSig and "~" not in demangledExportedSig and not demangledExportedSig.endswith("::$TSS0") and "::`vftable'" not in demangledExportedSig:
#             sigs_set.add(demangledExportedSig)
#     return list(sigs_set)

def GetDemangledExportedSigs(inputMD5: bytes) -> list[str]:
    """
    Generate a list of demangled function signatures from IDA's database.
    Uses a set to avoid duplicate entries based on (ordinal, signature).
    """
    sigsSet = set()
    entryQty = idc.get_entry_qty()
    for i in range(entryQty):
        #ea: int = idc.get_entry(i)
        #exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
        exportedSig = idc.get_entry_name(i)
        if not exportedSig:
            continue
        demangledExportedSig: str = DemangleSig(exportedSig)
        if demangledExportedSig and "~" not in demangledExportedSig and not demangledExportedSig.endswith("::$TSS0") and "::`vftable'" not in demangledExportedSig:
            sigsSet.add((i + 1, demangledExportedSig))
    
    return [sig for _, sig in sorted(sigsSet)]

@cache
def DemangleSig(sig: str) -> str:
    return idaapi.demangle_name(sig, idaapi.MNG_LONG_FORM)

@cache
def GetMangledTypePrefix(namespaces: tuple[str, ...], className: str) -> str:
    """
    Get the appropriate mangled type prefix for a class name.
    For class "X" this would be ".?AVX@@"
    For class "NS::X" this would be ".?AVX@NS@@"
    For templated classes, best to use get_mangled_name_for_template instead.
    """
    if not namespaces:
        return f".?AV{className}@@"
    
    # For namespaced classes, the format is .?AVClassName@Namespace@@
    # For nested namespaces, they are separated with @ in reverse order
    mangledNamespaces = "@".join(reversed(namespaces))
    return f".?AV{className}@{mangledNamespaces}@@"

@cache
def BytesToIDAPattern(data: bytes) -> str:
    """Convert bytes to IDA-friendly hex pattern string."""
    return " ".join("{:02X}".format(b) for b in data)

@cache
def GetSectionInfo(inputMD5: bytes, sectionName: str) -> tuple[int, int]:
    """Get start address and size of a specified section."""
    for seg_ea in idautils.Segments():
        if idc.get_segm_name(seg_ea) == sectionName:
            start = seg_ea
            end = idc.get_segm_end(seg_ea)
            return start, end - start
    return 0, 0

@cache
def GetIDARTTIStringsList(inputMD5: bytes) -> dict[str, int]:
    global idaRTTIStrings

    rttiStrings = idaRTTIStrings.get(inputMD5)
    if not rttiStrings:
        idaRTTIStrings[inputMD5] = {}
        strings = idautils.Strings()
        for stringItem in strings:
            if not stringItem:
                continue
            s = str(stringItem)
            if s.startswith(".?AV"):
                ea = stringItem.ea if stringItem.ea else idc.BADADDR
                idaRTTIStrings[inputMD5].update({ s: ea })
        rttiStrings = idaRTTIStrings[inputMD5]
    return rttiStrings

@cache
def FindPatternInRange(inputMD5: bytes, pattern: str, start: int, size: int, end: int = 0) -> int:
    if not end:
        end = start + size
    
    compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
    errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, pattern, 16, IDA_NALT_ENCODING)
    if errorParsingIDAPattern:
        return idc.BADADDR
        
    patternAddr: int = ida_bytes.bin_search(start, end, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
    if patternAddr == idc.BADADDR:
        return idc.BADADDR
        
    return patternAddr

@cache
def FindAllPatternsInRange(inputMD5: bytes, pattern: str, start: int, size: int, end: int = 0) -> list[int]:
    """Find all occurrences of a pattern within a memory range."""
    addresses: list[int] = []
    if not end:
        end = start + size
    
    while start < end:
        patternAddr = FindPatternInRange(inputMD5, pattern, start, size, end)
        if patternAddr == idc.BADADDR:
            break
            
        addresses.append(patternAddr)
        start = patternAddr + 8  # advance past found pattern
        
    return addresses