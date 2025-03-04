import os
import idc
import idaapi
import idautils
import ida_hexrays
import ida_nalt
import ida_bytes
import ida_ida
import struct
import re
from typing import Optional, Tuple
from dataclasses import dataclass, field

IDA_NALT_ENCODING = ida_nalt.get_default_encoding_idx(ida_nalt.BPU_1B)
CLASS_TYPES = ("class", "struct", "enum", "union")
FUNC_QUALIFIERS = ("virtual", "static")

# Configuration
INTERNAL_SCRIPT_NAME = "ExportClassToCPPH"
PROJECT_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"
OUTPUT_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\proxies\engine_x64_rwdi\scripts\generated"
CACHE_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\proxies\engine_x64_rwdi\scripts\cache"
GENERATE_CLASS_DEFS_MISSING_TYPES = False   # Flag to generate full class definitions for missing types, or just forward declare if false
SEARCH_CLASS_DEFS_IN_PROJECT_FOLDER = False # Flag to search for missing class types in the PROJECT_FOLDER

virtualFuncPlaceholderCounter: int = 0 # Counter for placeholder virtual functions
virtualFuncDuplicateCounter: dict[str, int] = {} # Counter for duplicate virtual functions

def PrintMsg(*args):
    #ida_kernwin.msg(f"[{INTERNAL_SCRIPT_NAME}] {args}")
    print(f"[{INTERNAL_SCRIPT_NAME}] {args}")

# -----------------------------------------------------------------------------
# String and type formatting utilities
# -----------------------------------------------------------------------------

def FixTypeSpacing(type: str) -> str:
    """Fix spacing for pointers/references, commas, and angle brackets."""
    type = re.sub(r'\s+([*&])', r'\1', type)             # Remove space before '*' or '&'
    type = re.sub(r'([*&])(?!\s)', r'\1 ', type)         # Ensure '*' or '&' is followed by one space if it's not already.
    type = re.sub(r'\s*,\s*', ', ', type)                # Ensure comma followed by one space
    type = re.sub(r'<\s+', '<', type)                    # Remove space after '<'
    type = re.sub(r'\s+>', '>', type)                    # Remove space before '>'
    type = re.sub(r'\s+', ' ', type)                     # Collapse multiple spaces
    return type.strip()

def CleanType(type: str) -> str:
    """Remove unwanted tokens from a type string, then fix spacing."""
    type = re.sub(r'\b(__cdecl|__fastcall|__ptr64|class|struct|enum|union)\b', '', type)
    return FixTypeSpacing(type)

def ReplaceIDATypes(type: str) -> str:
    """Replace IDA types with normal ones"""
    return type.replace("_QWORD", "uint64_t").replace("__int64", "int64_t").replace("unsigned int", "uint32_t")

def ExtractTypesFromString(types: str) -> list[str]:
    """Extract potential type names from a string."""
    # Remove pointer/reference symbols and qualifiers
    cleanedTypes: str = types.replace("*", " ").replace("&", " ")
    cleanedTypesList: list[str] = re.findall(r"[A-Za-z_][\w:]*", cleanedTypes)
    return cleanedTypesList

# -----------------------------------------------------------------------------
# Various class implementations
# -----------------------------------------------------------------------------

@dataclass(frozen=True)
class ClassName:
    """Split a potentially namespaced class name into namespace parts and class name."""
    namespaces: tuple[str] = field(default_factory=tuple)
    name: str = ""
    namespacedName: str = ""
    fullName: str = ""
    type: str = ""

    def __init__(self, fullName: str):
        object.__setattr__(self, "namespaces", ())
        object.__setattr__(self, "name", "")
        object.__setattr__(self, "namespacedName", "")
        object.__setattr__(self, "fullName", "")
        object.__setattr__(self, "type", "")
        
        fullName = (fullName or "").strip()
        if not fullName:
            return
        
        object.__setattr__(self, "fullName", fullName)
        types = ExtractTypesFromString(fullName)
        if len(types) > 1:
            if (types[0] in CLASS_TYPES):
                object.__setattr__(self, "type", types[0])
                fullName = types[1]
            elif (len(types) > 2 and types[0] in FUNC_QUALIFIERS and types[1] in CLASS_TYPES):
                object.__setattr__(self, "type", types[1])
                fullName = types[2]
            else:
                return

        parts = fullName.split("::")
        if len(parts) == 1:
            object.__setattr__(self, "name", parts[0])
            object.__setattr__(self, "namespacedName", self.name)
        else:
            object.__setattr__(self, "namespaces", tuple(parts[:-1]))
            object.__setattr__(self, "name", parts[-1])
            object.__setattr__(self, "namespacedName", f"{'::'.join(self.namespaces)}::{self.name}")

