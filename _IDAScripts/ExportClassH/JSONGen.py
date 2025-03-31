import os
import json
from typing import Optional
from functools import cache

from ExportClassH import Utils, IDAUtils, RTTIAnalyzer, Config
from ExportClassH.ClassDefs import ParsedParam, ParsedClass, ParsedFunction, ParsedClassVar, DefaultPydanticSerializer

CLASS_TYPES = ["namespace", "class", "struct", "enum", "union"]
FUNC_TYPES = ["function", "strippedVirtual", "basicVirtual", "virtual"]
TYPES_OF_RETURN_TYPES = ["returnType", "classReturnType"]
STD_CLASSES = ["std", "rapidjson"]

parsedClassesDict: dict[str, ParsedClass] = {}
unparsedDemangledExportedSigs: list[str]= []

@cache
def GetTypeAndNameStr(fullName: str, returnFullName: bool = False) -> str:
    parts = Utils.ExtractTypeTokensFromString(fullName)
    if not parts:
        return ""
    if not len(parts) > 1:
        return ""
    
    for i in range(len(parts) - 1):
        if parts[i] in CLASS_TYPES:
            return f"{parts[i]} {parts[i + 1] if not returnFullName else ' '.join(parts[i + 1:])}"
    return ""

@cache
def SplitTypeFromName(fullName: str, returnFullName: bool = False) -> tuple[str, str]:
    typeAndNameStr = GetTypeAndNameStr(fullName, returnFullName)
    if not typeAndNameStr:
        return "", fullName
    
    typeAndName = typeAndNameStr.split(maxsplit=1)
    return typeAndName[0], typeAndName[1]

def GetParsedParamsFromList(paramsList: list[str], paramType: str) -> list[ParsedParam]:
    params: list[ParsedParam] = []
    for i in range(len(paramsList)):
        typeOfParam: str = paramType
        classType, className = SplitTypeFromName(paramsList[i], True)
        nameOfParam: str = className
        parsedClassOfParam: Optional[ParsedClass] = None
        
        if classType:
            typeOfParam = f"class{typeOfParam[0].upper()}{typeOfParam[1:]}"
            parsedClassOfParam = ParseClassStr(f"{classType} {className}")
            if parsedClassOfParam and parsedClassOfParam.parentNamespaces:
                parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(parsedClassOfParam.parentNamespaces)
                parsedClassOfParam.parentNamespaces = parentNamespaces
                parsedClassOfParam.parentClasses = parentClasses
        params.append(ParsedParam(type=typeOfParam, name=nameOfParam, parsedClassParam=parsedClassOfParam))
    return params

@cache
def ExtractClassNameAndTemplateParams(templatedClassName: str) -> tuple[str, list[ParsedParam]]:
    className = templatedClassName
    templateParams: list[ParsedParam] = []
    
    templateOpen = templatedClassName.find('<')
    templateClose = templatedClassName.rfind('>')
    if templateOpen != -1 and templateClose != -1:
        className = templatedClassName[:templateOpen].strip()
        paramsStr = templatedClassName[templateOpen + 1:templateClose].strip()
        paramsStr = Utils.ReplaceIDATypes(paramsStr)
        paramsStr = Utils.CleanType(paramsStr)
        
        # Split by commas, but only those outside of nested templates
        templateParams = GetParsedParamsFromList(Utils.SplitByCommaOutsideTemplates(paramsStr), "templateParam")
    
    return className, templateParams

def ExtractParentNamespacesAndClasses(namespacesAndClasses: list[str]) -> tuple[list[str], list[str]]:
    global parsedClassesDict

    parentNamespaces: list[str] = []
    parentClasses: list[str] = []

    continueOnlyWithClasses: bool = False
    for part in namespacesAndClasses:
        namespacesAndClass = "::".join(parentNamespaces + [part])
        if (namespacesAndClass not in parsedClassesDict or parsedClassesDict[namespacesAndClass].type == "namespace") and not continueOnlyWithClasses:
            parentNamespaces.append(part)
        else:
            if not continueOnlyWithClasses:
                continueOnlyWithClasses = True
            parentClasses.append(part)
    
    return parentNamespaces, parentClasses

