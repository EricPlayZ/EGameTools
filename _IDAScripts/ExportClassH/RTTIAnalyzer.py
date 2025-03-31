import idc
import idaapi
import idautils
import ida_hexrays
from functools import cache

from ExportClassH import IDAUtils, Config

@cache
def GetVTablePtr(inputMD5: bytes, parentNamespacesClasses: tuple[str, ...], targetClassName: str, targetClassRTTIName: str = "") -> int:
    """
    Find vtable pointer for a class using RTTI information.
    Supports both simple class names and namespaced class names.
    For templated classes, you can directly provide the rtti_name pattern.
    
    Returns the vtable pointer (an integer) or 0 if not found.
    """
    # Use provided RTTI name if available (for templates), otherwise generate it
    if not targetClassRTTIName:
        # Check if this is a templated class
        typeDescriptorName: str = IDAUtils.GetMangledTypePrefix(parentNamespacesClasses, targetClassName)
    else:
        # Use the provided RTTI name directly
        typeDescriptorName: str = targetClassRTTIName
    
    rttiStringsList = IDAUtils.GetIDARTTIStringsList(Config.INPUT_MD5)
    if not rttiStringsList:
        return 0
    typeDescriptorAddr = rttiStringsList.get(typeDescriptorName)
    if not typeDescriptorAddr or typeDescriptorAddr == idc.BADADDR:
        return 0
        
    # Adjust to get RTTI type descriptor
    rttiTypeDescriptorAddr: int = typeDescriptorAddr - 0x10
    xrefsToRTTITypeDescriptor = idautils.DataRefsTo(rttiTypeDescriptorAddr)
    if not xrefsToRTTITypeDescriptor:
        return 0
    
    for xrefToRTTITypeDescriptor in xrefsToRTTITypeDescriptor:
        # Check offset from class
        offsetFromClass: int = idc.get_wide_dword(xrefToRTTITypeDescriptor - 0x8)
        if offsetFromClass:
            continue
            
        # Get object locator
        objectLocatorAddr: int = xrefToRTTITypeDescriptor - 0xC
        xrefsToObjectLocator = idautils.DataRefsTo(objectLocatorAddr)
        if not xrefsToObjectLocator:
            return 0

        for xrefToObjectLocator in xrefsToObjectLocator:
            # Vtable pointer is at (objectLocatorAddr + 0x8)
            vtableAddr: int = xrefToObjectLocator + 0x8
            if vtableAddr <= 0x8:
                break
            return vtableAddr
        
    return 0

@cache
def GetDemangledVTableFuncSigs(inputMD5: bytes, parentNamespacesClasses: tuple[str, ...], targetClassName: str, targetClassRTTIName: str = "") -> list[tuple[str, str]]:
    """
    Get the ordered list of function names from a class's vtable.
    For templated classes, you can provide the rtti_name pattern.
    """
    vtablePtr: int = GetVTablePtr(inputMD5, parentNamespacesClasses, targetClassName, targetClassRTTIName)
    if not vtablePtr:
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
        
        funcSig: str = idc.get_func_name(ptr)
        demangledFuncSig: str = IDAUtils.DemangleSig(funcSig)
        demangledFuncSig = demangledFuncSig if demangledFuncSig else funcSig
        rawType: str = ""

        if not demangledFuncSig:
            ea += 8
            continue
        
        if demangledFuncSig != "_purecall":
            if " " not in demangledFuncSig:
                cfunc = ida_hexrays.decompile(ptr)
                tinfo = idaapi.tinfo_t()
                cfunc.get_func_type(tinfo)
                
                funcType: str = idaapi.print_tinfo('', 0, 0, idaapi.PRTYPE_NOARGS, tinfo, '', '')
                rawType = "IDA_GEN_TYPE " + funcType
            if (demangledFuncSig, rawType) in demangledVTableFuncSigsList:
                demangledFuncSig = "DUPLICATE_FUNC " + demangledFuncSig
        
        demangledVTableFuncSigsList.append((demangledFuncSig, rawType))
        ea += 8
        
    return demangledVTableFuncSigsList