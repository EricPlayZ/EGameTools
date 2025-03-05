import os
import struct
import idc
import idaapi
import ida_hexrays
import ida_bytes
import ida_ida
import cloudpickle
from dataclasses import dataclass, field
from typing import Optional

idaapi.require("ExportClassToCPPH")
from ExportClassToCPPH import Utils, Config

def reconstruct_ClassName(namespaces, name, namespacedName, fullName, type_str):
    # Bypass __init__ by creating a new instance directly.
    obj = object.__new__(ClassName)
    object.__setattr__(obj, "namespaces", namespaces)
    object.__setattr__(obj, "name", name)
    object.__setattr__(obj, "namespacedName", namespacedName)
    object.__setattr__(obj, "fullName", fullName)
    object.__setattr__(obj, "type", type_str)
    return obj
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
        types = Utils.ExtractTypesFromString(fullName)
        if len(types) > 1:
            if (types[0] in Utils.CLASS_TYPES):
                object.__setattr__(self, "type", types[0])
                fullName = types[1]
            elif (len(types) > 2 and types[0] in Utils.FUNC_QUALIFIERS and types[1] in Utils.CLASS_TYPES):
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
    
    def __reduce__(self):
        return (reconstruct_ClassName, (self.namespaces, self.name, self.namespacedName, self.fullName, self.type))

virtualFuncPlaceholderCounter: int = 0 # Counter for placeholder virtual functions
virtualFuncDuplicateCounter: dict[str, int] = {} # Counter for duplicate virtual functions

def reconstruct_ParsedFunction(fullFuncSig, type, access, returnType, className, funcName, params, const):
    # Bypass __init__ by creating a new instance directly.
    obj = object.__new__(ParsedFunction)
    object.__setattr__(obj, "fullFuncSig", fullFuncSig)
    object.__setattr__(obj, "type", type)
    object.__setattr__(obj, "access", access)
    object.__setattr__(obj, "returnType", returnType)
    object.__setattr__(obj, "className", className)
    object.__setattr__(obj, "funcName", funcName)
    object.__setattr__(obj, "params", params)
    object.__setattr__(obj, "const", const)
    return obj
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

            if isDuplicateFunc:
                if signature not in virtualFuncDuplicateCounter:
                    virtualFuncDuplicateCounter[signature] = 0
                virtualFuncDuplicateCounter[signature] += 1
                funcName = f"_{funcName}{virtualFuncDuplicateCounter[signature]}"

            if onlyVirtualFuncs:
                if isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc or "virtual" not in returnType:
                    type = "basic_vfunc"
                else:
                    type = "vfunc"
            else:
                if "virtual" not in returnType:
                    type = "func"
                elif isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc:
                    type = "basic_vfunc"
                else:
                    type = "vfunc"
            #type = "func" if not (onlyVirtualFuncs or "virtual" in returnType) else ("basic_vfunc" if isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc else "vfunc")
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
    
    def __reduce__(self):
        return (reconstruct_ParsedFunction, (self.fullFuncSig, self.type, self.access, self.returnType, self.className, self.funcName, self.params, self.const))

def reconstruct_ParsedClassVar(access, varType, className, varName):
    # Bypass __init__ by creating a new instance directly.
    obj = object.__new__(ParsedClassVar)
    object.__setattr__(obj, "access", access)
    object.__setattr__(obj, "varType", varType)
    object.__setattr__(obj, "className", className)
    object.__setattr__(obj, "varName", varName)
    return obj