def ParseClassStr(clsStr: str) -> Optional[ParsedClass]:
    clsStr = clsStr.strip()
    if not clsStr:
        return None
    clsStr = Utils.CleanEndOfClassStr(clsStr)
    
    parsedClass = ParsedClass()
    
    # Extract type (struct, class, etc.)
    typeAndName = SplitTypeFromName(clsStr)
    if not typeAndName[0]:
        return None
    
    parsedClass.type = typeAndName[0]
    templatedClassNameWithNS = typeAndName[1]
    
    # Split into namespaced parts and the final class name with templates
    lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(templatedClassNameWithNS)
    namespacesAndClasses = ""
    templatedClassName = ""
    
    if lastClassSeparatorIndex != -1:
        namespacesAndClasses = templatedClassNameWithNS[:lastClassSeparatorIndex].strip()
        templatedClassName = templatedClassNameWithNS[lastClassSeparatorIndex+2:].strip()
    else:
        templatedClassName = templatedClassNameWithNS
    
    # Extract template parameters
    className, templateParams = ExtractClassNameAndTemplateParams(templatedClassName)
    parsedClass.name = className
    parsedClass.templateParams = templateParams
    parentNamespaces = Utils.SplitByClassSeparatorOutsideTemplates(namespacesAndClasses)
    if any(STD_CLASS in parentNamespaces for STD_CLASS in STD_CLASSES) or any(STD_CLASS in className for STD_CLASS in STD_CLASSES):
        return None
    parsedClass.parentNamespaces.extend(parentNamespaces)

    parsedClass.fullClassName = f"{'::'.join(parsedClass.parentNamespaces + parsedClass.parentClasses + [parsedClass.name])}"
    return parsedClass

virtualFuncDuplicateCounter: dict[tuple[str, str], int] = {}
virtualFuncPlaceholderCounter: dict[tuple[str, str], int] = {}

