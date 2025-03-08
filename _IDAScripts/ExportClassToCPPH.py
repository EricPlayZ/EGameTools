import os
import re
import struct
import idc
import idaapi
import ida_kernwin
import ida_hexrays
import ida_bytes
import ida_ida
import pickle
import json
from typing import Optional

idaapi.require("ExportClassToCPPH")
from ExportClassToCPPH import Utils, Config
from ExportClassToCPPH.ClassDefs import ClassName, ParsedFunction, ParsedClassVar

CONFIG_FILE = os.path.join(os.path.dirname(__file__), "ExportClassToCPPH.json")

# Global caches
parsedClassVarsByClass: dict[str, list[ParsedClassVar]] = {} # Cache of parsed class vars by class name
parsedVTableFuncsByClass: dict[str, list[ParsedFunction]] = {} # Cache of parsed functions by class name
parsedFuncsByClass: dict[str, list[ParsedFunction]] = {} # Cache of parsed functions by class name
allParsedFuncs: list[ParsedFunction] = []
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
                    parsedClassVarsByClass = pickle.load(cache_file)
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

                parsedClassVarsByClass.setdefault(parsedVar.className.fullName, []).append(parsedVar)

            allClassVarsAreParsed = True

            # Cache the parsed class variables
            try:
                # Create directory if it doesn't exist
                os.makedirs(Config.CACHE_OUTPUT_PATH, exist_ok=True)
                with open(Config.PARSED_VARS_CACHE_FILENAME, "wb") as cache_file:
                    pickle.dump(parsedClassVarsByClass, cache_file)
                print(f"Cached class variables to \"{Config.PARSED_VARS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to write cache to \"{Config.PARSED_VARS_CACHE_FILENAME}\": {e}")
                if os.path.exists(Config.PARSED_VARS_CACHE_FILENAME):
                    os.remove(Config.PARSED_VARS_CACHE_FILENAME)

    # Return all class variables or only those for the target class
    if targetClass is None:
        return [var for vars_list in parsedClassVarsByClass.values() for var in vars_list]
    else:
        return parsedClassVarsByClass.get(targetClass.fullName, [])

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
        parsedVTableFuncsByClass[targetClass.fullName] = []
        
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
            
            parsedVTableFuncsByClass[targetClass.fullName].append(parsedFunc)
        
    return parsedVTableFuncsByClass.get(targetClass.fullName, [])

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
                    parsedFuncsByClass = pickle.load(cache_file)
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
                parsedFuncsByClass.setdefault(parsedFunc.className.fullName, []).append(parsedFunc)
            allFuncsAreParsed = True
            try:
                os.makedirs(Config.CACHE_OUTPUT_PATH, exist_ok=True)
                with open(Config.PARSED_FUNCS_CACHE_FILENAME, "wb") as cache_file:
                    pickle.dump(parsedFuncsByClass, cache_file)
                print(f"Cached parsed functions to \"{Config.PARSED_FUNCS_CACHE_FILENAME}\"")
            except Exception as e:
                print(f"Failed to write cache to \"{Config.PARSED_FUNCS_CACHE_FILENAME}\": {e}")
                if os.path.exists(Config.PARSED_FUNCS_CACHE_FILENAME):
                    os.remove(Config.PARSED_FUNCS_CACHE_FILENAME)

    # Return functions based on targetClass if specified
    if targetClass is None:
        return [pf for funcList in parsedFuncsByClass.values() for pf in funcList]
    else:
        return parsedFuncsByClass.get(targetClass.fullName, [])

# -----------------------------------------------------------------------------
# Header generation
# -----------------------------------------------------------------------------

currentAccess: str = "public"
def GenerateClassVarCode(classVar: ParsedClassVar, indent: str = "\t", cleanedTypes: bool = True) -> str:
    """Generate code for a single class method."""
    global currentAccess

    access: str = f"{indent}{classVar.access}:\n{indent}\t" if classVar.access else f"{indent}\t"
    if currentAccess == classVar.access:
        access = f"{indent}\t"
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