@dataclass(frozen=True)
class ParsedClassVar:
    """Parse a demangled global class var signature and return an instance."""
    access: str = ""
    varType: Optional[ClassName] = None
    className: Optional[ClassName] = None
    varName: str = ""

    def __init__(self, signature: str):
        # Initialize defaults.
        object.__setattr__(self, "access", "")
        object.__setattr__(self, "varType", None)
        object.__setattr__(self, "className", None)
        object.__setattr__(self, "varName", "")
        
        signature = signature.strip()
        
        # Extract access specifier.
        access = ""
        for kw in ("public:", "protected:", "private:"):
            if signature.startswith(kw):
                access = kw[:-1]  # remove the colon
                signature = signature[len(kw):].strip()
                break

        # For class variables, we expect no parameters (i.e. no parentheses).
        if signature.find('(') == -1 and signature.rfind(')') == -1:
            # Use a backward search to find the last space outside templates.
            last_space = Utils.FindLastSpaceOutsideTemplates(signature)
            if last_space != -1:
                varType = signature[:last_space].strip()
                classAndVarName = signature[last_space+1:].strip()
            else:
                # If no space, assume there's no varType
                varType = ""
                classAndVarName = signature
            
            # Find the last "::" separator outside templates.
            last_sep = Utils.FindLastClassSeparatorOutsideTemplates(classAndVarName)
            if last_sep != -1:
                class_name_str = classAndVarName[:last_sep].strip()
                var_name = classAndVarName[last_sep+2:].strip()
            else:
                # Fallback: if there are "::" tokens, split them; otherwise, take entire string as varName.
                parts = classAndVarName.split("::")
                if len(parts) > 1:
                    class_name_str = "::".join(parts[:-1]).strip()
                    var_name = parts[-1].strip()
                else:
                    class_name_str = ""
                    var_name = classAndVarName.strip()
            
            object.__setattr__(self, "access", access if access else "public")
            object.__setattr__(self, "varType", ClassName(varType) if varType else None)
            object.__setattr__(self, "className", ClassName(class_name_str) if class_name_str else None)
            object.__setattr__(self, "varName", var_name)
            return
        
    def __reduce__(self):
        return (reconstruct_ParsedClassVar, (self.access, self.varType, self.className, self.varName))

# Global caches
parsedClassVarsByClass: dict[ClassName, list[ParsedClassVar]] = {} # Cache of parsed class vars by class name
parsedVTableFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
parsedFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
unparsedExportedSigs: list[str] = []
allClassVarsAreParsed = False # Flag to indicate if all class vars have been parsed
allFuncsAreParsed = False # Flag to indicate if all functions have been parsed

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
        print(f"Type descriptor pattern '{typeDescriptorName}' not found for {targetClass.fullName}.")
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
        
    print(f"Failed to locate vtable pointer for {targetClass.fullName}.")
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
    paramsList = [Utils.FixTypeSpacing(param.strip()) for param in paramsList]
    
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

def ComputeUnparsedExportedSigs(demangledExportedSigs: list[str], parsedSigs: list[str]) -> list[str]:
    # Join all parsed signatures into one large string.
    big_parsed = "\n".join(parsedSigs)
    # Then, for each exported signature, check if it appears in the big string.
    return [sig for sig in demangledExportedSigs if sig not in big_parsed]
    # # Start with a set of all exported signatures.
    # unparsed = set(demangledExportedSigs)
    # # For each parsed function signature, remove any exported signature that is a substring.
    # for ps in parsedSigs:
    #     # Create a temporary list of matching exported signatures to remove
    #     toRemove = [sig for sig in unparsed if sig in ps]
    #     for sig in toRemove:
    #         unparsed.discard(sig)
    #     # If the set becomes empty, we can break early.
    #     if not unparsed:
    #         break
    # return list(unparsed)

def GetDemangledExportedSigs() -> list[str]:
    """
    Generate a list of demangled function signatures from IDA's database.
    Uses a set to avoid duplicate entries.
    """
    sigs_set = set()
    entry_qty = idc.get_entry_qty()
    for i in range(entry_qty):
        ea: int = idc.get_entry(i)
        exportedSig: str = idc.get_func_name(ea) or idc.get_name(ea)
        if not exportedSig:
            continue
        demangledExportedSig: str = Utils.DemangleSig(exportedSig)
        if demangledExportedSig and "~" not in demangledExportedSig:
            sigs_set.add(demangledExportedSig)
    return list(sigs_set)

