import os

from ExportClassH import JSONGen, Utils, Config
from ExportClassH.ClassDefs import ParsedClass, ParsedFunction, ParsedClassVar

currentAccess: str = "public"
def GenerateClassVarCode(classVar: ParsedClassVar, indentLevel: str) -> list[str]:
    """Generate code for a single class variable."""
    global currentAccess

    access: str = f"{classVar.access}:" if classVar.access else ""
    if currentAccess == classVar.access:
        access = ""
    else:
        currentAccess = classVar.access

    if classVar.varTypes:
        varTypesList: list[str] = [varType.name for varType in classVar.varTypes if varType]
        varTypes: str = Utils.ReplaceIDATypes(" ".join(varTypesList))
        varTypes = Utils.CleanType(varTypes)
        if varTypes:
            varTypes += " "
        varTypes = "GAME_IMPORT " + varTypes
    else:
        varTypes: str = ""
    
    classVarSig: str = f"{varTypes}{classVar.varName}"
    
    classVarLines: list[str] = []
    if access:
        classVarLines.append(f"{indentLevel}{access}")
    if classVarSig:
        classVarLines.append(f"{indentLevel}\t{classVarSig};")
    return classVarLines

def GenerateClassFuncCode(func: ParsedFunction, indentLevel: str, vtFuncIndex: int = 0) -> list[str]:
    """Generate code for a single class method."""
    global currentAccess

    access: str = f"{func.access}:" if func.access else ""
    if currentAccess == func.access:
        access = ""
    else:
        currentAccess = func.access

    const: str = " const" if func.const else ""
    strippedVirtual: str = " = 0" if func.type == "strippedVirtual" else ""

    if func.returnTypes:
        returnTypesList: list[str] = [returnType.name for returnType in func.returnTypes if returnType]
        returnType: str = Utils.ReplaceIDATypes(" ".join(returnTypesList))
        returnType = Utils.CleanType(returnType)
        if returnType:
            if func.type == "basicVirtual":
                returnType = returnType.removeprefix("virtual").strip()
            else:
                returnType += " "
    else:
        returnType: str = ""
    if func.type != "strippedVirtual" and func.type != "basicVirtual":
        returnType = "GAME_IMPORT " + returnType
    
    if func.params:
        paramsList: list[str] = [param.name for param in func.params if param]
        params: str = Utils.ReplaceIDATypes(", ".join(paramsList))
        params = Utils.CleanType(params)
        if params == "void":
            params = ""
    else:
        params: str = ""

    targetParams: str = ""
    if func.type == "basicVirtual":
        targetParams = Utils.ExtractParamNames(params)
        targetParams = ", " + targetParams if targetParams else ""

    funcSig: str = f"{returnType}{func.funcName}({params}){const}{strippedVirtual}" if func.type != "basicVirtual" else f"VIRTUAL_CALL({vtFuncIndex}, {returnType}, {func.funcName}, ({params}){targetParams})"

    classFuncLines: list[str] = []
    if access:
        classFuncLines.append(f"{indentLevel}{access}")
    if funcSig:
        classFuncLines.append(f"{indentLevel}\t{funcSig};")
    return classFuncLines

indentLevel = ""
def GenerateClassContent(targetClass: ParsedClass, indentLevel: str) -> list[str]:
    """
    Generate the content to be inserted into an existing header file.
    This is just the class members, not the full header with includes, etc.
    """
    firstVarOrFuncAccess = ""
    
    # Generate class content (just the members)
    contentLines = []
    
    # Add class variables
    for parsedClassVar in targetClass.classVars:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = parsedClassVar.access
        contentLines.extend(GenerateClassVarCode(parsedClassVar, indentLevel))
    
    # Add newline between sections if both exist
    if targetClass.classVars and (targetClass.virtualFunctions or targetClass.functions):
        contentLines.append("")
    
    # Add vtable functions
    for index, vTableFunc in enumerate(targetClass.virtualFunctions):
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = vTableFunc.access
        contentLines.extend(GenerateClassFuncCode(vTableFunc, indentLevel, index))
    
    # Add newline between sections if both exist
    if targetClass.virtualFunctions and targetClass.functions:
        contentLines.append("")
    
    # Add regular functions
    for func in targetClass.functions:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = func.access
        contentLines.extend(GenerateClassFuncCode(func, indentLevel))
    
    hasAnyContent = bool(contentLines)

    # Insert access specifier if needed
    if not firstVarOrFuncAccess and targetClass.type == "class":
        contentLines.insert(1, f"{indentLevel}public:")
    
    childClassDefinitions: list[str] = []
    for parsedClass in targetClass.childClasses.values():
        classDefinition = GenerateClassDefinition(parsedClass, indentLevel + "\t")
        classContent = GenerateClassContent(parsedClass, indentLevel + "\t")

        childClassDefinitions.extend(classDefinition[:len(classDefinition) - 1] + classContent + classDefinition[len(classDefinition) - 1:])
        childClassDefinitions.append("")
    
    if (hasAnyContent):
        contentLines.insert(0, "#pragma region GENERATED by ExportClassToCPPH.py")
        contentLines[1:1] = childClassDefinitions
        contentLines.append("#pragma endregion")
    else:
        contentLines[0:0] = childClassDefinitions
    return contentLines

