import re
from collections import defaultdict

def parse_function_signature(signature):
    # Match class methods (including templates)
    match_function = re.match(
        r"(public:|protected:|private:)?\s*(.*?)\s+(?:__cdecl\s+)?([a-zA-Z_][\w:~<>]*)::([\w~<>]+)\((.*?)\)\s*(const)?", signature
    )
    
    # Match global variables (including pointer types)
    match_global_var = re.match(
        r"(.*?)\s+(__cdecl\s+)?([\w:~<>]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(.*)$", signature
    )
    
    # Handle class methods
    if match_function:
        access, return_type, class_name, function_name, parameters, const = match_function.groups()
        return {
            "type": "method",
            "access": access.strip() if access else "public",
            "return_type": return_type.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
            "class_name": class_name.strip(),
            "function_name": function_name.strip(),
            "parameters": parameters.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
            "const": "const" if const else "",  # Capture const if it exists
        }
    
    # Handle global variables
    elif match_global_var:
        return_type, _, _, var_name, modifiers = match_global_var.groups()
        return {
            "type": "global_var",
            "return_type": return_type.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
            "var_name": var_name.strip(),
            "modifiers": modifiers.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
        }
    
    # Handle free functions (not in a class)
    match_free_func = re.match(
        r"(.*?)\s+(__cdecl\s+)?([\w:~<>]+(?:<[^>]*>)?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\((.*?)\)\s*(const)?", signature
    )
    if match_free_func:
        return_type, _, _, function_name, parameters, const = match_free_func.groups()
        return {
            "type": "free_function",
            "return_type": return_type.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
            "function_name": function_name.strip(),
            "parameters": parameters.strip().replace("__cdecl ", "").replace(" __cdecl", "").replace("__cdecl", "").replace(" __ptr64", "").replace("__ptr64 ", "").replace("__ptr64", "").replace(" class", "").replace("class ", "").replace("class", "").replace(" struct", "").replace("struct ", "").replace("struct", "").replace(" enum", "").replace("enum ", "").replace("enum", ""),
            "const": "const" if const else "",  # Capture const if it exists
        }
    
    return None

def generate_header(parsed_functions):
    header_code = "#pragma once\n\n"
    class_definitions = defaultdict(list)
    global_variables = []
    free_functions = []
    
    for func in parsed_functions:
        if func["type"] == "method":
            class_definitions[func["class_name"]].append(func)
        elif func["type"] == "global_var":
            global_variables.append(func)
        elif func["type"] == "free_function":
            free_functions.append(func)
    
    # Generate classes
    for class_name, functions in class_definitions.items():
        header_code += f"class __declspec(dllimport) {class_name} {{\n"
        header_code += "public:\n"
        for func in functions:
            const = " const" if func["const"] else ""  # Add const if present
            rt = f"{func['return_type']} " if func["return_type"] else ""
            parameters = func['parameters']
            if parameters.strip() == "void":
                parameters = ""
            header_code += f"    {rt}{func['function_name']}({parameters}){const};\n"
        header_code += "};\n\n"
    
    # Generate global variables
    for var in global_variables:
        header_code += f"{var['return_type']} {var['var_name']}{' ' + var['modifiers'] if var['modifiers'] else ''};\n"
    
    # Generate free functions
    for func in free_functions:
        const = " const" if func["const"] else ""  # Add const if present
        header_code += f"{func['return_type']} {func['function_name']}({func['parameters']}){const};\n"
    
    return header_code

def read_function_signatures_from_file(file_path):
    """Reads the function signatures from a file and returns them as a list."""
    try:
        with open(file_path, 'r') as file:
            return file.readlines()
    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
        return []
    except IOError:
        print(f"Error: Unable to read the file '{file_path}'.")
        return []

def main():
    # Read function signatures from a file
    file_path = "demangled.txt"  # Replace with your file path
    function_signatures = read_function_signatures_from_file(file_path)
    
    if not function_signatures:
        return  # If the file couldn't be read or is empty, exit

    parsed_functions = []
    
    for signature in function_signatures:
        signature = signature.strip()  # Remove leading/trailing whitespaces
        if signature:  # Ignore empty lines
            parsed_func = parse_function_signature(signature)
            if parsed_func:
                parsed_functions.append(parsed_func)
    
    header_content = generate_header(parsed_functions)
    print(header_content)
    
if __name__ == "__main__":
    main()
