import json
from typing import Optional

from ExportClassH import Utils, IDAUtils
from ExportClassH.ClassDefs import ParsedClass, ParsedFunction, ParsedParam

CLASS_TYPES = ["namespace", "class", "struct", "enum", "union"]
FUNC_TYPES = ["function", "strippedVirtual", "basicVirtual", "virtual"]
TYPES_OF_RETURN_TYPES = ["returnType", "classReturnType"]
STD_CLASSES = ["std", "rapidjson"]

parsedClassesDict: dict[str, ParsedClass] = {}

def GetTypeAndNameStr(fullName: str) -> str:
    parts = Utils.ExtractTypeTokensFromString(fullName)
    if not parts:
        return ""
    if not len(parts) > 1:
        return ""
    
    for i in range(len(parts) - 1):
        if parts[i] in CLASS_TYPES:
            return f"{parts[i]} {parts[i + 1]}"
    return ""

def SplitTypeFromName(fullName: str) -> tuple[str, str]:
    typeAndNameStr = GetTypeAndNameStr(fullName)
    if not typeAndNameStr:
        return "", fullName
    
    typeAndName = typeAndNameStr.split(maxsplit=1)
    return typeAndName[0], typeAndName[1]

def GetParsedParamsFromList(paramsList: list[str], type: str) -> list[ParsedParam]:
    params: list[ParsedParam] = []
    for i in range(len(paramsList)):
        typeOfParam: str = type
        classType, className = SplitTypeFromName(paramsList[i])
        nameOfParam: str = className
        parsedClassOfParam: Optional[ParsedClass] = None
        
        if classType:
            typeOfParam = f"class{typeOfParam[0].upper()}{typeOfParam[1:]}"
            parsedClassOfParam = ParseClassStr(f"{classType} {className}")
        params.append(ParsedParam(type=typeOfParam, name=nameOfParam, parsedClassParam=parsedClassOfParam))
    return params

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

def ParseClassStr(clsStr: str) -> Optional[ParsedClass]:
    clsStr = clsStr.strip()
    if not clsStr:
        return None
    
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
    if any(STD_CLASS in parentNamespaces for STD_CLASS in STD_CLASSES):
        return None
    parsedClass.parentNamespaces.extend(parentNamespaces)

    parsedClass.fullClassName = f"{'::'.join(parsedClass.parentNamespaces + parsedClass.parentClasses + [parsedClass.name])}"
    return parsedClass

virtualFuncDuplicateCounter: dict[str, int] = {}
virtualFuncPlaceholderCounter: dict[str, int] = {}
def ParseFuncStr(funcStr: str, onlyVirtualFuncs: bool = False) -> Optional[ParsedFunction]:
    global virtualFuncDuplicateCounter
    global virtualFuncPlaceholderCounter

    # Strip whitespace
    funcStr = funcStr.strip()
    if not funcStr:
        return None
    
    parsedFunc = ParsedFunction(fullFuncSig=funcStr.strip())
    
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
        paramsStrList = Utils.SplitByCommaOutsideTemplates(paramsStr)
        finalParamsStrList = []
        ######################
        ################### TO SEE HOW TO PROPERLY IMPLEMENT
        ######################
        for paramStr in paramsStrList:
            parsedParamAsClass = ParseClassStr(paramStr)
            if parsedParamAsClass:
                finalParamsStrList.append(parsedParamAsClass)
            else:
                finalParamsStrList.append(paramStr)
        parsedFunc.params = finalParamsStrList
        
        # Check for const qualifier
        remainingInputAfterParamsParen = funcStr[paramsCloseParenIndex + 1:].strip()
        parsedFunc.const = "const" in remainingInputAfterParamsParen
        
        # Process everything before parameters
        remainingInputBeforeParamsParen = funcStr[:paramsOpenParenIndex].strip()
        
        returnType = ""
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
                    classParts = classAndFuncName.split("::")
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
                    returnType = ""
                    funcName = remainingInputBeforeParamsParen
                    namespacesAndClasses = ""
        else:
            returnType = remainingInputBeforeParamsParen
            namespacesAndClasses = ""
            funcName = ""

        # parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(namespacesAndClasses)
        # funcInfo.parentNamespaces.extend(parentNamespaces)
        # funcInfo.parentNamespaces.extend(parentClasses)
        
        # Handle duplicate function naming
        if isDuplicateFunc:
            if funcStr not in virtualFuncDuplicateCounter:
                    virtualFuncDuplicateCounter[funcStr] = 0
            virtualFuncDuplicateCounter[funcStr] += 1
            funcName = f"_{funcName}{virtualFuncDuplicateCounter[funcStr]}"
        
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
        
        ######################
        ################### TO SEE HOW TO PROPERLY IMPLEMENT
        ######################
        returnType = Utils.ReplaceIDATypes(returnType)
        returnType = Utils.CleanType(returnType)

        returnTypeTokens = Utils.ExtractTypeTokensFromString(returnType)
        for i in range(len(returnTypeTokens)):
            typeOfReturnType: str = "returnType"
            nameOfReturnType: str = returnTypeTokens[i]
            parsedClassOfReturnType: Optional[ParsedClass] = None

            if len(returnTypeTokens) > 1:
                if returnTypeTokens[i] in CLASS_TYPES:
                    typeOfReturnType = "classReturnType"
                    parsedClassOfReturnType = ParseClassStr(f"{returnTypeTokens[i]} {returnTypeTokens[i + 1]}")
            parsedFunc.returnTypes.append(ParsedParam(type=typeOfReturnType, name=nameOfReturnType, parsedClassParam=parsedClassOfReturnType))

        #funcInfo.class = ParseClassNameString(className) if className else {}
        parsedFunc.funcName = funcName
    
    # Handle special case for _purecall
    elif onlyVirtualFuncs and funcStr == "_purecall":
        virtualFuncPlaceholderCounter[funcStr] += 1
        parsedFunc.funcType = "strippedVirtual"
        parsedFunc.returnTypes = [ParsedParam(type="returnType", name="virtual"), ParsedParam(type="returnType", name="void")]
        parsedFunc.funcName = f"_StrippedVFunc{virtualFuncPlaceholderCounter[funcStr]}"
    
    return parsedFunc

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