def ParseFuncStr(funcStr: str, parsedClassFullName: str = "", onlyVirtualFuncs: bool = False) -> Optional[ParsedFunction]:
    global virtualFuncDuplicateCounter
    global virtualFuncPlaceholderCounter

    # Strip whitespace
    funcStr = funcStr.strip()
    if not funcStr:
        return None
    
    parsedFunc = ParsedFunction()
    parsedFunc.fullFuncSig = funcStr
    
    # Handle special cases
    isDuplicateFunc = False
    isIDAGeneratedType = False
    isIDAGeneratedTypeParsed = False
    
    if funcStr.startswith("DUPLICATE_FUNC"):
        isDuplicateFunc = True
        funcStr = funcStr.removeprefix("DUPLICATE_FUNC").strip()
    if funcStr.startswith("IDA_GEN_TYPE"):
        isIDAGeneratedType = True
        funcStr = funcStr.removeprefix("IDA_GEN_TYPE").strip()
    elif funcStr.startswith("IDA_GEN_PARSED"):
        isIDAGeneratedTypeParsed = True
        funcStr = funcStr.removeprefix("IDA_GEN_PARSED").strip()
    
    # Extract access modifier
    for keyword in ("public:", "protected:", "private:"):
        if funcStr.startswith(keyword):
            parsedFunc.access = keyword[:-1]  # remove the colon
            funcStr = funcStr[len(keyword):].strip()
            break
    
    # Find parameters and const qualifier
    paramsOpenParenIndex = funcStr.find('(')
    paramsCloseParenIndex = funcStr.rfind(')')
    
    if paramsOpenParenIndex != -1 and paramsCloseParenIndex != -1:
        # Extract parameters
        paramsStr = funcStr[paramsOpenParenIndex + 1:paramsCloseParenIndex]
        params = GetParsedParamsFromList(Utils.SplitByCommaOutsideTemplates(paramsStr), "param")
        if params and params[0] != "void":
            parsedFunc.params = params
        
        # Check for const qualifier
        remainingInputAfterParamsParen = funcStr[paramsCloseParenIndex + 1:].strip()
        parsedFunc.const = "const" in remainingInputAfterParamsParen
        
        # Process everything before parameters
        remainingInputBeforeParamsParen = funcStr[:paramsOpenParenIndex].strip()
        
        returnType = ""
        namespacesAndClasses = ""
        funcName = ""
        if not isIDAGeneratedType:
            # Find the last space outside of angle brackets
            lastSpaceIndex = Utils.FindLastSpaceOutsideTemplates(remainingInputBeforeParamsParen)
            
            if lastSpaceIndex != -1:
                # Split at the last space outside angle brackets
                returnType = remainingInputBeforeParamsParen[:lastSpaceIndex].strip()
                classAndFuncName = remainingInputBeforeParamsParen[lastSpaceIndex+1:].strip()
                
                # Find the last class separator outside of angle brackets
                lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(classAndFuncName)
                
                if lastClassSeparatorIndex != -1:
                    namespacesAndClasses = classAndFuncName[:lastClassSeparatorIndex]
                    funcName = classAndFuncName[lastClassSeparatorIndex+2:]
                else:
                    classParts = Utils.SplitByClassSeparatorOutsideTemplates(classAndFuncName)
                    namespacesAndClasses = "::".join(classParts[:-1]) if len(classParts) > 1 else ""
                    funcName = classParts[-1]
            else:
                # No space found, try to find class separator
                lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(remainingInputBeforeParamsParen)
                
                if lastClassSeparatorIndex != -1:
                    classAndFuncName = remainingInputBeforeParamsParen
                    namespacesAndClasses = classAndFuncName[:lastClassSeparatorIndex]
                    funcName = classAndFuncName[lastClassSeparatorIndex+2:]
                else:
                    funcName = remainingInputBeforeParamsParen
        else:
            returnType = remainingInputBeforeParamsParen

        parsedFunc.fullClassName = namespacesAndClasses
        parentNamespacesAndClasses = Utils.SplitByClassSeparatorOutsideTemplates(namespacesAndClasses)
        parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(parentNamespacesAndClasses)
        parsedFunc.parentNamespaces = parentNamespaces
        parsedFunc.parentClasses = parentClasses
        
        # Handle duplicate function naming
        if isDuplicateFunc:
            if not parsedClassFullName and not namespacesAndClasses:
                raise Exception("parsedClass variable not provided and namespacesAndClasses is empty for ParseFuncStr when func is duplicate")
            key = (parsedClassFullName if parsedClassFullName else namespacesAndClasses, funcStr)
            if key not in virtualFuncDuplicateCounter:
                    virtualFuncDuplicateCounter[key] = 0
            virtualFuncDuplicateCounter[key] += 1
            funcName = f"_{funcName}{virtualFuncDuplicateCounter[key]}"
        
        # Determine function type
        if onlyVirtualFuncs:
            returnType = returnType.replace("static", "").strip()
            if isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc or "virtual" not in returnType:
                parsedFunc.funcType = "basicVirtual"
            else:
                parsedFunc.funcType = "virtual"
        else:
            if "virtual" not in returnType:
                parsedFunc.funcType = "function"
            elif isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc:
                parsedFunc.funcType = "basicVirtual"
            else:
                parsedFunc.funcType = "virtual"
        
        returnType = Utils.ReplaceIDATypes(returnType)
        returnType = Utils.CleanType(returnType)
        returnTypes = GetParsedParamsFromList(Utils.ExtractTypeTokensFromString(returnType), "returnType")
        parsedFunc.returnTypes = returnTypes
        parsedFunc.funcName = funcName
    elif onlyVirtualFuncs and funcStr == "_purecall":
        if not parsedClassFullName:
            raise Exception("parsedClass variable not provided for ParseFuncStr when func is _purecall")
        
        key = (parsedClassFullName, funcStr)
        if key not in virtualFuncPlaceholderCounter:
            virtualFuncPlaceholderCounter[key] = 0
        virtualFuncPlaceholderCounter[key] += 1
        parsedFunc.funcType = "strippedVirtual"
        parsedFunc.returnTypes = [ParsedParam(type="returnType", name="virtual"), ParsedParam(type="returnType", name="void")]
        parsedFunc.funcName = f"_StrippedVFunc{virtualFuncPlaceholderCounter[key]}"
    
    return parsedFunc