def GenerateClassFuncCode(func: ParsedFunction, indent: str = "\t", cleanedTypes: bool = True, vtFuncIndex: int = 0) -> str:
    """Generate code for a single class method."""
    global currentAccess

    access: str = f"{indent}{func.access}:\n{indent}\t" if func.access else f"{indent}\t"
    if currentAccess == func.access:
        access = f"{indent}\t"
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

def GetClassTypeFromParsedSigs(targetClass: ClassName, allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]]) -> str:
    global allParsedFuncs

    if targetClass.type:
        return ""
    
    for parsedClassVar in allParsedClassVarsAndFuncs[0]:
        if parsedClassVar.varType and parsedClassVar.varType.namespacedName == targetClass.namespacedName and parsedClassVar.varType.type:
            return parsedClassVar.varType.type
    for parsedVTFunc in allParsedClassVarsAndFuncs[1]:
        if parsedVTFunc.returnType and parsedVTFunc.returnType.namespacedName == targetClass.namespacedName and parsedVTFunc.returnType.type:
            return parsedVTFunc.returnType.type
    for parsedFunc in allParsedFuncs:
        if parsedFunc.returnType and parsedFunc.returnType.namespacedName == targetClass.namespacedName and parsedFunc.returnType.type:
            return parsedFunc.returnType.type
    
    return ""

def GenerateClassContent(targetClass: ClassName, allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], indent: str = "\t", cleanedTypes: bool = True) -> str:
    """
    Generate the content to be inserted into an existing header file.
    This is just the class members, not the full header with includes, etc.
    """
    if not allParsedClassVarsAndFuncs[0] and not allParsedClassVarsAndFuncs[1] and not allParsedClassVarsAndFuncs[2]:
        return ""
    global currentAccess
    currentAccess = ""

    classType: str = GetClassTypeFromParsedSigs(targetClass, allParsedClassVarsAndFuncs)
    if classType:
        object.__setattr__(targetClass, "type", classType)
    
    # Generate class content (just the members, not the full class definition)
    contentLines = [f"#pragma region GENERATED by ExportClassToCPPH.py"]
    
    firstVarOrFuncAccess = ""
    for classVar in allParsedClassVarsAndFuncs[0]:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = classVar.access
        contentLines.append(GenerateClassVarCode(classVar, indent, cleanedTypes))
    if allParsedClassVarsAndFuncs[0] and (allParsedClassVarsAndFuncs[1] or allParsedClassVarsAndFuncs[2]):
        contentLines.append("")
    for index, vTableFunc in enumerate(allParsedClassVarsAndFuncs[1]):
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = vTableFunc.access
        contentLines.append(GenerateClassFuncCode(vTableFunc, indent, cleanedTypes, index))
    if (allParsedClassVarsAndFuncs[0] or allParsedClassVarsAndFuncs[1]) and allParsedClassVarsAndFuncs[2]:
        contentLines.append("")
    for func in allParsedClassVarsAndFuncs[2]:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = func.access
        contentLines.append(GenerateClassFuncCode(func, indent, cleanedTypes))
        
    contentLines.append("#pragma endregion")
    
    # Insert access specifier if needed
    if not firstVarOrFuncAccess:
        contentLines.insert(1, f"{indent}public:")
    
    return "\n".join(contentLines)

def GenerateClassDefinition(targetClass: ClassName, allParsedClassVarsAndFuncs: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool = True) -> str:
    """Generate a class definition from a list of methods."""
    # Build the class definition
    if not allParsedClassVarsAndFuncs[0] and not allParsedClassVarsAndFuncs[1] and not allParsedClassVarsAndFuncs[2]:
        return ""
    
    classContent: str = GenerateClassContent(targetClass, allParsedClassVarsAndFuncs, "\t", cleanedTypes)
    
    classLines: list[str] = [f"{targetClass.type if targetClass.type else 'class'} {targetClass.name} {{"]
    if classContent:
        classLines.append(classContent)
    classLines.append("};")

    return "\n".join(classLines)