def ExtractAllClassSigsFromFuncSig(funcSig: str) -> list[str]:
    parts = Utils.ExtractTypeTokensFromString(funcSig)
    if not len(parts) > 1:
        return []
    
    listOfClassSigs: list[str] = []
    for i in range(len(parts) - 1):
        (classType, className) = (parts[i], parts[i + 1])
        if classType in CLASS_TYPES and className:
            className = Utils.CleanEndOfClassStr(className)
            listOfClassSigs.append(f"{classType} {className}")
    return listOfClassSigs

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

def ParseAllClasses():
    global parsedClassesDict
    parsedClassesDict = {}

    # Get and parse all classes that are mentioned in a func sig, such as "class cbs::CPointer" in the params here: 'bool cbs::IsInDynamicRoot(class cbs::CPointer<class cbs::CEntity>, bool)'
    demangledExportedSigs = IDAUtils.GetDemangledExportedSigs()
    for demangledFuncSig in demangledExportedSigs:
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
    for demangledFuncSig in demangledExportedSigs:
        extractedMainClassSig = ExtractMainClassSigFromFuncSig(demangledFuncSig)
        parsedClass = ParseClassStr(extractedMainClassSig)
        if not parsedClass:
            continue

        if parsedClass.fullClassName not in parsedClassesDict:
            parsedClassesDict[parsedClass.fullClassName] = parsedClass
        elif parsedClass.type == "class" and parsedClassesDict[parsedClass.fullClassName].type == "namespace":
            parsedClassesDict[parsedClass.fullClassName].type = "class"
    
    # Fix parsed classes by setting the right parent namespaces and classes (because cbs might be a parent class and not a parent namespace, which will later change how the header generates for the class)
    for parsedClass in parsedClassesDict.values():
        parentNamespaces, parentClasses = ExtractParentNamespacesAndClasses(parsedClass.parentNamespaces)
        parsedClass.parentNamespaces = parentNamespaces
        parsedClass.parentClasses = parentClasses
        if (parentClasses or parsedClass.templateParams) and parsedClass.type == "namespace":
            parsedClass.type = "class"

    # Find and move child classes to parent classes
    for parsedClass in list(parsedClassesDict.values()):
        if not parsedClass.parentNamespaces and not parsedClass.parentClasses:
            continue
        
        parentName = ""
        if parsedClass.parentNamespaces:
            parentName = parsedClass.parentNamespaces[-1]
        elif parsedClass.parentClasses:
            parentName = parsedClass.parentClasses[-1]
        if not parentName:
            continue

        parentClass = None
        if parsedClass.parentNamespaces:
            parentClass = next((parentClass for parentClass in parsedClassesDict.values() if parentClass.name == parentName and parentClass.parentNamespaces == parsedClass.parentNamespaces[:-1]), None)
        elif parsedClass.parentClasses:
            parentClass = next((parentClass for parentClass in parsedClassesDict.values() if parentClass.name == parentName and parentClass.parentClasses == parsedClass.parentClasses[:-1]), None)
        if not parentClass:
            continue

        parentClass.childClasses.append(parsedClass)
        del parsedClassesDict[parsedClass.fullClassName]

def GetAllParsedClasses():
    ParseAllClasses()
    print(json.dumps(parsedClassesDict, indent=4))