def ParseClassVarStr(classVarStr: str) -> Optional[ParsedClassVar]:
    # Strip whitespace
    classVarStr = classVarStr.strip()
    if not classVarStr:
        return None
    
    parsedClassVar = ParsedClassVar()
    parsedClassVar.fullClassVarSig = classVarStr
    
    # Extract access modifier
    for keyword in ("public:", "protected:", "private:"):
        if classVarStr.startswith(keyword):
            parsedClassVar.access = keyword[:-1]  # remove the colon
            classVarStr = classVarStr[len(keyword):].strip()
            break
    # Find parameters and const qualifier
    paramsOpenParenIndex = classVarStr.find('(')
    paramsCloseParenIndex = classVarStr.rfind(')')
    
    # For class variables, we expect no parameters (i.e. no parentheses).
    if paramsOpenParenIndex == -1 and paramsCloseParenIndex == -1:
        varType = ""
        namespacesAndClasses = ""
        varName = ""

        # Find the last space outside of angle brackets
        lastSpaceIndex = Utils.FindLastSpaceOutsideTemplates(classVarStr)
        
        if lastSpaceIndex != -1:
            # Split at the last space outside angle brackets
            varType = classVarStr[:lastSpaceIndex].strip()
            classAndVarName = classVarStr[lastSpaceIndex+1:].strip()
            
            # Find the last class separator outside of angle brackets
            lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(classAndVarName)
            
            if lastClassSeparatorIndex != -1:
                namespacesAndClasses = classAndVarName[:lastClassSeparatorIndex]
                varName = classAndVarName[lastClassSeparatorIndex+2:]
            else:
                classParts = Utils.SplitByClassSeparatorOutsideTemplates(classAndVarName)
                namespacesAndClasses = "::".join(classParts[:-1]) if len(classParts) > 1 else ""
                varName = classParts[-1]
        else:
            return None

        parsedClassVar.fullClassName = namespacesAndClasses
        parentNamespacesAndClasses = Utils.SplitByClassSeparatorOutsideTemplates(namespacesAndClasses)
        parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(parentNamespacesAndClasses)
        parsedClassVar.parentNamespaces = parentNamespaces
        parsedClassVar.parentClasses = parentClasses
        
        varType = Utils.ReplaceIDATypes(varType)
        varType = Utils.CleanType(varType)
        varTypes = GetParsedParamsFromList(Utils.ExtractTypeTokensFromString(varType), "classVarType")
        parsedClassVar.varTypes = varTypes
        parsedClassVar.varName = varName
    else:
        return None
    
    return parsedClassVar

@cache
def ExtractAllClassSigsFromFuncSig(funcSig: str) -> list[str]:
    parts = Utils.ExtractTypeTokensFromString(funcSig)
    if not len(parts) > 1:
        return []
    
    listOfClassSigs: list[str] = []
    for i in range(len(parts) - 1):
        (classType, className) = (parts[i], parts[i + 1])
        if classType in CLASS_TYPES and className:
            listOfClassSigs.append(f"{classType} {className}")
    return listOfClassSigs

@cache
def ExtractMainClassSigFromFuncSig(funcSig: str) -> str:
    for keyword in ("public:", "protected:", "private:"):
        if funcSig.startswith(keyword):
            funcSig = funcSig[len(keyword):].strip()
            break

    paramsOpenParenIndex = funcSig.find('(')
    paramsCloseParenIndex = funcSig.rfind(')')
    if paramsOpenParenIndex == -1 or paramsCloseParenIndex == -1:
        return ""
    
    remainingInputBeforeParamsParen = funcSig[:paramsOpenParenIndex].strip()

    # Find the last space outside of angle brackets
    lastSpaceIndex = Utils.FindLastSpaceOutsideTemplates(remainingInputBeforeParamsParen)
    if lastSpaceIndex != -1:
        # Split at the last space outside angle brackets
        returnType = remainingInputBeforeParamsParen[:lastSpaceIndex].strip()
        parts = Utils.ExtractTypeTokensFromString(returnType)
        if not parts:
            return ""
        if len(parts) > 1:
            for i in range(len(parts)):
                classType = parts[i]
                if classType in CLASS_TYPES:
                    return ""
        
        classAndFuncName = remainingInputBeforeParamsParen[lastSpaceIndex + 1:].strip()
        
        # Find the last class separator outside of angle brackets
        lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(classAndFuncName)
        
        if lastClassSeparatorIndex != -1:
            namespacesAndClasses = classAndFuncName[:lastClassSeparatorIndex]
            funcName = classAndFuncName[lastClassSeparatorIndex+2:]
        else:
            classParts = Utils.SplitByClassSeparatorOutsideTemplates(classAndFuncName)
            namespacesAndClasses = "::".join(classParts[:-1]) if len(classParts) > 1 else ""
            funcName = classParts[-1]
    else:
        # No space found, try to find class separator
        lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(remainingInputBeforeParamsParen)
        
        if lastClassSeparatorIndex != -1:
            classAndFuncName = remainingInputBeforeParamsParen
            namespacesAndClasses = classAndFuncName[:lastClassSeparatorIndex]
            funcName = classAndFuncName[lastClassSeparatorIndex+2:]
        else:
            funcName = remainingInputBeforeParamsParen
            namespacesAndClasses = ""

    return f"{'class' if namespacesAndClasses.endswith(funcName) else 'namespace'} {namespacesAndClasses}" if namespacesAndClasses else ""