def GenerateClassDefinition(targetClass: ParsedClass, indentLevel: str, forwardDeclare: bool = False) -> list[str]:
    """Generate a class definition from a list of methods."""
    if not forwardDeclare:
        forwardDeclare = (targetClass.type == "enum" or targetClass.type == "union") or not targetClass.classVars and not targetClass.virtualFunctions and not targetClass.functions and targetClass.type != "namespace"

    classDefLines: list[str] = [f"{indentLevel}{targetClass.type} {targetClass.name}{' {' if not forwardDeclare else ';'}"]
    if not forwardDeclare:
        if targetClass.type == "class":
            classDefLines.append(f"{indentLevel}public:")
        classDefLines.append(f"{indentLevel}}};")
    return classDefLines

def GenerateHeaderCode(targetClass: ParsedClass, parsedClassesDict: dict[str, ParsedClass]) -> list[str]:
    """Generate header code for a standard class (not nested)."""
    global indentLevel
    fullClassDefinition: list[str] = []

    for namespace in targetClass.parentNamespaces:
        fullClassDefinition.append(f"{indentLevel}namespace {namespace} {{")
        indentLevel += "\t"

    if targetClass.parentClasses:
        parsedParentClass = parsedClassesDict.get(targetClass.parentClasses[0])
        if parsedParentClass:
            parentClassDefinition = GenerateClassDefinition(parsedParentClass, indentLevel)
            parentClassContent = GenerateClassContent(parsedParentClass, indentLevel)

            fullClassDefinition.extend(parentClassDefinition[:len(parentClassDefinition) - 1] + parentClassContent + parentClassDefinition[len(parentClassDefinition) - 1:])
    
    classDefinition = GenerateClassDefinition(targetClass, indentLevel)
    classContent = GenerateClassContent(targetClass, indentLevel)
    fullClassDefinition.extend(classDefinition[:len(classDefinition) - 1] + classContent + classDefinition[len(classDefinition) - 1:])

    for namespace in reversed(targetClass.parentNamespaces):
        indentLevel = indentLevel[:-1]  # Remove one level of indentation
        fullClassDefinition.append(f"{indentLevel}}}")
    
    # Combine all parts of the header
    headerParts = ["#pragma once", r"#include <EGSDK\Imports.h>", ""]
    
    # Add the target class definition
    headerParts.extend(fullClassDefinition)
    return headerParts

def WriteHeaderToFile(targetClass: ParsedClass, headerCode: str, fileName: str) -> bool:
    """Write the generated header code to a file."""
    outputFolderPath: str = Config.HEADER_OUTPUT_PATH

    if targetClass.parentNamespaces:
        # Create folder structure for namespaces
        classFolderPath: str = os.path.join(*targetClass.parentNamespaces)
        outputFolderPath: str = os.path.join(outputFolderPath, classFolderPath)
        
        # Create directory if it doesn't exist
        os.makedirs(outputFolderPath, exist_ok=True)
        # Output file path is inside the namespace folder
        outputFilePath: str = os.path.join(classFolderPath, fileName)
    else:
        # Create directory if it doesn't exist
        os.makedirs(outputFolderPath, exist_ok=True)
        # No namespace, just save in current directory
        outputFilePath: str = fileName
    
    outputFilePath = os.path.join(Config.HEADER_OUTPUT_PATH, outputFilePath)

    try:
        with open(outputFilePath, 'w') as headerFile:
            headerFile.write(headerCode)
        print(f"Header file '{outputFilePath}' created successfully.")
        
        return True
    except Exception as e:
        print(f"Error writing header file '{outputFilePath}': {e}")
        return False
    
def ExportClassHeader(targetClass: str):
    """
    Generate and save a C++ header file for the target class.
    Handles multiple levels of nested classes and also generates dependencies.
    """
    parsedClassesDict = JSONGen.GetAllParsedClasses()

    parsedClass = parsedClassesDict.get(targetClass)
    if not parsedClass:
        print(f"There is no class {targetClass} available, therefore will not generate.")
        return

    JSONGen.MoveChildClasses(parsedClassesDict)
    headerCodeLines = GenerateHeaderCode(parsedClass, parsedClassesDict)
    headerCode: str = "\n".join(headerCodeLines)
    
    WriteHeaderToFile(parsedClass, headerCode, f"{parsedClass.name}.h")

def ExportClassHeaders():
    global indentLevel

    parsedClassesDict = JSONGen.GetAllParsedClasses()
    JSONGen.MoveChildClasses(parsedClassesDict)

    for (parsedClassName, parsedClass) in parsedClassesDict.items():
        headerCodeLines = GenerateHeaderCode(parsedClass, parsedClassesDict)
        headerCode: str = "\n".join(headerCodeLines)
        
        WriteHeaderToFile(parsedClass, headerCode, f"{parsedClass.name}.h" if parsedClass.type != "namespace" else "")
        # classDefinition = GenerateClassDefinition(parsedClass, indentLevel)
        # classContent = GenerateClassContent(parsedClass, indentLevel)

        # # Combine all parts of the header
        # headerParts = ["#pragma once", r"#include <EGSDK\Imports.h>", ""]
        
        # # Add the target class definition
        # headerParts.extend(classDefinition[:len(classDefinition) - 1] + classContent + classDefinition[len(classDefinition) - 1:])
        # headerCode: str = "\n".join(headerParts)
        
        # WriteHeaderToFile(parsedClass, headerCode, f"{parsedClass.name}.h" if parsedClass.type != "namespace" else "")