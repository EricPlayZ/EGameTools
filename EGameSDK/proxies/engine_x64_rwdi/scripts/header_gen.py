import re
import os
import sys
from collections import defaultdict

# Global variable for the project folder—set this to your project's path as needed.
PROJECT_FOLDER = "D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"

def fix_spacing(s):
    """Fix spacing for pointers/references, commas, and angle brackets."""
    # Remove extra space before '*' or '&'
    s = re.sub(r'\s+([*&])', r'\1', s)
    # Ensure each comma is followed by exactly one space.
    s = re.sub(r'\s*,\s*', ', ', s)
    # Remove extra space after '<' and before '>'
    s = re.sub(r'<\s+', '<', s)
    s = re.sub(r'\s+>', '>', s)
    s = s.strip()
    # Collapse multiple spaces into a single space
    s = re.sub(r'\s+', ' ', s)
    return s

def clean_type(s):
    """Removes unwanted tokens (__cdecl, __ptr64, class, struct, enum) from a type string,
    then fixes spacing."""
    # Remove the unwanted tokens as whole words.
    s = re.sub(r'\b(__cdecl|__ptr64|class|struct|enum|union)\b', '', s)
    return fix_spacing(s)

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
            "return_type": return_type.strip(),
            "class_name": class_name.strip(),
            "function_name": function_name.strip(),
            "parameters": parameters.strip(),
            "const": "const" if const else "",  # Capture const if it exists
        }
    
    # Handle global variables
    elif match_global_var:
        return_type, _, _, var_name, modifiers = match_global_var.groups()
        return {
            "type": "global_var",
            "return_type": return_type.strip(),
            "var_name": var_name.strip(),
            "modifiers": modifiers.strip(),
        }
    
    # Handle free functions (not in a class)
    match_free_func = re.match(
        r"(.*?)\s+(__cdecl\s+)?([\w:~<>]+(?:<[^>]*>)?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\((.*?)\)\s*(const)?", signature
    )
    if match_free_func:
        return_type, _, function_name, parameters, const = match_free_func.groups()
        return {
            "type": "free_function",
            "return_type": return_type.strip(),
            "function_name": function_name.strip(),
            "parameters": parameters.strip(),
            "const": "const" if const else "",  # Capture const if it exists
        }
    
    return None

def generate_header_for_class(target_class, parsed_functions):
    header_code = ""
    # Filter only the methods belonging to the target class.
    methods = [func for func in parsed_functions if func["type"] == "method" and func["class_name"] == target_class]
    if methods:
        header_code += f"class __declspec(dllimport) {target_class} {{\n"
        header_code += "public:\n"
        for func in methods:
            const = " const" if func["const"] else ""
            rt = (f"{func['return_type']} " if func["return_type"] else "")
            # Remove unwanted tokens.
            rt = clean_type(func['return_type'])
            if rt:
                rt += " "
            params = clean_type(func['parameters'])
            if params.strip() == "void":
                params = ""
            header_code += f"    {rt}{func['function_name']}({params}){const};\n"
        header_code += "};\n"
    else:
        # If no methods were found for the class, generate a minimal empty definition.
        header_code += f"class __declspec(dllimport) {target_class};\n"
    return header_code

def extract_types_from_string(s):
    # Remove pointer/reference symbols and qualifiers.
    cleaned = s.replace("*", " ").replace("&", " ")
    tokens = re.findall(r"[A-Za-z_][\w:]*", cleaned)
    return tokens

def extract_custom_types_from_functions(filtered_functions, target_class):
    custom_types = set()
    for func in filtered_functions:
        raw_rt = func.get("return_type", "")
        tokens_rt = extract_types_from_string(raw_rt)
        if len(tokens_rt) > 1:
            if (tokens_rt[0] in ("class", "struct", "enum", "union") and tokens_rt[1] != target_class):
                custom_types.add(tokens_rt[1])
            elif (len(tokens_rt) > 2 and tokens_rt[0] in ("virtual", "static") and tokens_rt[1] in ("class", "struct", "enum", "union") and tokens_rt[2] != target_class):
                custom_types.add(tokens_rt[2])
        if '<' in raw_rt and '>' in raw_rt:
            inner_contents = re.findall(r'<\s*([^>]+?)\s*>', raw_rt)
            for content in inner_contents:
                for part in content.split(','):
                    part = part.strip()
                    tokens_inner = extract_types_from_string(part)
                    if (tokens_inner[0] in ("class", "struct", "enum", "union") and tokens_inner[1] != target_class):
                        custom_types.add(tokens_inner[1])
                    elif (len(tokens_inner) > 2 and tokens_inner[0] in ("virtual", "static") and tokens_inner[1] in ("class", "struct", "enum", "union") and tokens_inner[2] != target_class):
                        custom_types.add(tokens_inner[2])
        
        raw_params = func.get("parameters", "")
        param_chunks = [chunk.strip() for chunk in raw_params.split(',')]
            
        for chunk in param_chunks:
            tokens_params = extract_types_from_string(chunk)
            if len(tokens_params) > 1:
                if (tokens_params[0] in ("class", "struct", "enum", "union") and tokens_params[1] != target_class):
                    custom_types.add(tokens_params[1])
                elif (len(tokens_params) > 2 and tokens_params[0] in ("virtual", "static") and tokens_params[1] in ("class", "struct", "enum", "union") and tokens_params[2] != target_class):
                    custom_types.add(tokens_params[2])
            if '<' in chunk and '>' in chunk:
                inner_contents = re.findall(r'<\s*([^>]+?)\s*>', chunk)
                for content in inner_contents:
                    for part in content.split(','):
                        part = part.strip()
                        tokens_inner = extract_types_from_string(part)
                        if (tokens_inner[0] in ("class", "struct", "enum", "union") and tokens_inner[1] != target_class):
                            custom_types.add(tokens_inner[1])
                        elif (len(tokens_inner) > 2 and tokens_inner[0] in ("virtual", "static") and tokens_inner[1] in ("class", "struct", "enum", "union") and tokens_inner[2] != target_class):
                            custom_types.add(tokens_inner[2])

    return custom_types