@dataclass(frozen=True)
class ParsedFunction:
    """Parse a demangled function signature and return an instance."""
    fullFuncSig: str = ""
    type: str = ""
    access: str = ""
    returnType: Optional[ClassName] = None
    className: Optional[ClassName] = None
    funcName: str = ""
    params: str = ""
    const: bool = False

    def __init__(self, signature: str, onlyVirtualFuncs: bool):
        global virtualFuncPlaceholderCounter
        global virtualFuncDuplicateCounter

        object.__setattr__(self, "fullFuncSig", signature)
        object.__setattr__(self, "type", "")
        object.__setattr__(self, "access", "")
        object.__setattr__(self, "returnType", None)
        object.__setattr__(self, "className", None)
        object.__setattr__(self, "funcName", "")
        object.__setattr__(self, "params", "")
        object.__setattr__(self, "const", False)

        signature = signature.strip()
        
        isDuplicateFunc: bool = False
        isIDAGeneratedType: bool = False
        isIDAGeneratedTypeParsed: bool = False
        if (signature.startswith("DUPLICATE_FUNC")):
            isDuplicateFunc = True
            signature = signature.removeprefix("DUPLICATE_FUNC").strip()
        if (signature.startswith("IDA_GEN_TYPE")):
            isIDAGeneratedType = True
            signature = signature.removeprefix("IDA_GEN_TYPE").strip()
        elif (signature.startswith("IDA_GEN_PARSED")):
            isIDAGeneratedTypeParsed = True
            signature = signature.removeprefix("IDA_GEN_PARSED").strip()

        access: str = ""
        if signature.startswith("public:"):
            access = "public"
        elif signature.startswith("protected:"):
            access = "protected"
        elif signature.startswith("private:"):
            access = "private"
        signature = signature.removeprefix(f"{access}:").strip()

        # Find parameters and const qualifier
        paramsOpenParenIndex: int = signature.find('(')
        paramsCloseParenIndex: int = signature.rfind(')')
        
        if paramsOpenParenIndex != -1 and paramsCloseParenIndex != -1:
            params: str = signature[paramsOpenParenIndex + 1:paramsCloseParenIndex]

            remainingInputBeforeParamsParen: str = signature[:paramsOpenParenIndex].strip()
            remainingInputAfterParamsParen: str = signature[paramsCloseParenIndex + 1:].strip()
            const: str = "const" if "const" in remainingInputAfterParamsParen else ""
            
            returnType: str = ""
            classAndFuncName: str = ""
            className: str = ""
            funcName: str = ""
            if not isIDAGeneratedType:
                # Find the last space outside of angle brackets
                lastSpaceIndex: int = -1
                lastClassSeparatorIndex: int = -1

                templateDepth: int = 0
                for i in range(len(remainingInputBeforeParamsParen)):
                    if remainingInputBeforeParamsParen[i] == '<':
                        templateDepth += 1
                    elif remainingInputBeforeParamsParen[i] == '>':
                        templateDepth -= 1
                    elif templateDepth == 0 and remainingInputBeforeParamsParen[i] == ' ':
                        lastSpaceIndex = i
                
                if lastSpaceIndex != -1:
                    # Split at the last space outside angle brackets
                    returnType = remainingInputBeforeParamsParen[:lastSpaceIndex].strip()
                    classAndFuncName = remainingInputBeforeParamsParen[lastSpaceIndex+1:].strip()

                    templateDepth = 0
                    # Find the last class separator outside of angle brackets
                    for i in range(len(classAndFuncName)):
                        if classAndFuncName[i] == '<':
                            templateDepth += 1
                        elif classAndFuncName[i] == '>':
                            templateDepth -= 1
                        elif templateDepth == 0 and classAndFuncName[i:i+2] == '::':
                            lastClassSeparatorIndex = i
                    
                    if lastClassSeparatorIndex != -1:
                        className = classAndFuncName[:lastClassSeparatorIndex]
                        funcName = classAndFuncName[lastClassSeparatorIndex+2:]
                    else:
                        className = "::".join(classAndFuncName.split("::")[:-1])
                        funcName = classAndFuncName.split("::")[-1]
                else:
                    templateDepth = 0
                    # Find the last class separator outside of angle brackets
                    for i in range(len(remainingInputBeforeParamsParen)):
                        if remainingInputBeforeParamsParen[i] == '<':
                            templateDepth += 1
                        elif remainingInputBeforeParamsParen[i] == '>':
                            templateDepth -= 1
                        elif templateDepth == 0 and remainingInputBeforeParamsParen[i:i+2] == '::':
                            lastClassSeparatorIndex = i
                
                    if lastClassSeparatorIndex != -1:
                        classAndFuncName: str = remainingInputBeforeParamsParen
                        className: str = classAndFuncName[:lastClassSeparatorIndex]
                        funcName: str = classAndFuncName[lastClassSeparatorIndex+2:]
            else:
                returnType = remainingInputBeforeParamsParen

            if funcName.startswith("~"):
                return
            if isDuplicateFunc:
                if signature not in virtualFuncDuplicateCounter:
                    virtualFuncDuplicateCounter[signature] = 0
                virtualFuncDuplicateCounter[signature] += 1
                funcName = f"_{funcName}{virtualFuncDuplicateCounter[signature]}"

            type = "func" if not (onlyVirtualFuncs or "virtual" in returnType) else ("basic_vfunc" if isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc else "vfunc")
            object.__setattr__(self, "type", type)
            object.__setattr__(self, "access", access if access else "public")
            object.__setattr__(self, "returnType", ClassName(returnType) if returnType else None)
            object.__setattr__(self, "className", ClassName(className) if className else None)
            object.__setattr__(self, "funcName", funcName)
            object.__setattr__(self, "params", params)
            object.__setattr__(self, "const", bool(const))
            return

        # Generate a simple virtual void function
        if onlyVirtualFuncs and signature == "_purecall":
            virtualFuncPlaceholderCounter += 1
            object.__setattr__(self, "type", "stripped_vfunc")
            object.__setattr__(self, "access", access if access else "public")
            object.__setattr__(self, "returnType", ClassName("virtual void"))
            object.__setattr__(self, "funcName", f"_StrippedVFunc{virtualFuncPlaceholderCounter}")

