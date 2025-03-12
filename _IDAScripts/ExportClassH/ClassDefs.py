from dataclasses import dataclass, field
from typing import Optional

from ExportClassH import Utils, HeaderGen

virtualFuncPlaceholderCounter: int = 0 # Counter for placeholder virtual functions
virtualFuncDuplicateCounter: dict[str, int] = {} # Counter for duplicate virtual functions

@dataclass(frozen=True)
class ParsedFunction:
    """Parse a demangled function signature and return an instance."""
    fullFuncSig: str = ""
    type: str = ""
    access: str = ""
    returnType: Optional[ClassName] = None
    className: Optional[ClassName] = None
    funcName: str = ""
    params: list[ClassName] = field(default_factory=list[ClassName])
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
        object.__setattr__(self, "params", [])
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
        for keyword in ("public:", "protected:", "private:"):
            if signature.startswith(keyword):
                access = keyword[:-1]  # remove the colon
                signature = signature[len(keyword):].strip()
                break

        # Find parameters and const qualifier
        paramsOpenParenIndex: int = signature.find('(')
        paramsCloseParenIndex: int = signature.rfind(')')
        
        if paramsOpenParenIndex != -1 and paramsCloseParenIndex != -1:
            params: str = signature[paramsOpenParenIndex + 1:paramsCloseParenIndex]
            paramsStrList: list[str] = Utils.SplitByCommaOutsideTemplates(params)
            paramsList: list[ClassName] = [ClassName(paramStr) for paramStr in paramsStrList if paramStr]

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
                returnType = returnType.replace("static", "").strip()
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
            object.__setattr__(self, "type", type)
            object.__setattr__(self, "access", access if access else "public")
            object.__setattr__(self, "returnType", ClassName(returnType) if returnType else None)
            object.__setattr__(self, "className", ClassName(className) if className else None)
            object.__setattr__(self, "funcName", funcName)
            object.__setattr__(self, "params", paramsList)
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
        # Initialize defaults.
        object.__setattr__(self, "access", "")
        object.__setattr__(self, "varType", None)
        object.__setattr__(self, "className", None)
        object.__setattr__(self, "varName", "")
        
        # Extract access specifier.
        access = ""
        for keyword in ("public:", "protected:", "private:"):
            if signature.startswith(keyword):
                access = keyword[:-1]  # remove the colon
                signature = signature[len(keyword):].strip()
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