def FindClassDefInFile(className: str, filePath: str) -> tuple[bool, str, int, int, str]:
    """
    Search for a class definition in a header file.
    Returns:
        - bool: Whether the class was found
        - str: The class type (class, struct, union, etc.)
        - int: Start line of the class definition
        - int: End line of the class definition (or -1 if not found)
    """
    try:
        with open(filePath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        # Regular expression to match class definitions with potential attributes like EGameSDK_API
        classPattern = re.compile(r'^\s*(class|struct|union|enum)\s+(?:[A-Za-z0-9_]+\s+)*' + re.escape(className) + r'\s*(?::|{)')
        
        for i, line in enumerate(lines):
            match = classPattern.search(line)
            if match:
                # Found the class definition
                classType = match.group(1)
                
                # Find the end of the class definition (closing brace)
                braceCount = 0
                foundOpenBrace = False
                indent = ""
                
                for j in range(i, len(lines)):
                    if '{' in lines[j]:
                        foundOpenBrace = True
                        braceCount += lines[j].count('{')
                        
                        # If this is the first open brace, determine the indentation for the class content
                        if braceCount == 1:
                            # Look at the next non-empty line to determine indentation
                            if lines[j].strip():
                                # Extract indentation
                                indentMatch = re.match(r'^(\s+)', lines[j])
                                if indentMatch:
                                    indent = indentMatch.group(1)
                                break
                    
                    if '}' in lines[j]:
                        braceCount -= lines[j].count('}')
                    
                    if foundOpenBrace and braceCount == 0:
                        return True, classType, i, j, indent
                
                # If we couldn't find the end, just return the start
                return True, classType, i, -1, indent
        
        return False, "", -1, -1, ""
    
    except Exception as e:
        print(f"Error reading file '{filePath}': {e}")
        return False, "", -1, -1, ""

def FindGeneratedRegionInFile(filePath: str) -> tuple[int, int, str]:
    """
    Search for an existing generated region in a header file.
    Returns:
        - int: Start line of the generated region
        - int: End line of the generated region
    """
    try:
        with open(filePath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        startPattern = r'^\s*#pragma\s+region\s+GENERATED\s+by\s+ExportClassToCPPH\.py'
        endPattern = r'^\s*#pragma\s+endregion'
        
        startLine = -1
        indent = ""
        
        for i, line in enumerate(lines):
            if re.search(startPattern, line):
                startLine = i
                # Extract indentation
                indentMatch = re.match(r'^(\s+)', line)
                if indentMatch:
                    indent = indentMatch.group(1)
                break
        
        if startLine != -1:
            for i in range(startLine + 1, len(lines)):
                if re.search(endPattern, lines[i]):
                    return startLine, i, indent
        
        return -1, -1, ""
    
    except Exception as e:
        print(f"Error reading file '{filePath}': {e}")
        return -1, -1, ""

def FindExistingHeaderFiles(basePath: str) -> dict[str, str]:
    """
    Find all header files in the project directory.
    Returns a dictionary mapping class names to file paths.
    """
    classToFile = {}
    
    for root, _, files in os.walk(basePath):
        for file in files:
            if file.endswith(".h") or file.endswith(".hpp"):
                filePath = os.path.join(root, file)
                
                # Extract the base name without extension
                className = os.path.splitext(file)[0]
                
                # If the file name matches a potential class name, add it to our dictionary
                classToFile[className] = filePath
    
    return classToFile

def UpdateExistingHeaderFile(targetClass: ClassName, filePath: str, generatedCode: str) -> bool:
    """
    Update an existing header file with the generated code.
    If a generated region already exists, replace it.
    Otherwise, insert the generated code at the start of the class definition.
    """
    try:
        with open(filePath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        # First, check if there's an existing generated region
        startRegion, endRegion, indent = FindGeneratedRegionInFile(filePath)
        
        if startRegion != -1 and endRegion != -1:
            # Replace existing region
            updatedLines = lines[:startRegion] + [generatedCode + ('\n' if endRegion + 1 < len(lines) and lines[endRegion + 1].strip() == "};" else '\n\n')] + lines[endRegion+1:]
            
            with open(filePath, 'w', encoding='utf-8') as f:
                f.writelines(updatedLines)
            
            print(f"Updated existing generated region in '{filePath}'")
            return True
        
        # If no existing region, find the class definition
        found, classType, classStart, _, indent = FindClassDefInFile(targetClass.name, filePath)
        
        if found:
            # Find the line after the opening brace
            braceLine = -1
            for i in range(classStart, len(lines)):
                if '{' in lines[i]:
                    braceLine = i
                    break
            
            if braceLine != -1:
                # Insert generated code after the opening brace
                insertPos = braceLine + 1
                
                updatedLines = lines[:insertPos] + [generatedCode + '\n'] + lines[insertPos:]
                
                with open(filePath, 'w', encoding='utf-8') as f:
                    f.writelines(updatedLines)
                
                print(f"Inserted generated code in '{filePath}'")
                return True
        
        print(f"Could not find a suitable position to insert code in '{filePath}'")
        return False
    
    except Exception as e:
        print(f"Error updating file '{filePath}': {e}")
        return False

def ProcessExistingHeaders():
    """
    Scan PROJECT_PATH for header files, find matching class definitions,
    and update them with generated code.
    """
    print(f"Scanning {Config.PROJECT_INCLUDES_PATH} for header files...")
    classFiles = FindExistingHeaderFiles(Config.PROJECT_INCLUDES_PATH)
    print(f"Found {len(classFiles)} header files.")
    
    processedCount = 0
    for className, filePath in classFiles.items():
        # Create a ClassName object
        targetClass = ClassName(className)
        
        # Check if this class has any functions or variables to export
        allParsedClassVarsAndFuncs = GetAllParsedClassVarsAndFuncs(targetClass)
        hasContent = (
            len(allParsedClassVarsAndFuncs[0]) > 0 or 
            len(allParsedClassVarsAndFuncs[1]) > 0 or 
            len(allParsedClassVarsAndFuncs[2]) > 0
        )
        
        if hasContent:
            # Verify the class exists in the file
            found, class_type, _, _, indent = FindClassDefInFile(className, filePath)
            
            if found:
                print(f"Found {class_type} {className} in {filePath}")
                
                # Set the class type
                #object.__setattr__(targetClass, "type", class_type)
                
                # Generate and insert the code
                generatedContent = f"{GenerateClassContent(targetClass, allParsedClassVarsAndFuncs, indent)}"
                if generatedContent:
                    success = UpdateExistingHeaderFile(targetClass, filePath, generatedContent)
                    if success:
                        processedCount += 1
    
    print(f"Successfully processed {processedCount} header files.")

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
    global allParsedFuncs

    parsedVTableClassFuncs: list[ParsedFunction] = GetParsedVTableFuncs(targetClass)
    if not parsedVTableClassFuncs:
        print(f"No matching VTable function signatures were found for {targetClass.fullName}.")

    parsedClassFuncs: list[ParsedFunction] = GetParsedFuncs(targetClass)
    if not parsedClassFuncs:
        print(f"No matching function signatures were found for {targetClass.fullName}.")
    allParsedFuncs = GetParsedFuncs()

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

# -----------------------------------------------------------------------------
# UI Integration
# -----------------------------------------------------------------------------

def SetConfigVars(settings):
    Config.PROJECT_INCLUDES_PATH = settings["PROJECT_INCLUDES_PATH"]
    Config.OUTPUT_PATH = settings["OUTPUT_PATH"]
    Config.LAST_CLICKED_RADIO = settings["LAST_CLICKED_RADIO"]

    Config.HEADER_OUTPUT_PATH = os.path.join(Config.OUTPUT_PATH, "generated")
    Config.CACHE_OUTPUT_PATH = os.path.join(Config.OUTPUT_PATH, "cache")
    Config.PARSED_VARS_CACHE_FILENAME = os.path.join(Config.CACHE_OUTPUT_PATH, "parsedClassVarsByClass.cache")
    Config.PARSED_FUNCS_CACHE_FILENAME = os.path.join(Config.CACHE_OUTPUT_PATH, "parsedFuncsByClass.cache")

# Load settings from file
def LoadConfig():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            loadedJson = json.load(f)
            SetConfigVars(loadedJson)
            return loadedJson
    return Config.DEFAULT_CONFIG

# Save settings to file
def SaveConfig(settings = {}):
    if not settings:
        settings = {
            "PROJECT_INCLUDES_PATH": Config.PROJECT_INCLUDES_PATH,
            "OUTPUT_PATH": Config.OUTPUT_PATH,
            "LAST_CLICKED_RADIO": Config.LAST_CLICKED_RADIO
        }

    with open(CONFIG_FILE, "w") as f:
        json.dump(settings, f, indent=4)
    
    SetConfigVars(settings)

class SettingsDialog(ida_kernwin.Form):
    def __init__(self, current_config):
        self.config = current_config
        ida_kernwin.Form.__init__(self, r"""STARTITEM 0
BUTTON YES* Save
BUTTON CANCEL Cancel
Settings

Modify the paths below:

<##Project Includes Path:{i_project_path}>
<##Output Path:{i_output_path}>
""", {
            'i_project_path': ida_kernwin.Form.StringInput(swidth=50, value=self.config["PROJECT_INCLUDES_PATH"]),
            'i_output_path': ida_kernwin.Form.StringInput(swidth=50, value=self.config["OUTPUT_PATH"]),
        })

    def GetValues(self):
        return {
            "PROJECT_INCLUDES_PATH": self.i_project_path.value,
            "OUTPUT_PATH": self.i_output_path.value,
            "LAST_CLICKED_RADIO": Config.LAST_CLICKED_RADIO
        }
    
    def OnFormChange(self, fid):
        return 1  # Required for form functionality

class MainDialog(ida_kernwin.Form):
    def __init__(self):
        ida_kernwin.Form.__init__(self, r"""STARTITEM 0
BUTTON YES* OK
BUTTON CANCEL Cancel
Export Class to C++ Header

{FormChangeCb}

<##Update Project Code:{r_update}>
<##Generate Class Code:{r_generate}>
<##Settings:{r_settings}>{radioGroup}>
""", {
            'radioGroup': ida_kernwin.Form.RadGroupControl(("r_update", "r_generate", "r_settings")),
            'FormChangeCb': ida_kernwin.Form.FormChangeCb(self.OnFormChange),
        })

    def OnFormChange(self, fid):
        return 1  # Required for form functionality

def OpenSettingsDlg():
    """Opens the Settings Dialog and returns to Main Dialog on Cancel"""
    settings = LoadConfig()
    settingsDlg = SettingsDialog(settings)
    settingsDlg.Compile()
    result = settingsDlg.Execute()

    if result == 1:  # Save clicked
        newSettings = settingsDlg.GetValues()
        SaveConfig(newSettings)  # Save new settings to file
        print("[INFO] Settings saved successfully!")

    settingsDlg.Free()
    
    # Always return to the main dialog after closing settings
    OpenMainDlg()

def OpenMainDlg():
    """Reopens the Main Dialog"""
    LoadConfig()

    mainDlg = MainDialog()
    mainDlg.Compile()
    mainDlg.radioGroup.value = Config.LAST_CLICKED_RADIO
    result = mainDlg.Execute()

    if result == 1:  # OK clicked
        selectedOption = mainDlg.radioGroup.value
        Config.LAST_CLICKED_RADIO = selectedOption
        SaveConfig()

        if selectedOption == 0:
            print("[INFO] Update Project Code selected!")
            ProcessExistingHeaders()
        elif selectedOption == 1:
            print("[INFO] Generate Class Code selected!")
            targetClassName = ida_kernwin.ask_str("", 0, "Enter target class name:")
            if not targetClassName:
                print("No target class specified. Aborting.")
                return
            ExportClassHeader(ClassName(targetClassName))
        elif selectedOption == 2:
            print("[INFO] Settings selected!")
            OpenSettingsDlg()  # Open settings when selected
    else:
        print("[INFO] User clicked Cancel. No action taken.")

    mainDlg.Free()

def Main():
    """Main entry point for the script."""
    OpenMainDlg()
    
    # Reload modules to apply any changes
    import importlib
    import ExportClassToCPPH.Utils
    import ExportClassToCPPH.Config
    import ExportClassToCPPH.ClassDefs
    importlib.reload(ExportClassToCPPH.Config)
    importlib.reload(ExportClassToCPPH.Utils)
    importlib.reload(ExportClassToCPPH.ClassDefs)

# -----------------------------------------------------------------------------
# IDA plugin integration
# -----------------------------------------------------------------------------

class ExportClassToCPPHClass(idaapi.plugin_t):
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
    return ExportClassToCPPHClass()

if __name__ == "__main__":
    Main()