@dataclass(frozen=True)
class ParsedClassVar:
    """Parse a demangled global class var signature and return an instance."""
    access: str = ""
    varType: Optional[ClassName] = None
    className: Optional[ClassName] = None
    varName: str = ""

    def __init__(self, signature: str):
        object.__setattr__(self, "access", "")
        object.__setattr__(self, "varType", None)
        object.__setattr__(self, "className", None)
        object.__setattr__(self, "varName", "")

        signature = signature.strip()
        
        access: str = ""
        if signature.startswith("public:"):
            access = "public"
        elif signature.startswith("protected:"):
            access = "protected"
        elif signature.startswith("private:"):
            access = "private"
        signature = signature.removeprefix(f"{access}:").strip()

        # Find parameters and const qualifier
        paramsOpenParenIndex: int = signature.find('(')
        paramsCloseParenIndex: int = signature.rfind(')')
        
        if paramsOpenParenIndex == -1 and paramsCloseParenIndex == -1:
            varType: str = ""
            classAndVarName: str = ""
            className: str = ""
            varName: str = ""

            # Find the last space outside of angle brackets
            lastSpaceIndex: int = -1
            lastClassSeparatorIndex: int = -1

            templateDepth: int = 0
            for i in range(len(signature)):
                if signature[i] == '<':
                    templateDepth += 1
                elif signature[i] == '>':
                    templateDepth -= 1
                elif templateDepth == 0 and signature[i] == ' ':
                    lastSpaceIndex = i
            
            if lastSpaceIndex != -1:
                # Split at the last space outside angle brackets
                varType = signature[:lastSpaceIndex].strip()
                classAndVarName = signature[lastSpaceIndex+1:].strip()

                templateDepth = 0
                # Find the last class separator outside of angle brackets
                for i in range(len(classAndVarName)):
                    if classAndVarName[i] == '<':
                        templateDepth += 1
                    elif classAndVarName[i] == '>':
                        templateDepth -= 1
                    elif templateDepth == 0 and classAndVarName[i:i+2] == '::':
                        lastClassSeparatorIndex = i
                
                if lastClassSeparatorIndex != -1:
                    className = classAndVarName[:lastClassSeparatorIndex]
                    varName = classAndVarName[lastClassSeparatorIndex+2:]
                else:
                    className = "::".join(classAndVarName.split("::")[:-1])
                    varName = classAndVarName.split("::")[-1]
            else:
                templateDepth = 0
                # Find the last class separator outside of angle brackets
                for i in range(len(signature)):
                    if signature[i] == '<':
                        templateDepth += 1
                    elif signature[i] == '>':
                        templateDepth -= 1
                    elif templateDepth == 0 and signature[i:i+2] == '::':
                        lastClassSeparatorIndex = i
            
                if lastClassSeparatorIndex != -1:
                    classAndVarName: str = signature
                    className: str = classAndVarName[:lastClassSeparatorIndex]
                    varName: str = classAndVarName[lastClassSeparatorIndex+2:]

            object.__setattr__(self, "access", access if access else "public")
            object.__setattr__(self, "varType", ClassName(varType) if varType else None)
            object.__setattr__(self, "className", ClassName(className) if className else None)
            object.__setattr__(self, "varName", varName)
            return

