from ExportClassH import Utils, ClassParser

def SplitTypeFromName(fullName: str) -> tuple[str, str]:
    CLASS_TYPES = ["class", "struct", "enum", "union"]
    FUNC_QUALIFIERS = ["virtual", "static", "inline", "explicit", "friend"]
    
    parts = Utils.ExtractTypeTokensFromString(fullName)
    if not parts:
        return "", ""
    
    if len(parts) > 1:
        if parts[0] in CLASS_TYPES:
            return parts[0], parts[1]
        elif len(parts) > 2 and parts[0] in FUNC_QUALIFIERS and parts[1] in CLASS_TYPES:
            return parts[1], parts[2]
    
    return "", fullName

def ExtractClassNameAndTemplateParams(templatedClassName: str) -> tuple[str, list[str]]:
    templateParams = []
    className = templatedClassName
    
    templateOpen = templatedClassName.find('<')
    templateClose = templatedClassName.rfind('>')
    
    if templateOpen != -1 and templateClose != -1:
        className = templatedClassName[:templateOpen].strip()
        paramsStr = templatedClassName[templateOpen + 1:templateClose].strip()
        
        # Split by commas, but only those outside of nested templates
        templateParams = Utils.SplitByCommaOutsideTemplates(paramsStr)
    
    return className, templateParams

def ParseClassStr(clsStr: str) -> dict:
    classInfo = {
        "type": "",
        "name": "",
        "templateParams": [],
        "parentNamespace": [],
        "parentClass": []
    }
    
    # Strip whitespace
    clsStr = clsStr.strip()
    if not clsStr:
        return {}
    
    # Extract type (struct, class, etc.)
    typeAndName = SplitTypeFromName(clsStr)
    if not typeAndName[0]:
        return {}
    
    classInfo["type"] = typeAndName[0]
    templatedClassNameWithNS = typeAndName[1]
    
    # Split into namespaced parts and the final class name with templates
    lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(templatedClassNameWithNS)
    namespacesAndClasses = ""
    templatedClassName = ""
    
    if lastClassSeparatorIndex != -1:
        namespacesAndClasses = templatedClassNameWithNS[:lastClassSeparatorIndex].strip()
        templatedClassName = templatedClassNameWithNS[lastClassSeparatorIndex+2:].strip()
    
    # Extract template parameters
    className, templateParams = ExtractClassNameAndTemplateParams(templatedClassName)
    classInfo["name"] = className
    classInfo["templateParams"] = templateParams
    
    # Split namespaces and classes - for this example we'll use a simple approach
    # where we assume the first part(s) are namespaces and later parts are parent classes
    if namespacesAndClasses:
        allParts = namespacesAndClasses.split("::")
        continueOnlyWithClasses: bool = False
        for part in allParts:
            if not ClassParser.IsClassGenerable(classInfo) and not continueOnlyWithClasses:
                classInfo["parentNamespace"].append(part)
            else:
                if not continueOnlyWithClasses:
                    continueOnlyWithClasses = True
                classInfo["parentClass"].append(part)
    
    return classInfo

def ParseFuncStr(funcStr: str, onlyVirtualFuncs: bool = False) -> dict:
    funcInfo = {
        "type": "function",
        "access": "public",
        "funcType": "",
        "returnType": {},
        "funcName": "",
        "params": [],
        "const": False,
        "parentNamespace": [],
        "parentClass": [],
        "fullFuncSig": funcStr.strip()
    }
    
    # Strip whitespace
    funcStr = funcStr.strip()
    if not funcStr:
        return {}
    
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
            funcInfo["access"] = keyword[:-1]  # remove the colon
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
        funcInfo["params"] = finalParamsStrList
        
        # Check for const qualifier
        remainingInputAfterParamsParen = funcStr[paramsCloseParenIndex + 1:].strip()
        funcInfo["const"] = "const" in remainingInputAfterParamsParen
        
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
                    className = classAndFuncName[:lastClassSeparatorIndex]
                    funcName = classAndFuncName[lastClassSeparatorIndex+2:]
                else:
                    classParts = classAndFuncName.split("::")
                    className = "::".join(classParts[:-1]) if len(classParts) > 1 else ""
                    funcName = classParts[-1]
            else:
                # No space found, try to find class separator
                lastClassSeparatorIndex = Utils.FindLastClassSeparatorOutsideTemplates(remainingInputBeforeParamsParen)
                
                if lastClassSeparatorIndex != -1:
                    classAndFuncName = remainingInputBeforeParamsParen
                    className = classAndFuncName[:lastClassSeparatorIndex]
                    funcName = classAndFuncName[lastClassSeparatorIndex+2:]
                else:
                    returnType = ""
                    funcName = remainingInputBeforeParamsParen
                    className = ""
        else:
            returnType = remainingInputBeforeParamsParen
            className = ""
            funcName = ""
        
        # Handle duplicate function naming
        if isDuplicateFunc:
            funcName = f"_{funcName}1"  # Simplified counter handling
        
        # Determine function type
        if onlyVirtualFuncs:
            returnType = returnType.replace("static", "").strip()
            if isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc or "virtual" not in returnType:
                funcInfo["funcType"] = "basicVirtual"
            else:
                funcInfo["funcType"] = "virtual"
        else:
            if "virtual" not in returnType:
                funcInfo["funcType"] = "func"
            elif isIDAGeneratedType or isIDAGeneratedTypeParsed or isDuplicateFunc:
                funcInfo["funcType"] = "basic_vfunc"
            else:
                funcInfo["funcType"] = "vfunc"
        
        ######################
        ################### TO SEE HOW TO PROPERLY IMPLEMENT
        ######################
        funcInfo["returnType"] = ParseClassNameString(returnType) if returnType else {}
        funcInfo["className"] = ParseClassNameString(className) if className else {}
        funcInfo["funcName"] = funcName
    
    # Handle special case for _purecall
    elif onlyVirtualFuncs and funcStr == "_purecall":
        funcInfo["type"] = "stripped_vfunc"
        funcInfo["returnType"] = ParseClassNameString("virtual void")
        funcInfo["funcName"] = "_StrippedVFunc1"  # Simplified counter
    
    return funcInfo