def search_for_class_definition(project_folder, full_class_name):
    """
    Searches the given project folder recursively for a file that contains a definition
    for the specified class or struct. This function uses a regular expression that
    matches 'class' or 'struct' followed by any number of non-space tokens (to skip qualifiers)
    and then the target name.
    For namespaced classes, only the last part is used in the search.
    """
    search_name = full_class_name.split("::")[-1]
    # This pattern matches 'class' or 'struct', followed by any non-space tokens (like macros, qualifiers),
    # and then the search_name as a whole word.
    pattern = re.compile(r'\b(?:struct|class|enum|union)\s+(?:\S+\s+)*' + re.escape(search_name) + r'\b(?=\s*[;{])', re.MULTILINE)
    for root, dirs, files in os.walk(project_folder):
        for file in files:
            if file.endswith(('.h')):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        if pattern.search(content):
                            return True
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")
    return False

def is_namespace_type(full_class_name):
    """
    Returns True if full_class_name is namespaced (e.g., 'EBones::TYPE'),
    otherwise False (e.g., 'IAnimBind').
    """
    return len(full_class_name.split("::")) > 1

def group_placeholder_definitions(custom_types):
    ns_groups = defaultdict(set)
    non_ns = []
    for typ in sorted(custom_types):
        if is_namespace_type(typ):
            parts = typ.split("::")
            ns_key = tuple(parts[:-1])
            class_name = parts[-1]
            ns_groups[ns_key].add(class_name)
        else:
            non_ns.append(typ)
    return ns_groups, non_ns

def generate_namespace_placeholder(ns_key, class_names):
    """
    Given a namespace key (tuple of namespace parts) and a set of class names,
    generate a single forward declaration block with nested namespace blocks.
    """
    ns_open = "\n".join(f"namespace {ns} {{" for ns in ns_key)
    class_defs = "\n".join(f"    class {cls};" for cls in sorted(class_names))
    ns_close = "\n".join("}" for _ in ns_key)
    return f"{ns_open}\n{class_defs}\n{ns_close}"

def build_final_placeholders_string(custom_types):
    # Group by namespace.
    ns_groups, non_ns = group_placeholder_definitions(custom_types)
    # Build namespace placeholder blocks.
    namespace_placeholders = [generate_namespace_placeholder(ns_key, class_names) for ns_key, class_names in ns_groups.items()]
    # Build non-namespaced placeholders.
    class_placeholders = [f"class {typ};" for typ in sorted(non_ns)]
    
    # Join namespace placeholders with a blank line between each.
    ns_part = "\n\n".join(namespace_placeholders) if namespace_placeholders else ""
    # Join non-namespaced placeholders with a single newline between each.
    cls_part = "\n".join(class_placeholders) if class_placeholders else ""
    
    if ns_part and cls_part:
        return ns_part + "\n\n" + cls_part
    else:
        return ns_part or cls_part

def generate_full_header_for_class(target_class, parsed_functions):
    filtered_functions = [func for func in parsed_functions if func["type"] == "method" and func["class_name"] == target_class]
    header_content = generate_header_for_class(target_class, filtered_functions)
    
    # Extract custom types from the filtered functions.
    custom_types = extract_custom_types_from_functions(filtered_functions, target_class)
    
    # For each custom type, if not found in the project folder, keep it.
    missing_custom_types = {typ for typ in custom_types if not search_for_class_definition(PROJECT_FOLDER, typ)}
    
    placeholders_str = build_final_placeholders_string(missing_custom_types)

    if placeholders_str:
        final_header = placeholders_str + "\n\n" + header_content
    else:
        final_header = header_content
    return f"#pragma once\n\n{final_header}"

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
    if len(sys.argv) < 2:
        print("Usage: python script.py <TargetClassName>")
        return

    target_class = sys.argv[1]
    file_path = "demangled.txt"  # Replace with your file path containing function signatures.
    function_signatures = read_function_signatures_from_file(file_path)
    
    if not function_signatures:
        return

    parsed_functions = []
    for signature in function_signatures:
        signature = signature.strip()
        if signature:
            parsed_func = parse_function_signature(signature)
            if parsed_func:
                parsed_functions.append(parsed_func)
    
    full_header = generate_full_header_for_class(target_class, parsed_functions)
    
    # Create the header file named after the target class (e.g., IGame.h)
    output_filename = f"{target_class}.h"
    try:
        with open(output_filename, 'w') as header_file:
            header_file.write(full_header)
        print(f"Header file '{output_filename}' created successfully.")
    except Exception as e:
        print(f"Error writing header file '{output_filename}': {e}")
    
if __name__ == "__main__":
    main()
