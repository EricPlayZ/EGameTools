import os
import re

from ExportClassH import ClassGen, Config, HeaderGen
from ExportClassH.ClassDefs import ClassName

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
        allParsedClassVarsAndFuncs = ClassGen.GetAllParsedClassVarsAndFuncs(targetClass)
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
                
                # Generate and insert the code
                generatedContent = f"{HeaderGen.GenerateClassContent(targetClass, allParsedClassVarsAndFuncs, indent)}"
                if generatedContent:
                    success = UpdateExistingHeaderFile(targetClass, filePath, generatedContent)
                    if success:
                        processedCount += 1
    
    print(f"Successfully processed {processedCount} header files.")