# Global caches
parsedClassVarsByClass: dict[ClassName, list[ParsedClassVar]] = {} # Cache of parsed class vars by class name
parsedVTableFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
parsedFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
allClassVarsAreParsed = False # Flag to indicate if all class vars have been parsed
allFuncsAreParsed = False # Flag to indicate if all functions have been parsed
processedClasses: set[ClassName] = set() # Track classes we've already processed to avoid recursion
template_class_info = {} # Store information about detected template classes

# -----------------------------------------------------------------------------
# IDA util functions
# -----------------------------------------------------------------------------

def DemangleSig(sig: str) -> str:
    return idaapi.demangle_name(sig, idaapi.MNG_LONG_FORM)

def GetMangledTypePrefix(targetClass: ClassName) -> str:
    """
    Get the appropriate mangled type prefix for a class name.
    For class "X" this would be ".?AVX@@"
    For class "NS::X" this would be ".?AVX@NS@@"
    For templated classes, best to use get_mangled_name_for_template instead.
    """
    if not targetClass.namespaces:
        return f".?AV{targetClass.name}@@"
    
    # For namespaced classes, the format is .?AVClassName@Namespace@@
    # For nested namespaces, they are separated with @ in reverse order
    mangledNamespaces = "@".join(reversed(targetClass.namespaces))
    return f".?AV{targetClass.name}@{mangledNamespaces}@@"

# -----------------------------------------------------------------------------
# IDA pattern search utilities
# -----------------------------------------------------------------------------

def BytesToIDAPattern(data: bytes) -> str:
    """Convert bytes to IDA-friendly hex pattern string."""
    return " ".join("{:02X}".format(b) for b in data)

def GetSectionInfo(sectionName: str) -> Tuple[int, int]:
    """Get start address and size of a specified section."""
    for seg_ea in idautils.Segments():
        if idc.get_segm_name(seg_ea) == sectionName:
            start = seg_ea
            end = idc.get_segm_end(seg_ea)
            return start, end - start
    return 0, 0

def FindAllPatternsInRange(pattern: str, start: int, size: int) -> list[int]:
    """Find all occurrences of a pattern within a memory range."""
    addresses: list[int] = []
    ea: int = start
    end: int = start + size
    
    while ea < end:
        compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
        errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, pattern, 16, IDA_NALT_ENCODING)
        if errorParsingIDAPattern:
            return []
            
        patternAddr: int = ida_bytes.bin_search(ea, end, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
        if patternAddr == idc.BADADDR:
            break
            
        addresses.append(patternAddr)
        ea = patternAddr + 8  # advance past found pattern
        
    return addresses

# -----------------------------------------------------------------------------
# RTTI and vtable analysis
# -----------------------------------------------------------------------------

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
        typeDescriptorName: str = GetMangledTypePrefix(targetClass)
    else:
        # Use the provided RTTI name directly
        typeDescriptorName: str = targetClassRTTIName
    
    # Search for the RTTI type descriptor
    typeDescriptorBytes: bytes = typeDescriptorName.encode('ascii')
    idaPattern: str = BytesToIDAPattern(typeDescriptorBytes)
    
    # Search in .rdata
    rdataStartAddr, rdataSize = GetSectionInfo(".rdata")
    if not rdataStartAddr:
        return 0
        
    # Look for the type descriptor
    compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
    errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, idaPattern, 16, IDA_NALT_ENCODING)
    if errorParsingIDAPattern:
        return 0
        
    typeDescriptorPatternAddr: int = ida_bytes.bin_search(rdataStartAddr, ida_ida.cvar.inf.max_ea, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
    if typeDescriptorPatternAddr == idc.BADADDR:
        PrintMsg(f"Type descriptor pattern '{typeDescriptorName}' not found for {targetClass.fullName}.\n")
        return 0
        
    # Adjust to get RTTI type descriptor
    rttiTypeDescriptorAddr: int = typeDescriptorPatternAddr - 0x10
    
    # Compute offset relative to base address
    rttiTypeDescriptorOffset: int = rttiTypeDescriptorAddr - baseDLLAddr
    rttiTypeDescriptorOffsetBytes: bytes = struct.pack("<I", rttiTypeDescriptorOffset)
    rttiTypeDescriptorOffsetPattern: str = BytesToIDAPattern(rttiTypeDescriptorOffsetBytes)
    
    # Search for references to this offset
    xrefs: list[int] = FindAllPatternsInRange(rttiTypeDescriptorOffsetPattern, rdataStartAddr, rdataSize)
    
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
        objectLocatorPattern: str = BytesToIDAPattern(objectLocatorBytes)
        
        compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
        errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, objectLocatorPattern, 16, IDA_NALT_ENCODING)
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
        
    PrintMsg(f"Failed to locate vtable pointer for {targetClass.fullName}.\n")
    return 0