def GetParsedClassVars(targetClass: Optional[ClassName] = None) -> list[ParsedClassVar]:
    """
    Collect and parse all class var signatures from the IDA database.
    If target_class is provided, only return class vars for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedClassVarsByClass, allClassVarsAreParsed, parsedFuncsByClass, unparsedExportedSigs

    if not allClassVarsAreParsed:
        # Attempt to load from cache
        if os.path.exists(Config.PARSED_VARS_CACHE_FILENAME):
            try:
                with open(Config.PARSED_VARS_CACHE_FILENAME, "rb") as cache_file:
                    parsedClassVarsByClass = cloudpickle.load(cache_file)
                allClassVarsAreParsed = True
                print(f"Loaded cached class variables from \"{Config.PARSED_VARS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to load cache from \"{Config.PARSED_VARS_CACHE_FILENAME}\": {e}")

        # If cache not loaded, parse from unparsed signatures.
        if not allClassVarsAreParsed:
            # Build the list of unparsed exported signatures only once
            if not unparsedExportedSigs:
                demangledExportedSigs = GetDemangledExportedSigs()
                # Precompute a flat list of all parsed function signatures.
                parsedSigs = [pf.fullFuncSig for funcList in parsedFuncsByClass.values() for pf in funcList]
                unparsedExportedSigs = ComputeUnparsedExportedSigs(demangledExportedSigs, parsedSigs)
            # Use existing unparsedExportedSigs if available; otherwise, generate them.
            sigs = unparsedExportedSigs if unparsedExportedSigs else GetDemangledExportedSigs()

            for sig in sigs:
                # Skip invalid signatures
                if sig.endswith("::$TSS0") or "::`vftable'" in sig:
                    continue

                parsedVar = ParsedClassVar(sig)
                if not parsedVar.className or not parsedVar.varName:
                    print(f"Failed parsing class var sig: \"{sig}\"")
                    continue

                parsedClassVarsByClass.setdefault(parsedVar.className, []).append(parsedVar)

            allClassVarsAreParsed = True

            # Cache the parsed class variables
            try:
                # Create directory if it doesn't exist
                os.makedirs(Config.CACHE_OUTPUT_PATH, exist_ok=True)
                with open(Config.PARSED_VARS_CACHE_FILENAME, "wb") as cache_file:
                    cloudpickle.dump(parsedClassVarsByClass, cache_file)
                print(f"Cached class variables to \"{Config.PARSED_VARS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to write cache to \"{Config.PARSED_VARS_CACHE_FILENAME}\": {e}")

    # Return all class variables or only those for the target class
    if targetClass is None:
        return [var for vars_list in parsedClassVarsByClass.values() for var in vars_list]
    else:
        return parsedClassVarsByClass.get(targetClass, [])

def GetDemangledVTableFuncSigs(targetClass: ClassName, targetClassRTTIName: str = "") -> list[tuple[str, str]]:
    """
    Get the ordered list of function names from a class's vtable.
    For templated classes, you can provide the rtti_name pattern.
    """
    vtablePtr: int = GetVTablePtr(targetClass, targetClassRTTIName)
    if not vtablePtr:
        print(f"Vtable pointer not found for {targetClass.fullName}.")
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
    If targetClass is provided, only return functions for that class.
    Caches results in a file for better performance on subsequent calls.
    Also builds a list of unparsed exported signatures for later use.
    """
    global parsedFuncsByClass, allFuncsAreParsed, unparsedExportedSigs

    # Attempt to load cache if we haven't parsed everything yet.
    if not allFuncsAreParsed:
        if os.path.exists(Config.PARSED_FUNCS_CACHE_FILENAME):
            try:
                with open(Config.PARSED_FUNCS_CACHE_FILENAME, "rb") as cache_file:
                    breakpoint()
                    parsedFuncsByClass = cloudpickle.load(cache_file)
                allFuncsAreParsed = True
                print(f"Loaded cached parsed functions from \"{Config.PARSED_FUNCS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to load cache from \"{Config.PARSED_FUNCS_CACHE_FILENAME}\": {e}")
        # If no cache was loaded, parse the signatures
        if not allFuncsAreParsed:
            demangledExportedSigs = GetDemangledExportedSigs()
            for demangledFuncSig in demangledExportedSigs:
                # Skip known invalid functions
                if demangledFuncSig.endswith("::$TSS0") or "::`vftable'" in demangledFuncSig:
                    continue
                parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig, False)
                if not parsedFunc.type or not parsedFunc.className:
                    print(f"Failed parsing func sig: \"{demangledFuncSig}\"")
                    continue
                parsedFuncsByClass.setdefault(parsedFunc.className, []).append(parsedFunc)
            allFuncsAreParsed = True
            try:
                os.makedirs(Config.CACHE_OUTPUT_PATH, exist_ok=True)
                with open(Config.PARSED_FUNCS_CACHE_FILENAME, "wb") as cache_file:
                    cloudpickle.dump(parsedFuncsByClass, cache_file)
                print(f"Cached parsed functions to \"{Config.PARSED_FUNCS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to write cache to \"{Config.PARSED_FUNCS_CACHE_FILENAME}\": {e}")

    # Return functions based on targetClass if specified
    if targetClass is None:
        return [pf for funcList in parsedFuncsByClass.values() for pf in funcList]
    else:
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
        varType: str = Utils.ReplaceIDATypes(classVar.varType.fullName)
        varType = Utils.CleanType(varType) if cleanedTypes else classVar.varType.fullName
        if varType:
            varType += " "
        varType = "GAME_IMPORT " + varType
    else:
        varType: str = ""
    
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
        returnType: str = Utils.ReplaceIDATypes(func.returnType.fullName)
        returnType = Utils.CleanType(returnType) if cleanedTypes else func.returnType.fullName
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
        params: str = Utils.ReplaceIDATypes(func.params)
        params = Utils.CleanType(params) if cleanedTypes else func.params
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
    if allParsedClassVarsAndFuncs[0] and (allParsedClassVarsAndFuncs[1] or allParsedClassVarsAndFuncs[2]):
        classLines.append("")
    for index, vTableFunc in enumerate(allParsedClassVarsAndFuncs[1]):
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = vTableFunc.access
        classLines.append(GenerateClassFuncCode(vTableFunc, cleanedTypes, index))
    if (allParsedClassVarsAndFuncs[0] or allParsedClassVarsAndFuncs[1]) and allParsedClassVarsAndFuncs[2]:
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
        print(f"No matching VTable function signatures were found for {targetClass.fullName}.")

    parsedClassFuncs: list[ParsedFunction] = GetParsedFuncs(targetClass)
    if not parsedClassFuncs:
        print(f"No matching function signatures were found for {targetClass.fullName}.")

    parsedClassVars: list[ParsedClassVar] = GetParsedClassVars(targetClass)
    if not parsedClassVars:
        print(f"No matching class var signatures were found for {targetClass.fullName}.")

    # Get non-vtable methods
    vTableFuncsSet: set[str] = {pf.fullFuncSig for pf in parsedVTableClassFuncs}
    finalParsedClassFuncs: list[ParsedFunction] = [
        parsedFunc for parsedFunc in parsedClassFuncs
        if parsedFunc.fullFuncSig not in vTableFuncsSet
    ]
    
    return (parsedClassVars, parsedVTableClassFuncs, finalParsedClassFuncs)