def FindParentClassRecursive(fullClassName: str, parsedClassesDict: dict[str, ParsedClass]) -> Optional[ParsedClass]:
    if fullClassName in parsedClassesDict:
        return parsedClassesDict[fullClassName]
    for parsedClass in parsedClassesDict.values():
        result = FindParentClassRecursive(fullClassName, parsedClass.childClasses)
        if result is not None:
            return result
    return None

def MoveChildClasses(parsedClassesDict: dict[str, ParsedClass]):
    # Find and move child classes to parent classes
    parsedClassesDictCopy = parsedClassesDict.copy()
    for childClass in parsedClassesDictCopy.values():
        if not childClass.parentNamespaces and not childClass.parentClasses:
            continue
        
        parentFullName = childClass.fullClassName
        lastClassSeparator = Utils.FindLastClassSeparatorOutsideTemplates(parentFullName)
        if lastClassSeparator:
            parentFullName = parentFullName[:lastClassSeparator]

        parentClass = FindParentClassRecursive(parentFullName, parsedClassesDict)
        if not parentClass:
            continue

        parentClass.childClasses[childClass.fullClassName] = childClass
        del parsedClassesDict[childClass.fullClassName]

def ParseAllClasses(parsedClassesDict: dict[str, ParsedClass]):
    # Get and parse all classes that are mentioned in a func sig, such as "class cbs::CPointer" in the params here: 'bool cbs::IsInDynamicRoot(class cbs::CPointer<class cbs::CEntity>, bool)'
    global unparsedDemangledExportedSigs
    unparsedDemangledExportedSigs = IDAUtils.GetDemangledExportedSigs(Config.INPUT_MD5)

    for demangledFuncSig in unparsedDemangledExportedSigs:
        listOfExtractedClassSigs = ExtractAllClassSigsFromFuncSig(demangledFuncSig)
        for clsSig in listOfExtractedClassSigs:
            parsedClass = ParseClassStr(clsSig)
            if not parsedClass:
                continue
            
            alreadyParsedClass = parsedClassesDict.get(parsedClass.fullClassName)
            if not alreadyParsedClass:
                parsedClassesDict[parsedClass.fullClassName] = parsedClass
            elif parsedClass.templateParams and parsedClass.templateParams[0] not in alreadyParsedClass.templateParams:
                alreadyParsedClass.templateParams.extend(parsedClass.templateParams)
    
    # Get and parse the main class that is mentioned in a func sig, such as "cbs" from "cbs::IsInDynamicRoot" in the name of the function here: 'bool cbs::IsInDynamicRoot(class cbs::CPointer<class cbs::CEntity>, bool)'
    for demangledFuncSig in unparsedDemangledExportedSigs:
        extractedMainClassSig = ExtractMainClassSigFromFuncSig(demangledFuncSig)
        parsedClass = ParseClassStr(extractedMainClassSig)
        if not parsedClass:
            continue

        if parsedClass.fullClassName not in parsedClassesDict:
            parsedClassesDict[parsedClass.fullClassName] = parsedClass
        elif parsedClass.type == "class" and parsedClassesDict[parsedClass.fullClassName].type == "namespace":
            parsedClassesDict[parsedClass.fullClassName].type = "class"
    
    # Generate missing namespaces or classes so we can properly move child classes into these generated parent classes later on
    parsedClassesCopy = list(parsedClassesDict.values())
    for parsedClass in parsedClassesCopy:
        parentNamespaces: list[str] = []
        parentClasses: list[str] = []

        for parentNamespace in parsedClass.parentNamespaces:
            namespaces = "::".join(parentNamespaces + [parentNamespace])
            parentNamespaces.append(parentNamespace)
            if namespaces in parsedClassesDict:
                continue
            
            parsedMissingClass = ParseClassStr(f"namespace {namespaces}")
            if parsedMissingClass:
                parsedClassesDict[parsedMissingClass.fullClassName] = parsedMissingClass
        
        for parentClass in parsedClass.parentClasses:
            classes = "::".join(parentNamespaces + parentClasses + [parentClass])
            parentClasses.append(parentClass)
            if classes in parsedClassesDict:
                continue

            parsedMissingClass = ParseClassStr(f"class {classes}")
            if parsedMissingClass:
                parsedClassesDict[parsedMissingClass.fullClassName] = parsedMissingClass
    
    # Fix parsed classes by setting the right parent namespaces and classes (because cbs might be a parent class and not a parent namespace, which will later change how the header generates for the class)
    for parsedClass in parsedClassesDict.values():
        parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(parsedClass.parentNamespaces)
        parsedClass.parentNamespaces = parentNamespaces
        parsedClass.parentClasses = parentClasses
        if (parentClasses or parsedClass.templateParams) and parsedClass.type == "namespace":
            parsedClass.type = "class"

