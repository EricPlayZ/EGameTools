import os
from typing import Optional

from ExportClassH import Utils, Config, ClassParser
from ExportClassH.ClassDefs import ClassName, ParsedFunction, ParsedClassVar

def IsClassGenerable(className: ClassName) -> bool:
    """
    Check if a class has any parsable elements (class vars, vtable functions, regular functions).
    Returns True if the class is generable, False if it should be treated as a namespace.
    """
    parsedVars = ClassParser.GetParsedClassVars(className)
    parsedVtFuncs = ClassParser.GetParsedVTableFuncs(className) if className else []
    parsedFuncs = ClassParser.GetParsedFuncs(className)
    
    return len(parsedVars) > 0 or len(parsedVtFuncs) > 0 or len(parsedFuncs) > 0

def IdentifyClassHierarchy(targetClass: ClassName) -> list[tuple[ClassName, bool]]:
    """
    Given a class name with namespace/nested parts, identify which parts are classes
    and which are namespaces.
    
    Returns a list of tuples (ClassName, isClass) for each part of the hierarchy.
    """
    if not targetClass.namespaces:
        return [(targetClass, IsClassGenerable(targetClass))]
    
    hierarchy = []
    
    # Check each part of the namespace to see if it's a class
    currentNamespace = []
    for part in targetClass.namespaces:
        currentNamespace.append(part)
        partClass = ClassName("::".join(currentNamespace))
        
        isClass = IsClassGenerable(partClass)
        hierarchy.append((partClass, isClass))
    
    # Add the target class at the end
    hierarchy.append((targetClass, IsClassGenerable(targetClass)))
    
    return hierarchy

currentAccess: str = "public"

def GenerateClassVarCode(classVar: ParsedClassVar, indent: str = "", cleanedTypes: bool = True) -> str:
    """Generate code for a single class variable."""
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

def GenerateClassFuncCode(func: ParsedFunction, indent: str = "", cleanedTypes: bool = True, vtFuncIndex: int = 0) -> str:
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
        targetParams = ClassParser.ExtractParamNames(params)
        targetParams = ", " + targetParams if targetParams else ""

    funcSig: str = f"{returnType}{func.funcName}({params}){const}{stripped_vfunc}" if func.type != "basic_vfunc" else f"VIRTUAL_CALL({vtFuncIndex}, {returnType}, {func.funcName}, ({params}){targetParams})"
    return f"{access}{funcSig};"

def GetClassTypeFromParsedSigs(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]]) -> str:
    """Determine the class type (class, struct, etc.) from parsed signatures."""
    if targetClass.type:
        return ""
    
    parsedVars, parsedVtFuncs, _ = allParsedElements
    
    # Check class vars first
    for parsedClassVar in parsedVars:
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
    for parsedFunc in ClassParser.allParsedFuncs:
        if (parsedFunc.returnType and 
            parsedFunc.returnType.namespacedName == targetClass.namespacedName and 
            parsedFunc.returnType.type):
            return parsedFunc.returnType.type
    
    return ""

def GenerateClassContent(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], indent: str = "", cleanedTypes: bool = True) -> str:
    """
    Generate the content to be inserted into an existing header file.
    This is just the class members, not the full header with includes, etc.
    """
    parsedVars, parsedVtFuncs, parsedFuncs = allParsedElements
    
    if not parsedVars and not parsedVtFuncs and not parsedFuncs:
        return ""
        
    global currentAccess
    currentAccess = ""

    # Get and set class type if available
    classType: str = GetClassTypeFromParsedSigs(targetClass, allParsedElements)
    if classType:
        object.__setattr__(targetClass, "type", classType)
    
    # Generate class content (just the members)
    contentLines = [f"#pragma region GENERATED by ExportClassToCPPH.py"]
    
    firstVarOrFuncAccess = ""
    
    # Add class variables
    for classVar in parsedVars:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = classVar.access
        contentLines.append(GenerateClassVarCode(classVar, indent, cleanedTypes))
    
    # Add newline between sections if both exist
    if parsedVars and (parsedVtFuncs or parsedFuncs):
        contentLines.append("")
    
    # Add vtable functions
    for index, vTableFunc in enumerate(parsedVtFuncs):
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = vTableFunc.access
        contentLines.append(GenerateClassFuncCode(vTableFunc, indent, cleanedTypes, index))
    
    # Add newline between sections if both exist
    if parsedVtFuncs and parsedFuncs:
        contentLines.append("")
    
    # Add regular functions
    for func in parsedFuncs:
        if not firstVarOrFuncAccess:
            firstVarOrFuncAccess = func.access
        contentLines.append(GenerateClassFuncCode(func, indent, cleanedTypes))
        
    contentLines.append("#pragma endregion")
    
    # Insert access specifier if needed
    if not firstVarOrFuncAccess:
        contentLines.insert(1, f"{indent}public:")
    
    return "\n".join(contentLines)