# -----------------------------------------------------------------------------
# Function collection and parsing
# -----------------------------------------------------------------------------

def CreateParamNamesForVTFunc(parsedFunc: ParsedFunction, skipFirstParam: bool) -> str:
    paramsList: list[str] = [param.strip() for param in parsedFunc.params.split(',')
                   if param.strip()]
    if len(paramsList) == 1 and paramsList[0] == "void":
        return "void"
    # Skip the first parameter (typically the "this" pointer)
    if skipFirstParam:
        paramsList = paramsList[1:]
    paramsList = [FixTypeSpacing(param.strip()) for param in paramsList]
    
    paramNames: list[str] = [f"a{i+1}" for i in range(len(paramsList))]
    newParams: str = ", ".join(f"{paramType} {paramName}" for paramType, paramName in zip(paramsList, paramNames))
    return newParams

def ExtractParamNames(params: str) -> str:
    paramsList: list[str] = [param.strip() for param in params.split(',')
                   if param.strip()]
    if len(paramsList) == 1 and paramsList[0] == "void":
        return ""
    
    paramNames: list[str] = [param.split(" ")[-1].strip() for param in paramsList]
    newParams: str = ", ".join(paramNames)
    return newParams

def GetDemangledExportedSigs() -> list[str]:
    """
    Generate a list of demangled function sigs from IDA's database
    """
    cachedFilePath = os.path.join(CACHE_FOLDER, "demangled_exported_sigs_cache.txt")
    
    if os.path.exists(cachedFilePath):
        try:
            with open(cachedFilePath, "r") as cachedFile:
                return [line.strip() for line in cachedFile if line.strip()]
        except Exception as e:
            PrintMsg(f"Error opening '{cachedFilePath}' for reading: {e}\n")
        
    demangledExportedSigsList: list[str] = []

    for i in range(idc.get_entry_qty()):
        ea: int = idc.get_entry(i)
        exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
        if not exportedSig:
            continue
        demangledExportedSig: str = DemangleSig(exportedSig)
        if not demangledExportedSig:
            continue

        if demangledExportedSig not in demangledExportedSigsList:
            demangledExportedSigsList.append(demangledExportedSig)

    os.makedirs(CACHE_FOLDER, exist_ok=True)
    try:
        with open(cachedFilePath, "w") as cachedFile:
            for demangledExportedSig in demangledExportedSigsList:
                cachedFile.write(demangledExportedSig + "\n")
    except Exception as e:
        PrintMsg(f"Error opening '{cachedFilePath}' for writing: {e}\n")
    
    return demangledExportedSigsList

