import os
import pickle
import idc
from typing import Optional

from ExportClassH import Utils, Config, RTTIAnalyzer
from ExportClassH.ClassDefs import ClassName, ParsedFunction, ParsedClassVar

# Global caches
parsedClassVarsByClass: dict[str, list[ParsedClassVar]] = {} # Cache of parsed class vars by class name
parsedVTableFuncsByClass: dict[str, list[ParsedFunction]] = {} # Cache of parsed functions by class name
parsedFuncsByClass: dict[str, list[ParsedFunction]] = {} # Cache of parsed functions by class name
allParsedFuncs: list[ParsedFunction] = []
unparsedExportedSigs: list[str] = []
allClassVarsAreParsed = False # Flag to indicate if all class vars have been parsed
allFuncsAreParsed = False # Flag to indicate if all functions have been parsed

def CreateParamNamesForVTFunc(parsedFunc: ParsedFunction, skipFirstParam: bool) -> str:
    paramsList: list[str] = [param.fullName for param in parsedFunc.params if param.fullName]
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
    paramsList: list[str] = [param.strip() for param in params.split(',') if param.strip()]
    if len(paramsList) == 1 and paramsList[0] == "void":
        return ""
    
    paramNames: list[str] = [param.split(" ")[-1].strip() for param in paramsList]
    newParams: str = ", ".join(paramNames)
    return newParams

def GetClassTypeFromParsedSigs(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]]) -> str:
    """Determine the class type (class, struct, etc.) from parsed signatures."""
    if targetClass.type:
        return ""
    parsedClassVars, parsedVtFuncs, _ = allParsedElements
    # Check class vars first
    for parsedClassVar in parsedClassVars:
        if (parsedClassVar.varType and 
            parsedClassVar.varType.namespacedName == targetClass.namespacedName and 
            parsedClassVar.varType.type):
            return parsedClassVar.varType.type
    # Check vtable functions next
    for parsedVTFunc in parsedVtFuncs:
        if (parsedVTFunc.returnType and 
            parsedVTFunc.returnType.namespacedName == targetClass.namespacedName and 
            parsedVTFunc.returnType.type):
            return parsedVTFunc.returnType.type
    # Check all parsed functions last
    for parsedFunc in allParsedFuncs:
        if (parsedFunc.returnType and 
            parsedFunc.returnType.namespacedName == targetClass.namespacedName and 
            parsedFunc.returnType.type):
            return parsedFunc.returnType.type
    
    return ""

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

                parsedClassVarsByClass.setdefault(parsedVar.className.namespacedName, []).append(parsedVar)

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
        return parsedClassVarsByClass.get(targetClass.namespacedName, [])

def GetParsedVTableFuncs(targetClass: ClassName) -> list[ParsedFunction]:
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedVTableFuncsByClass
    
    if targetClass not in parsedVTableFuncsByClass:
        parsedVTableFuncsByClass[targetClass.namespacedName] = []
        
        for (demangledFuncSig, rawType) in RTTIAnalyzer.GetDemangledVTableFuncSigs(targetClass):
            if rawType:
                parsedFunc: ParsedFunction = ParsedFunction(rawType, True)
                if parsedFunc.returnType:
                    newParamTypes: str = CreateParamNamesForVTFunc(parsedFunc, True) if parsedFunc.params else ""
                    demangledFuncSig = f"{'DUPLICATE_FUNC ' if demangledFuncSig.startswith('DUPLICATE_FUNC') else ''}IDA_GEN_PARSED virtual {parsedFunc.returnType.namespacedName} {demangledFuncSig.removeprefix('DUPLICATE_FUNC').strip()}({newParamTypes})"
            elif demangledFuncSig.startswith("DUPLICATE_FUNC"):
                parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig.removeprefix("DUPLICATE_FUNC").strip(), True)
                if parsedFunc.returnType:
                    newParamTypes: str = CreateParamNamesForVTFunc(parsedFunc, False) if parsedFunc.params else ""
                    demangledFuncSig = f"DUPLICATE_FUNC {parsedFunc.returnType.namespacedName} {parsedFunc.funcName}({newParamTypes})"

            parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig, True)
            if not parsedFunc.className:
                object.__setattr__(parsedFunc, "className", targetClass)
            
            parsedVTableFuncsByClass[targetClass.namespacedName].append(parsedFunc)
        
    return parsedVTableFuncsByClass.get(targetClass.namespacedName, [])

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
                parsedFuncsByClass.setdefault(parsedFunc.className.namespacedName, []).append(parsedFunc)
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
        return parsedFuncsByClass.get(targetClass.namespacedName, [])

def GetAllParsedClassVarsAndFuncs(targetClass: ClassName) -> tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]]:
    global allParsedFuncs

    parsedVTableClassFuncs: list[ParsedFunction] = GetParsedVTableFuncs(targetClass)
    if not parsedVTableClassFuncs:
        print(f"No matching VTable function signatures were found for {targetClass.namespacedName}.")

    parsedClassFuncs: list[ParsedFunction] = GetParsedFuncs(targetClass)
    if not parsedClassFuncs:
        print(f"No matching function signatures were found for {targetClass.namespacedName}.")
    allParsedFuncs = GetParsedFuncs()

    parsedClassVars: list[ParsedClassVar] = GetParsedClassVars(targetClass)
    if not parsedClassVars:
        print(f"No matching class var signatures were found for {targetClass.namespacedName}.")

    # Get non-vtable methods
    vTableFuncsSet: set[str] = {pf.fullFuncSig for pf in parsedVTableClassFuncs}
    finalParsedClassFuncs: list[ParsedFunction] = [
        parsedFunc for parsedFunc in parsedClassFuncs
        if parsedFunc.fullFuncSig not in vTableFuncsSet
    ]

    allParsedElements = (parsedClassVars, parsedVTableClassFuncs, finalParsedClassFuncs)

    # Get and set class type if available
    classType: str = GetClassTypeFromParsedSigs(targetClass, allParsedElements)
    if classType:
        object.__setattr__(targetClass, "type", classType)
    
    return allParsedElements