def GenerateClassDefinition(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], indent: str = "", cleanedTypes: bool = True) -> str:
    """Generate a class definition from a list of methods."""
    parsedVars, parsedVtFuncs, parsedFuncs = allParsedElements
    
    if not parsedVars and not parsedVtFuncs and not parsedFuncs:
        return ""
    
    classContent: str = GenerateClassContent(targetClass, allParsedElements, indent, cleanedTypes)
    
    classLines: list[str] = [f"{indent}{targetClass.type if targetClass.type else 'class'} {targetClass.name} {{"]
    if classContent:
        classLines.append(classContent)
    classLines.append(f"{indent}}};")

    return "\n".join(classLines)

def GenerateClassDefinitionRecursive(currentClass: ClassName, currentElements: tuple, nestedStructure: dict, indent: str, cleanedTypes: bool, placeholderClasses: list = []) -> str:
    """
    Recursively generate class definitions with nested classes and type dependencies.
    """
    parsedVars, parsedVtFuncs, parsedFuncs = currentElements
    
    # Check if we have any content (including placeholders)
    if not parsedVars and not parsedVtFuncs and not parsedFuncs and not nestedStructure and not placeholderClasses:
        return ""
    
    # Start the class definition
    classLines = [f"{indent}class {currentClass.name} {{"]
    
    # Add placeholder class forward declarations if any
    if placeholderClasses:
        classLines.append(f"{indent}public:")
        
        # Add explicitly requested placeholders
        if placeholderClasses:
            for placeholderClass in placeholderClasses:
                classLines.append(f"{indent}\tclass {placeholderClass};")
        
        classLines.append("")  # Add empty line after placeholders
    
    # Add nested classes with additional indentation
    if nestedStructure:
        # Only add public: if not already added for placeholders
        if not placeholderClasses:
            classLines.append(f"{indent}public:")
            
        for nestedClass, (nestedStructureLevel, nestedElements) in nestedStructure.items():
            nestedDefinition = GenerateClassDefinitionRecursive(
                nestedClass, 
                nestedElements, 
                nestedStructureLevel or {}, 
                indent + "\t",
                cleanedTypes
            )
            if nestedDefinition:
                classLines.append(nestedDefinition)
                classLines.append("")
    
    # Add the current class content
    classContent = GenerateClassContent(currentClass, currentElements, indent, cleanedTypes)
    if classContent:
        classLines.append(classContent)
    
    # Close the class
    classLines.append(f"{indent}}};")
    
    return "\n".join(classLines)

def BuildNestedStructure(hierarchy: list[tuple[ClassName, bool]], cleanedTypes: bool = True) -> tuple[Optional[ClassName], dict]:
    """
    Build a nested dictionary structure representing the class hierarchy.
    Returns (outermost_class, nested_structure_dict)
    """
    if not hierarchy:
        return None, {}
    
    outermostClass, _ = hierarchy[0]
    if len(hierarchy) == 1:
        return outermostClass, {}
    
    # Recursively build the nested structure
    nestedStructure = {}
    currentLevel = nestedStructure
    
    for i in range(1, len(hierarchy)):
        currentClass, isClass = hierarchy[i]
        if isClass:
            currentElements = ClassParser.GetAllParsedClassVarsAndFuncs(currentClass)
            
            # If this is not the last class, create a new level
            if i < len(hierarchy) - 1:
                currentLevel[currentClass] = ({}, currentElements)
                currentLevel = currentLevel[currentClass][0]
            else:
                # Last class in the hierarchy
                currentLevel[currentClass] = (None, currentElements)
    
    return outermostClass, nestedStructure

def GenerateHeaderCode(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool = True) -> str:
    """
    Generate a C++ header file for the target class.
    Supports handling nested classes with multiple levels of nesting.
    """
    # Identify the class hierarchy
    hierarchy = IdentifyClassHierarchy(targetClass)
    
    # If none of the namespace parts are classes (standard case), generate a normal class definition
    if not any(isClass for _, isClass in hierarchy[:-1]):
        return GenerateStandardHeaderCode(targetClass, allParsedElements, cleanedTypes, hierarchy)
    else:
        return GenerateNestedHeaderCode(targetClass, allParsedElements, cleanedTypes, hierarchy)

def GenerateStandardHeaderCode(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool, hierarchy: list[tuple[ClassName, bool]]) -> str:
    """Generate header code for a standard class (not nested)."""
    classDefinition = GenerateClassDefinition(targetClass, allParsedElements, "", cleanedTypes)
    if not classDefinition:
        return ""
    
    # Wrap in namespace blocks if needed
    if targetClass.namespaces:
        namespaceCode = []
        indentLevel = ""
        
        # Opening namespace blocks with increasing indentation
        for namespace in targetClass.namespaces:
            namespaceCode.append(f"{indentLevel}namespace {namespace} {{")
            indentLevel += "\t"

        classLines = classDefinition.split('\n')
        indentedClassLines = [f"{indentLevel}{line}" for line in classLines]
        indentedClassDefinition = "\n".join(indentedClassLines)
        
        namespaceCode.append(f"{indentedClassDefinition}")

        for namespace in reversed(targetClass.namespaces):
            indentLevel = indentLevel[:-1]  # Remove one level of indentation
            namespaceCode.append(f"{indentLevel}}}")
        
        classDefinition = "\n".join(namespaceCode)
    
    # Combine all parts of the header
    headerParts = ["#pragma once\n", r"#include <EGSDK\Imports.h>", "\n\n"]
    headerParts.append(classDefinition)
    
    return "".join(headerParts)

