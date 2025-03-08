import struct
import idc
import idaapi
import ida_bytes
import ida_ida
import ida_hexrays

from ExportClassH import Utils
from ExportClassH.ClassDefs import ClassName

def GetVTablePtr(targetClass: ClassName, targetClassRTTIName: str = "") -> int:
    """
    Find vtable pointer for a class using RTTI information.
    Supports both simple class names and namespaced class names.
    For templated classes, you can directly provide the rtti_name pattern.
    
    Returns the vtable pointer (an integer) or 0 if not found.
    """
    baseDLLAddr: int = idaapi.get_imagebase()
    
    # Use provided RTTI name if available (for templates), otherwise generate it
    if not targetClassRTTIName:
        # Check if this is a templated class
        typeDescriptorName: str = Utils.GetMangledTypePrefix(targetClass.namespaces, targetClass.name)
    else:
        # Use the provided RTTI name directly
        typeDescriptorName: str = targetClassRTTIName
    
    # Search for the RTTI type descriptor
    typeDescriptorBytes: bytes = typeDescriptorName.encode('ascii')
    idaPattern: str = Utils.BytesToIDAPattern(typeDescriptorBytes)
    
    # Search in .rdata
    rdataStartAddr, rdataSize = Utils.GetSectionInfo(".rdata")
    if not rdataStartAddr:
        return 0
        
    # Look for the type descriptor
    compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
    errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, idaPattern, 16, Utils.IDA_NALT_ENCODING)
    if errorParsingIDAPattern:
        return 0
        
    typeDescriptorPatternAddr: int = ida_bytes.bin_search(rdataStartAddr, ida_ida.cvar.inf.max_ea, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
    if typeDescriptorPatternAddr == idc.BADADDR:
        print(f"Type descriptor pattern '{typeDescriptorName}' not found for {targetClass.namespacedName}.")
        return 0
        
    # Adjust to get RTTI type descriptor
    rttiTypeDescriptorAddr: int = typeDescriptorPatternAddr - 0x10
    
    # Compute offset relative to base address
    rttiTypeDescriptorOffset: int = rttiTypeDescriptorAddr - baseDLLAddr
    rttiTypeDescriptorOffsetBytes: bytes = struct.pack("<I", rttiTypeDescriptorOffset)
    rttiTypeDescriptorOffsetPattern: str = Utils.BytesToIDAPattern(rttiTypeDescriptorOffsetBytes)
    
    # Search for references to this offset
    xrefs: list[int] = Utils.FindAllPatternsInRange(rttiTypeDescriptorOffsetPattern, rdataStartAddr, rdataSize)
    
    # Analyze each reference to find the vtable
    for xref in xrefs:
        xref: int

        # Check offset from class
        offsetFromClass: int = idc.get_wide_dword(xref - 8)
        if offsetFromClass:
            continue
            
        # Get object locator
        objectLocatorOffsetAddr: int = xref - 0xC
        
        # Look for references to the object locator
        objectLocatorBytes: bytes = struct.pack("<Q", objectLocatorOffsetAddr)
        objectLocatorPattern: str = Utils.BytesToIDAPattern(objectLocatorBytes)
        
        compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
        errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, objectLocatorPattern, 16, Utils.IDA_NALT_ENCODING)
        if errorParsingIDAPattern:
            continue
            
        objectLocatorAddr: int = ida_bytes.bin_search(rdataStartAddr, ida_ida.cvar.inf.max_ea, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
        if objectLocatorAddr == idc.BADADDR:
            continue
            
        # Vtable pointer is at (objectLocatorAddr + 0x8)
        vtableAddr: int = objectLocatorAddr + 8
        if vtableAddr <= 8:
            continue
            
        return vtableAddr
        
    print(f"Failed to locate vtable pointer for {targetClass.namespacedName}.")
    return 0

def GetDemangledVTableFuncSigs(targetClass: ClassName, targetClassRTTIName: str = "") -> list[tuple[str, str]]:
    """
    Get the ordered list of function names from a class's vtable.
    For templated classes, you can provide the rtti_name pattern.
    """
    vtablePtr: int = GetVTablePtr(targetClass, targetClassRTTIName)
    if not vtablePtr:
        print(f"Vtable pointer not found for {targetClass.namespacedName}.")
        return []
        
    demangledVTableFuncSigsList: list[tuple[str, str]] = []
    segmEnd: int = idc.get_segm_end(vtablePtr)
    ea: int = vtablePtr
    
    while ea < segmEnd:
        ptr: int = idc.get_qword(ea)
        if not ptr:
            break
        seg = idaapi.getseg(ptr)
        if seg is None or seg.type != idaapi.SEG_CODE:
            break
        
        # Force function decompilation to generate the full function type signature
        funcSig: str = idc.get_func_name(ptr)
        demangledFuncSig: str = Utils.DemangleSig(funcSig)
        demangledFuncSig = demangledFuncSig if demangledFuncSig else funcSig
        rawType: str = ""

        if not demangledFuncSig:
            ea += 8
            continue
        
        if demangledFuncSig != "_purecall":
            if " " not in demangledFuncSig:
                ida_hexrays.decompile(ptr)
                rawType = "IDA_GEN_TYPE " + idc.get_type(ptr)
            if (demangledFuncSig, rawType) in demangledVTableFuncSigsList:
                demangledFuncSig = "DUPLICATE_FUNC " + demangledFuncSig
        
        demangledVTableFuncSigsList.append((demangledFuncSig, rawType))
        ea += 8
        
    return demangledVTableFuncSigsList