def CreateParamNamesForVTFunc(parsedFunc: ParsedFunction, skipFirstParam: bool) -> str:
    paramsList: list[str] = [param.name for param in parsedFunc.params if param.name]
    if len(paramsList) == 1 and paramsList[0] == "void":
        return "void"
    # Skip the first parameter (typically the "this" pointer)
    if skipFirstParam:
        paramsList = paramsList[1:]
    paramsList = [Utils.FixTypeSpacing(param.strip()) for param in paramsList]
    
    paramNames: list[str] = [f"a{i+1}" for i in range(len(paramsList))]
    newParams: str = ", ".join(f"{paramType} {paramName}" for paramType, paramName in zip(paramsList, paramNames))
    return newParams

def ParseClassVTFuncs(parsedClass: ParsedClass):
    for (demangledFuncSig, rawType) in RTTIAnalyzer.GetDemangledVTableFuncSigs(Config.INPUT_MD5, tuple(parsedClass.parentNamespaces + parsedClass.parentClasses), parsedClass.name):
        if rawType:
            parsedFunc = ParseFuncStr(rawType, parsedClass.fullClassName, True)
            if not parsedFunc:
                continue

            if parsedFunc.returnTypes:
                newParamTypes = CreateParamNamesForVTFunc(parsedFunc, True) if parsedFunc.params else ""
                returnTypes = [returnType.name for returnType in parsedFunc.returnTypes if returnType.name]
                returnTypesStr = ' '.join(returnTypes)
                demangledFuncSig = f"{'DUPLICATE_FUNC ' if demangledFuncSig.startswith('DUPLICATE_FUNC') else ''}IDA_GEN_PARSED virtual {returnTypesStr}  {demangledFuncSig.removeprefix('DUPLICATE_FUNC').strip()}({newParamTypes})"
        elif demangledFuncSig.startswith("DUPLICATE_FUNC"):
            parsedFunc = ParseFuncStr(demangledFuncSig.removeprefix("DUPLICATE_FUNC").strip(), parsedClass.fullClassName, True)
            if not parsedFunc:
                continue

            if parsedFunc.returnTypes:
                newParamTypes: str = CreateParamNamesForVTFunc(parsedFunc, False) if parsedFunc.params else ""
                returnTypes = [returnType.name for returnType in parsedFunc.returnTypes if returnType.name]
                returnTypesStr = ' '.join(returnTypes)
                demangledFuncSig = f"DUPLICATE_FUNC {returnTypesStr} {parsedFunc.funcName}({newParamTypes})"

        parsedFunc = ParseFuncStr(demangledFuncSig, parsedClass.fullClassName, True)
        if not parsedFunc:
            continue

        if parsedClass.type == "namespace":
            parsedClass.type = "class"

        # Add dependency classes by going through the func class name, return types and params
        if parsedFunc.fullClassName and parsedFunc.fullClassName not in parsedClass.classDependencies:
            parsedClass.classDependencies.append(parsedFunc.fullClassName)
        for param in parsedFunc.params:
            if param.parsedClassParam and param.parsedClassParam.fullClassName and param.parsedClassParam.fullClassName not in parsedClass.classDependencies:
                parsedClass.classDependencies.append(param.parsedClassParam.fullClassName)
        for returnType in parsedFunc.returnTypes:
            if returnType.parsedClassParam and returnType.parsedClassParam.fullClassName and returnType.parsedClassParam.fullClassName not in parsedClass.classDependencies:
                parsedClass.classDependencies.append(returnType.parsedClassParam.fullClassName)

        # Append parsed func to class
        parsedClass.virtualFunctions.append(parsedFunc)