def WriteHeaderToFile(targetClass: ClassName, headerCode: str, fileName: str = "") -> bool:
    if targetClass.namespaces:
        # Create folder structure for namespaces
        classFolderPath: str = os.path.join(*targetClass.namespaces)
        outputFolderPath: str = os.path.join(Config.HEADER_OUTPUT_PATH, classFolderPath)
        
        # Create directory if it doesn't exist
        os.makedirs(outputFolderPath, exist_ok=True)
        
        # Output file path is inside the namespace folder
        outputFilePath: str = os.path.join(classFolderPath, f"{targetClass.name}.h" if not fileName else fileName)
    else:
        # No namespace, just save in current directory
        outputFilePath: str = f"{targetClass.name}.h" if not fileName else fileName
    
    outputFilePath: str = os.path.join(Config.HEADER_OUTPUT_PATH, outputFilePath)

    try:
        with open(outputFilePath, 'w') as headerFile:
            headerFile.write(headerCode)
        print(f"Header file '{outputFilePath}' created successfully.")
        
        return True
    except Exception as e:
        print(f"Error writing header file '{outputFilePath}': {e}")
        return False

def ExportClassHeader(targetClass: ClassName):
    """
    Generate and save a C++ header file for the target class.
    For namespaced classes, creates appropriate folder structure.
    """
    allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]] = GetAllParsedClassVarsAndFuncs(targetClass)

    headerCode: str = GenerateHeaderCode(targetClass, allParsedClassVarsAndFuncs)
    if not headerCode:
        print(f"No functions were found for class {targetClass.fullName}, therefore will not generate.")
        return
    WriteHeaderToFile(targetClass, headerCode)

    nonCleanedHeaderCode: str = GenerateHeaderCode(targetClass, allParsedClassVarsAndFuncs, False)
    WriteHeaderToFile(targetClass, nonCleanedHeaderCode, f"{targetClass.name}-unclean.h")

def Main():
    """Main entry point for the script."""
    # Ask user for target class
    #targetClass = ida_kernwin.ask_str("IModelObject", 0, "Enter target class name (supports namespaces and templates):")
    targetClassName: str = "CLevel"
    if not targetClassName:
        print("No target class specified. Aborting.")
        return
    targetClass: ClassName = ClassName(targetClassName)

    breakpoint()
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
        print(f"Initializing \"{self.wanted_name}\" Plugin")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        print(f"Running \"{self.wanted_name}\" Plugin")
        Main()

    def term(self):
        print(f"Terminating \"{self.wanted_name}\" Plugin")
        pass

def PLUGIN_ENTRY():
    idaapi.msg(f"Creating \"Export Class to C++ Header\" Plugin entry")
    return ExportClassToCPPH()

if __name__ == "__main__":
    Main()