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

def ParseClassStr(fullName: str):
    classInfo = {
        "type": "",
        "name": "",
        "templateParams": [],
        "parentNamespace": [],
        "parentClass": []
    }
    
    # Strip whitespace
    fullName = fullName.strip()
    if not fullName:
        return classInfo
    
    # Extract type (struct, class, etc.)
    typeAndName = SplitTypeFromName(fullName)
    if not typeAndName[0]:
        return classInfo
    
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