def ParseAllClassVTFuncs(parsedClassesDict: dict[str, ParsedClass]):
    for parsedClass in parsedClassesDict.values():
        ParseClassVTFuncs(parsedClass)

def ParseAllClassFuncs(parsedClassesDict: dict[str, ParsedClass]):
    global unparsedDemangledExportedSigs

    for demangledExportedSig in unparsedDemangledExportedSigs.copy():
        parsedFunc = ParseFuncStr(demangledExportedSig, "", False)
        if not parsedFunc or not parsedFunc.fullClassName:
            continue

        parsedClass = parsedClassesDict.get(parsedFunc.fullClassName)
        if not parsedClass:
            continue
        
        # Add dependency classes by going through the return types and params
        for param in parsedFunc.params:
            if param.parsedClassParam and param.parsedClassParam.fullClassName and param.parsedClassParam.fullClassName not in parsedClass.classDependencies:
                parsedClass.classDependencies.append(param.parsedClassParam.fullClassName)
        for returnType in parsedFunc.returnTypes:
            if returnType.parsedClassParam and returnType.parsedClassParam.fullClassName and returnType.parsedClassParam.fullClassName not in parsedClass.classDependencies:
                parsedClass.classDependencies.append(returnType.parsedClassParam.fullClassName)

        # Append parsed func to class
        parsedClass.functions.append(parsedFunc)
        unparsedDemangledExportedSigs.remove(demangledExportedSig)

def ParseAllClassVars(parsedClassesDict: dict[str, ParsedClass]):
    global unparsedDemangledExportedSigs

    for demangledExportedSig in unparsedDemangledExportedSigs.copy():
        parsedClassVar = ParseClassVarStr(demangledExportedSig)
        if not parsedClassVar or not parsedClassVar.fullClassName:
            continue

        parsedClass = parsedClassesDict.get(parsedClassVar.fullClassName)
        if not parsedClass:
            continue

        # Add dependency classes by going through the var types
        for varType in parsedClassVar.varTypes:
            if varType.parsedClassParam and varType.parsedClassParam.fullClassName and varType.parsedClassParam.fullClassName not in parsedClass.classDependencies:
                parsedClass.classDependencies.append(varType.parsedClassParam.fullClassName)

        # Append parsed func to class
        parsedClass.classVars.append(parsedClassVar)
        unparsedDemangledExportedSigs.remove(demangledExportedSig)

def GetAllParsedClasses() -> dict[str, ParsedClass]:
    global parsedClassesDict

    if os.path.exists(Config.PARSED_CLASSES_OUTPUT_FILE) and not parsedClassesDict:
        with open(Config.PARSED_CLASSES_OUTPUT_FILE, 'r') as fileStream:
            data = json.load(fileStream)
        parsedClassesDict = { key: ParsedClass.model_validate(value) for key, value in data.items() }
    elif not parsedClassesDict:
        parsedClassesDict.clear()
        ParseAllClasses(parsedClassesDict)
        ParseAllClassVTFuncs(parsedClassesDict)
        ParseAllClassFuncs(parsedClassesDict)
        ParseAllClassVars(parsedClassesDict)
        #MoveChildClasses(parsedClassesDict)
        with open(Config.PARSED_CLASSES_OUTPUT_FILE, 'w') as fileStream:
            fileStream.write(json.dumps(parsedClassesDict, indent=4, default=DefaultPydanticSerializer))
    
    return parsedClassesDict