def GetParsedClassVars(targetClass: Optional[ClassName] = None) -> list[ParsedClassVar]:
    """
    Collect and parse all class var signatures from the IDA database.
    If target_class is provided, only return class vars for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedClassVarsByClass, allClassVarsAreParsed, parsedFuncsByClass
    
    if not allClassVarsAreParsed:
        for demangledExportedSig in GetDemangledExportedSigs():
            isAlreadyPresentInParsedFuncs: bool = False
            for parsedFuncList in list(parsedFuncsByClass.values()):
                for parsedFunc in parsedFuncList:
                    if demangledExportedSig in parsedFunc.fullFuncSig:
                        isAlreadyPresentInParsedFuncs = True
                        break
                if isAlreadyPresentInParsedFuncs:
                    break
            if isAlreadyPresentInParsedFuncs:
                continue

            # Skip invalid functions
            if demangledExportedSig.endswith("::$TSS0") or "::`vftable'" in demangledExportedSig:
                continue

            parsedClassVar: ParsedClassVar = ParsedClassVar(demangledExportedSig)
            if not parsedClassVar.className or not parsedClassVar.varName:
                PrintMsg(f"Failed parsing class var sig: \"{demangledExportedSig}\"")
                continue
            
            if parsedClassVar.className not in parsedClassVarsByClass:
                parsedClassVarsByClass[parsedClassVar.className] = []
            parsedClassVarsByClass[parsedClassVar.className].append(parsedClassVar)
        
        allClassVarsAreParsed = True
    
    # Return the requested functions
    if not targetClass:
        # Return all parsed functions
        allClassVars: list[ParsedClassVar] = []
        for classVars in parsedClassVarsByClass.values():
            allClassVars.extend(classVars)
        return allClassVars
    else:
        # Return only functions for the specified class
        return parsedClassVarsByClass.get(targetClass, [])

def GetDemangledVTableFuncSigs(targetClass: ClassName, targetClassRTTIName: str = "") -> list[tuple[str, str]]:
    """
    Get the ordered list of function names from a class's vtable.
    For templated classes, you can provide the rtti_name pattern.
    """
    vtablePtr: int = GetVTablePtr(targetClass, targetClassRTTIName)
    if not vtablePtr:
        idaapi.msg(f"Vtable pointer not found for {targetClass.fullName}.\n")
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
        demangledFuncSig: str = DemangleSig(funcSig)
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
                demangledFuncSig = "DUPLICATE_FUNC " + demangledFuncSig# if not rawType else demangledFuncSig
                #rawType = "DUPLICATE_FUNC " + rawType if rawType else rawType
        
        demangledVTableFuncSigsList.append((demangledFuncSig, rawType))
        ea += 8
        
    return demangledVTableFuncSigsList

def GetParsedVTableFuncs(targetClass: ClassName) -> list[ParsedFunction]:
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedVTableFuncsByClass
    
    if targetClass not in parsedVTableFuncsByClass:
        parsedVTableFuncsByClass[targetClass] = []
        
        for (demangledFuncSig, rawType) in GetDemangledVTableFuncSigs(targetClass):
            if rawType:
                parsedFunc: ParsedFunction = ParsedFunction(rawType, True)
                if parsedFunc.returnType:
                    newParamTypes: str = CreateParamNamesForVTFunc(parsedFunc, True) if parsedFunc.params else ""
                    demangledFuncSig = f"{'DUPLICATE_FUNC ' if demangledFuncSig.startswith('DUPLICATE_FUNC') else ''}IDA_GEN_PARSED virtual {parsedFunc.returnType.fullName} {demangledFuncSig.removeprefix('DUPLICATE_FUNC').strip()}({newParamTypes})"
            elif demangledFuncSig.startswith("DUPLICATE_FUNC"):
                parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig.removeprefix("DUPLICATE_FUNC").strip(), True)
                if parsedFunc.returnType:
                    newParamTypes: str = CreateParamNamesForVTFunc(parsedFunc, False) if parsedFunc.params else ""
                    demangledFuncSig = f"DUPLICATE_FUNC {parsedFunc.returnType.fullName} {parsedFunc.funcName}({newParamTypes})"

            parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig, True)
            if not parsedFunc.className:
                object.__setattr__(parsedFunc, "className", targetClass)
            
            parsedVTableFuncsByClass[targetClass].append(parsedFunc)
        
    return parsedVTableFuncsByClass.get(targetClass, [])

def GetParsedFuncs(targetClass: Optional[ClassName] = None) -> list[ParsedFunction]:
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedFuncsByClass, allFuncsAreParsed
    
    if not allFuncsAreParsed:
        for demangledFuncSig in GetDemangledExportedSigs():
            # Skip invalid functions
            if demangledFuncSig.endswith("::$TSS0") or "::`vftable'" in demangledFuncSig:
                continue

            parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig, False)
            if not parsedFunc.type or not parsedFunc.className:
                PrintMsg(f"Failed parsing func sig: \"{demangledFuncSig}\"")
                continue
            
            if parsedFunc.className not in parsedFuncsByClass:
                parsedFuncsByClass[parsedFunc.className] = []
            parsedFuncsByClass[parsedFunc.className].append(parsedFunc)
        
        allFuncsAreParsed = True
    
    # Return the requested functions
    if not targetClass:
        # Return all parsed functions
        allFuncs: list[ParsedFunction] = []
        for funcs in parsedFuncsByClass.values():
            allFuncs.extend(funcs)
        return allFuncs
    else:
        # Return only functions for the specified class
        return parsedFuncsByClass.get(targetClass, [])

# -----------------------------------------------------------------------------
# Header generation
# -----------------------------------------------------------------------------

currentAccess: str = "public"
def GenerateClassVarCode(classVar: ParsedClassVar, cleanedTypes: bool = True) -> str:
    """Generate code for a single class method."""
    global currentAccess

    access: str = f"{classVar.access}:\n    " if classVar.access else "    "
    if currentAccess == classVar.access:
        access = "    "
    else:
        currentAccess = classVar.access

    if classVar.varType:
        varType: str = ReplaceIDATypes(classVar.varType.fullName)
        varType = CleanType(varType) if cleanedTypes else classVar.varType.fullName
        if varType:
            varType += " "
    else:
        varType: str = ""
        varType = "GAME_IMPORT " + varType
    
    classVarSig: str = f"{varType}{classVar.varName}"
    return f"{access}{classVarSig};"

def GenerateClassFuncCode(func: ParsedFunction, cleanedTypes: bool = True, vtFuncIndex: int = 0) -> str:
    """Generate code for a single class method."""
    global currentAccess

    access: str = f"{func.access}:\n    " if func.access else "    "
    if currentAccess == func.access:
        access = "    "
    else:
        currentAccess = func.access

    const: str = " const" if func.const else ""
    stripped_vfunc: str = " = 0" if func.type == "stripped_vfunc" else ""

    if func.returnType:
        returnType: str = ReplaceIDATypes(func.returnType.fullName)
        returnType = CleanType(returnType) if cleanedTypes else func.returnType.fullName
        if returnType:
            if func.type == "basic_vfunc":
                returnType = returnType.removeprefix("virtual").strip()
            else:
                returnType += " "
    else:
        returnType: str = ""
    if func.type != "stripped_vfunc" and func.type != "basic_vfunc":
        returnType = "GAME_IMPORT " + returnType
    
    if func.params:
        params: str = ReplaceIDATypes(func.params)
        params = CleanType(params) if cleanedTypes else func.params
        if params == "void":
            params = ""
    else:
        params: str = ""

    targetParams: str = ""
    if func.type == "basic_vfunc":
        targetParams = ExtractParamNames(params)
        targetParams = ", " + targetParams if targetParams else ""

    funcSig: str = f"{returnType}{func.funcName}({params}){const}{stripped_vfunc}" if func.type != "basic_vfunc" else f"VIRTUAL_CALL({vtFuncIndex}, {returnType}, {func.funcName}, ({params}){targetParams})"
    return f"{access}{funcSig};"

def GenerateClassDefinition(targetClass: ClassName, allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool = True) -> str:
    """Generate a class definition from a list of methods."""
    # Build the class definition
    if not allParsedClassVarsAndFuncs[0] and not allParsedClassVarsAndFuncs[1] and not allParsedClassVarsAndFuncs[2]:
        return ""
    
    if not targetClass.type:
        targetClassType: str = ""
        for parsedClassVarsList in allParsedClassVarsAndFuncs[1:]:
            for parsedClassVar in parsedClassVarsList:
                if parsedClassVar.returnType and parsedClassVar.returnType.namespacedName == targetClass.namespacedName and parsedClassVar.returnType.type:
                    targetClassType = parsedClassVar.returnType.type
                    object.__setattr__(targetClass, "type", targetClassType)
                    break
            if targetClassType:
                break
        if not targetClassType:
            for parsedClassVar in allParsedClassVarsAndFuncs[0]:
                if parsedClassVar.varType and parsedClassVar.varType.namespacedName == targetClass.namespacedName and parsedClassVar.varType.type:
                    targetClassType = parsedClassVar.varType.type
                    object.__setattr__(targetClass, "type", targetClassType)
                    break
    
    classLines: list[str] = [f"{targetClass.type if targetClass.type else 'class'} {targetClass.name} {{", "    #pragma region GENERATED by ExportClassToCPPH.py"]
    
    firstVarOrFuncAccess: str = ""
    for classVar in allParsedClassVarsAndFuncs[0]:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = classVar.access
        classLines.append(GenerateClassVarCode(classVar, cleanedTypes))
    if allParsedClassVarsAndFuncs[0] and allParsedClassVarsAndFuncs[1] and allParsedClassVarsAndFuncs[2]:
        classLines.append("")
    for index, vTableFunc in enumerate(allParsedClassVarsAndFuncs[1]):
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = vTableFunc.access
        classLines.append(GenerateClassFuncCode(vTableFunc, cleanedTypes, index))
    if allParsedClassVarsAndFuncs[0] and allParsedClassVarsAndFuncs[1] and allParsedClassVarsAndFuncs[2]:
        classLines.append("")
    for func in allParsedClassVarsAndFuncs[2]:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = func.access
        classLines.append(GenerateClassFuncCode(func, cleanedTypes))
        
    classLines.append("    #pragma endregion")
    classLines.append("};")
    # Insert first function access if there is any, otherwise just make it public by default
    classLines.insert(2, f"{firstVarOrFuncAccess if firstVarOrFuncAccess else 'public'}:")

    return "\n".join(classLines)

def GenerateHeaderCode(targetClass: ClassName, allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool = True) -> str:
    """
    Generate a C++ header file for the target class.
    Organizes methods with vtable order first, then remaining methods.
    Also handles dependencies by generating classes for missing types.
    """
    # Generate the class definition
    classDefinition: str = GenerateClassDefinition(targetClass, allParsedClassVarsAndFuncs, cleanedTypes)
    if not classDefinition:
        return ""

    # Combine all parts of the header
    headerParts = ["#pragma once\n", r"#include <EGSDK\Imports.h>", "\n"]
    # Add the main class definition
    headerParts.append("\n" + classDefinition)
    
    return "".join(headerParts)

# -----------------------------------------------------------------------------
# Main functionality
# -----------------------------------------------------------------------------

def GetAllParsedClassVarsAndFuncs(targetClass: ClassName) -> tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]]:
    parsedVTableClassFuncs: list[ParsedFunction] = GetParsedVTableFuncs(targetClass)
    if not parsedVTableClassFuncs:
        PrintMsg(f"No matching VTable function signatures were found for {targetClass.fullName}.\n")

    parsedClassFuncs: list[ParsedFunction] = GetParsedFuncs(targetClass)
    if not parsedClassFuncs:
        PrintMsg(f"No matching function signatures were found for {targetClass.fullName}.\n")

    parsedClassVars: list[ParsedClassVar] = GetParsedClassVars(targetClass)
    if not parsedClassVars:
        PrintMsg(f"No matching class var signatures were found for {targetClass.fullName}.\n")

    # Get non-vtable methods
    finalParsedClassFuncs: list[ParsedFunction] = [
        parsedFunc for parsedFunc in parsedClassFuncs
        if parsedFunc not in parsedVTableClassFuncs
    ]
    
    return (parsedClassVars, parsedVTableClassFuncs, finalParsedClassFuncs)

def WriteHeaderToFile(targetClass: ClassName, headerCode: str, fileName: str = "") -> bool:
    if targetClass.namespaces:
        # Create folder structure for namespaces
        classFolderPath: str = os.path.join(*targetClass.namespaces)
        outputFolderPath: str = os.path.join(OUTPUT_FOLDER, classFolderPath)
        
        # Create directory if it doesn't exist
        os.makedirs(outputFolderPath, exist_ok=True)
        
        # Output file path is inside the namespace folder
        outputFilePath: str = os.path.join(classFolderPath, f"{targetClass.name}.h" if not fileName else fileName)
    else:
        # No namespace, just save in current directory
        outputFilePath: str = f"{targetClass.name}.h" if not fileName else fileName
    
    outputFilePath: str = os.path.join(OUTPUT_FOLDER, outputFilePath)

    try:
        with open(outputFilePath, 'w') as headerFile:
            headerFile.write(headerCode)
        PrintMsg(f"Header file '{outputFilePath}' created successfully.\n")
        
        return True
    except Exception as e:
        PrintMsg(f"Error writing header file '{outputFilePath}': {e}\n")
        return False

def ExportClassHeader(targetClass: ClassName):
    """
    Generate and save a C++ header file for the target class.
    For namespaced classes, creates appropriate folder structure.
    """
    global processedClasses
    processedClasses = set()
    # Add the target class to processed classes to prevent recursion
    processedClasses.add(targetClass)

    allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]] = GetAllParsedClassVarsAndFuncs(targetClass)

    headerCode: str = GenerateHeaderCode(targetClass, allParsedClassVarsAndFuncs)
    if not headerCode:
        PrintMsg(f"No functions were found for class {targetClass.fullName}, therefore will not generate.")
        return
    WriteHeaderToFile(targetClass, headerCode)

    nonCleanedHeaderCode: str = GenerateHeaderCode(targetClass, allParsedClassVarsAndFuncs, False)
    WriteHeaderToFile(targetClass, nonCleanedHeaderCode, f"{targetClass.name}-unclean.h")

def Main():
    """Main entry point for the script."""
    # Ask user for target class
    #targetClass = ida_kernwin.ask_str("IModelObject", 0, "Enter target class name (supports namespaces and templates):")
    targetClassName: str = "CModelObject"
    if not targetClassName:
        PrintMsg("No target class specified. Aborting.\n")
        return
    targetClass: ClassName = ClassName(targetClassName)

    ExportClassHeader(targetClass)

# -----------------------------------------------------------------------------
# IDA plugin integration
# -----------------------------------------------------------------------------

class ExportClassToCPPH(idaapi.plugin_t):
    """IDA Pro plugin for exporting C++ class definitions to header files."""
    flags = idaapi.PLUGIN_UNL
    comment = "Extract exported and virtual functions and generate a C++ header"
    help = "Extracts exported and virtual functions and generates a C++ header"
    wanted_name = "Export Class to C++ Header"
    wanted_hotkey = "Shift-F"
    
    def init(self):
        PrintMsg(f"Initializing \"{self.wanted_name}\" Plugin\n")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        PrintMsg(f"Running \"{self.wanted_name}\" Plugin\n")
        Main()

    def term(self):
        PrintMsg(f"Terminating \"{self.wanted_name}\" Plugin\n")
        pass

def PLUGIN_ENTRY():
    idaapi.msg(f"Creating \"Export Class to C++ Header\" Plugin entry\n")
    return ExportClassToCPPH()

if __name__ == "__main__":
    Main()