def GenerateNestedHeaderCode(targetClass: ClassName, allParsedElements: tuple[list[ParsedClassVar], list[ParsedFunction], list[ParsedFunction]], cleanedTypes: bool, hierarchy: list[tuple[ClassName, bool]], placeholderClasses: list = []) -> str:
    """Generate header code for nested classes."""
    # Handle nested classes
    outermostClassIndex = next((i for i, (_, isClass) in enumerate(hierarchy) if isClass), -1)
    outermostClass, _ = hierarchy[outermostClassIndex]
    outermostClassElements = ClassParser.GetAllParsedClassVarsAndFuncs(outermostClass)
    
    # Build the nested structure
    _, nestedStructure = BuildNestedStructure(hierarchy[outermostClassIndex:], cleanedTypes)
    
    # Generate the class definition recursively
    classDefinition = GenerateClassDefinitionRecursive(
        outermostClass, 
        outermostClassElements, 
        nestedStructure, 
        "", 
        cleanedTypes,
        placeholderClasses
    )
    
    if not classDefinition:
        return ""
    
    # Wrap in namespace blocks for any namespaces before the outermost class
    if outermostClassIndex > 0:
        namespaceCode = []
        indentLevel = ""
        
        # Opening namespace blocks with increasing indentation
        for namespace_part, _ in hierarchy[:outermostClassIndex]:
            namespaceCode.append(f"{indentLevel}namespace {namespace_part.name} {{")
            indentLevel += "\t"

        classLines = classDefinition.split('\n')
        indentedClassLines = [f"{indentLevel}{line}" for line in classLines]
        indentedClassDefinition = "\n".join(indentedClassLines)
        
        namespaceCode.append(f"{indentedClassDefinition}")

        for namespace_part, _ in reversed(hierarchy[:outermostClassIndex]):
            indentLevel = indentLevel[:-1]  # Remove one level of indentation
            namespaceCode.append(f"{indentLevel}}}")
        
        classDefinition = "\n".join(namespaceCode)
    
    # Combine all parts of the header
    headerParts = ["#pragma once\n", r"#include <EGSDK\Imports.h>", "\n\n"]
    headerParts.append(classDefinition)
    
    return "".join(headerParts)

def WriteHeaderToFile(targetClass: ClassName, headerCode: str, fileName: str = "") -> bool:
    """Write the generated header code to a file."""
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
    Handles multiple levels of nested classes and also generates dependencies.
    """
    # Identify the class hierarchy
    hierarchy = IdentifyClassHierarchy(targetClass)
    
    # Get the parsed elements for the target class
    allParsedElements = ClassParser.GetAllParsedClassVarsAndFuncs(targetClass)
    
    # If this is a nested class with no elements, add it as a placeholder
    if len(hierarchy) > 1 and not IsClassGenerable(targetClass):
        # Find the parent class (nearest generable class in hierarchy)
        parentIndex = -1
        for i in range(len(hierarchy) - 2, -1, -1):
            if hierarchy[i][1]:  # If this part is a class
                parentIndex = i
                break
                
        if parentIndex >= 0:
            # Get the parent class and its elements
            parentClass, _ = hierarchy[parentIndex]
            parentElements = ClassParser.GetAllParsedClassVarsAndFuncs(parentClass)
            
            # Use the existing nested structure generation with an added placeholder
            _, nestedStructure = BuildNestedStructure(hierarchy[parentIndex:], True)
            
            # Add the placeholder
            placeholderClasses = [targetClass.name]
            
            # Generate the header code with the placeholder
            headerCode = GenerateNestedHeaderCode(
                parentClass, 
                parentElements, 
                True, 
                hierarchy[:parentIndex+1],  # Only include up to parent
                placeholderClasses
            )
            
            if headerCode:
                WriteHeaderToFile(targetClass, headerCode)
                print(f"Generated header for class {parentClass.namespacedName} with placeholder for {targetClass.name}")
            return
    
    # Generate the header code
    headerCode = GenerateHeaderCode(targetClass, allParsedElements, True)
    if not headerCode:
        print(f"No functions were found for class {targetClass.namespacedName}, therefore will not generate.")
        return
    
    WriteHeaderToFile(targetClass, headerCode)

    headerCodeUnclean = GenerateHeaderCode(targetClass, allParsedElements, False)
    WriteHeaderToFile(targetClass, headerCodeUnclean, f"{targetClass.name}-unclean.h")