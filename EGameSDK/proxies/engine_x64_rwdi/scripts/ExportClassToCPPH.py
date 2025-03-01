import re
import os
import idc
import idaapi
import idautils
import ida_kernwin
import ida_nalt
import ida_bytes
import ida_ida
import struct
from collections import defaultdict

# Configuration
PROJECT_FOLDER = "D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"
IDA_NALT_ENCODING = ida_nalt.get_default_encoding_idx(ida_nalt.BPU_1B)

# -----------------------------------------------------------------------------
# String and type formatting utilities
# -----------------------------------------------------------------------------

def fix_spacing(s):
    """Fix spacing for pointers/references, commas, and angle brackets."""
    s = re.sub(r'\s+([*&])', r'\1', s)             # Remove space before '*' or '&'
    s = re.sub(r'\s*,\s*', ', ', s)                # Ensure comma followed by one space
    s = re.sub(r'<\s+', '<', s)                    # Remove space after '<'
    s = re.sub(r'\s+>', '>', s)                    # Remove space before '>'
    s = re.sub(r'\s+', ' ', s)                     # Collapse multiple spaces
    return s.strip()

def clean_type(s):
    """Remove unwanted tokens from a type string, then fix spacing."""
    s = re.sub(r'\b(__cdecl|__ptr64|class|struct|enum|union)\b', '', s)
    return fix_spacing(s)

# -----------------------------------------------------------------------------
# Function signature parsing
# -----------------------------------------------------------------------------

def parse_function_signature(signature):
    """Parse a demangled function signature for class methods."""
    signature = signature.strip()
    
    # Pattern 1: With return type
    pattern1 = r"^(?:(public:|protected:|private:)\s+)?([\w\s:*&<>,]+)\s+(?:__cdecl\s+)?([A-Za-z_][\w:~<>]*)::([\w~<>]+)\((.*?)\)\s*(const)?$"
    m = re.match(pattern1, signature)
    if m:
        access, return_type, class_name, function_name, parameters, const = m.groups()
        return {
            "type": "method",
            "access": access.strip() if access else "public",
            "return_type": return_type.strip(),
            "class_name": class_name.strip(),
            "function_name": function_name.strip(),
            "parameters": parameters.strip(),
            "const": "const" if const else "",
        }
    
    # Pattern 2: Without return type (assume empty return type)
    pattern2 = r"^(?:(public:|protected:|private:)\s+)?([A-Za-z_][\w:~<>]*)::([\w~<>]+)\((.*?)\)\s*(const)?$"
    m = re.match(pattern2, signature)
    if m:
        access, class_name, function_name, parameters, const = m.groups()
        return {
            "type": "method",
            "access": access.strip() if access else "public",
            "return_type": "",
            "class_name": class_name.strip(),
            "function_name": function_name.strip(),
            "parameters": parameters.strip(),
            "const": "const" if const else "",
        }
    
    return None

# -----------------------------------------------------------------------------
# IDA pattern search utilities
# -----------------------------------------------------------------------------

def bytes_to_ida_pattern(data):
    """Convert bytes to IDA-friendly hex pattern string."""
    return " ".join("{:02X}".format(b) for b in data)

def get_section_info(section_name):
    """Get start address and size of a specified section."""
    for seg_ea in idautils.Segments():
        if idc.get_segm_name(seg_ea) == section_name:
            start = seg_ea
            end = idc.get_segm_end(seg_ea)
            return start, end - start
    return None, None

def find_all_patterns_in_range(pattern, start, size):
    """Find all occurrences of a pattern within a memory range."""
    addresses = []
    ea = start
    end = start + size
    
    while ea < end:
        compiled_pattern = ida_bytes.compiled_binpat_vec_t()
        err = ida_bytes.parse_binpat_str(compiled_pattern, 0, pattern, 16, IDA_NALT_ENCODING)
        if err:
            return []
            
        found = ida_bytes.bin_search(ea, end, compiled_pattern, ida_bytes.BIN_SEARCH_FORWARD)
        if found == idc.BADADDR:
            break
            
        addresses.append(found)
        ea = found + 8  # advance past found pattern
        
    return addresses

# -----------------------------------------------------------------------------
# RTTI and vtable analysis
# -----------------------------------------------------------------------------

def get_vtable_ptr(class_name):
    """
    Find vtable pointer for a class using RTTI information.
    Returns the vtable pointer (an integer) or 0 if not found.
    """
    if not class_name:
        return 0

    base_address = idaapi.get_imagebase()
    
    # Find type descriptor
    type_descriptor_name = f".?AV{class_name}@@"
    type_descriptor_bytes = type_descriptor_name.encode('ascii')
    ida_pattern = bytes_to_ida_pattern(type_descriptor_bytes)
    
    # Search in .rdata
    rdata_start, rdata_size = get_section_info(".rdata")
    if rdata_start is None:
        return 0
        
    # Look for the type descriptor
    compiled_pattern = ida_bytes.compiled_binpat_vec_t()
    err = ida_bytes.parse_binpat_str(compiled_pattern, 0, ida_pattern, 16, IDA_NALT_ENCODING)
    if err:
        return 0
        
    found_addr = ida_bytes.bin_search(rdata_start, ida_ida.cvar.inf.max_ea, compiled_pattern, ida_bytes.BIN_SEARCH_FORWARD)
    if found_addr == idc.BADADDR:
        idaapi.msg(f"Type descriptor pattern not found for {class_name}.\n")
        return 0
        
    # Adjust to get RTTI type descriptor
    rtti_type_descriptor = found_addr - 0x10
    
    # Compute offset relative to base address
    final_offset = rtti_type_descriptor - base_address
    final_offset_bytes = struct.pack("<I", final_offset)
    offset_pattern = bytes_to_ida_pattern(final_offset_bytes)
    
    # Search for references to this offset
    xrefs = find_all_patterns_in_range(offset_pattern, rdata_start, rdata_size)
    
    # Analyze each reference to find the vtable
    for xref in xrefs:
        # Check offset from class
        offset_from_class = idc.get_wide_dword(xref - 8)
        if offset_from_class != 0:
            continue
            
        # Get object locator
        object_locator = xref - 0xC
        
        # Look for references to the object locator
        object_locator_bytes = struct.pack("<Q", object_locator)
        locator_pattern = bytes_to_ida_pattern(object_locator_bytes)
        
        compiled_pattern = ida_bytes.compiled_binpat_vec_t()
        err = ida_bytes.parse_binpat_str(compiled_pattern, 0, locator_pattern, 16, IDA_NALT_ENCODING)
        if err:
            continue
            
        locator_found = ida_bytes.bin_search(rdata_start, ida_ida.cvar.inf.max_ea, compiled_pattern, ida_bytes.BIN_SEARCH_FORWARD)
        if locator_found == idc.BADADDR:
            continue
            
        # Vtable pointer is at (locator_found + 0x8)
        vtable_addr = locator_found + 8
        if vtable_addr <= 8:
            continue
            
        return vtable_addr
        
    idaapi.msg(f"Failed to locate vtable pointer for {class_name}.\n")
    return 0

def get_vtable_order(target_class):
    """
    Get the ordered list of function names from a class's vtable.
    """
    vtable_ptr = get_vtable_ptr(target_class)
    if vtable_ptr == 0:
        idaapi.msg(f"Vtable pointer not found for {target_class}.\n")
        return []
        
    order = []
    seg_end = idc.get_segm_end(vtable_ptr)
    ea = vtable_ptr
    
    while ea < seg_end:
        ptr = idc.get_qword(ea)
        if ptr == 0:
            break
            
        seg = idaapi.getseg(ptr)
        if seg is None or seg.type != idaapi.SEG_CODE:
            break
            
        func_name = idc.get_func_name(ptr) or idc.get_name(ptr)
        order.append(func_name)
        ea += 8
        
    return order

# -----------------------------------------------------------------------------
# Vtable function processing
# -----------------------------------------------------------------------------

placeholder_counter = 0

def create_placeholder_method(target_class, index):
    """Create a placeholder method dictionary for stripped virtual functions."""
    return {
        "type": "method",
        "access": "public",
        "return_type": "virtual void",
        "class_name": target_class,
        "function_name": f"StrippedVFunc{index}",
        "parameters": "",
        "const": "",
    }

def parse_vtable_functions(vtable_order, target_class):
    """
    Parse vtable entries into function dictionaries.
    Creates placeholders for functions without proper names.
    """
    global placeholder_counter
    vtable_methods = []
    
    for entry in vtable_order:
        # Attempt to demangle the name
        demangled = idaapi.demangle_name(entry, idaapi.MNG_LONG_FORM)
        
        if demangled:
            # Parse the signature
            parsed = parse_function_signature(demangled)
            if parsed and parsed["class_name"] == target_class:
                vtable_methods.append(parsed)
                continue
                
        # Create placeholder for entries we can't parse
        vtable_methods.append(create_placeholder_method(target_class, placeholder_counter))
        placeholder_counter += 1
        
    return vtable_methods

# -----------------------------------------------------------------------------
# Type extraction and processing
# -----------------------------------------------------------------------------

def extract_types_from_string(s):
    """Extract potential type names from a string."""
    # Remove pointer/reference symbols and qualifiers
    cleaned = s.replace("*", " ").replace("&", " ")
    tokens = re.findall(r"[A-Za-z_][\w:]*", cleaned)
    return tokens

def extract_custom_types_from_template(content, target_class, custom_types):
    """Extract custom types from template parameters."""
    if '<' in content and '>' in content:
        inner_contents = re.findall(r'<\s*([^>]+?)\s*>', content)
        for inner_content in inner_contents:
            for part in inner_content.split(','):
                part = part.strip()
                tokens = extract_types_from_string(part)
                
                # Add type if it matches our patterns and isn't the target class
                if tokens:
                    if (tokens[0] in ("class", "struct", "enum", "union") and 
                        len(tokens) > 1 and tokens[1] != target_class):
                        custom_types.add((tokens[0], tokens[1]))
                    elif (len(tokens) > 2 and 
                          tokens[0] in ("virtual", "static") and 
                          tokens[1] in ("class", "struct", "enum", "union") and 
                          tokens[2] != target_class):
                        custom_types.add((tokens[1], tokens[2]))

def extract_custom_types_from_functions(functions, target_class):
    """Extract all custom types from a list of function dictionaries."""
    custom_types = set()
    
    for func in functions:
        # Process return type
        raw_rt = func.get("return_type", "")
        tokens_rt = extract_types_from_string(raw_rt)
        
        if len(tokens_rt) > 1:
            if (tokens_rt[0] in ("class", "struct", "enum", "union") and 
                tokens_rt[1] != target_class):
                custom_types.add((tokens_rt[0], tokens_rt[1]))
            elif (len(tokens_rt) > 2 and 
                  tokens_rt[0] in ("virtual", "static") and 
                  tokens_rt[1] in ("class", "struct", "enum", "union") and 
                  tokens_rt[2] != target_class):
                custom_types.add((tokens_rt[1], tokens_rt[2]))
                
        # Process template parameters in return type
        extract_custom_types_from_template(raw_rt, target_class, custom_types)
        
        # Process parameters
        raw_params = func.get("parameters", "")
        param_chunks = [chunk.strip() for chunk in raw_params.split(',') if chunk.strip()]
            
        for chunk in param_chunks:
            tokens_params = extract_types_from_string(chunk)
            
            if len(tokens_params) > 1:
                if (tokens_params[0] in ("class", "struct", "enum", "union") and 
                    tokens_params[1] != target_class):
                    custom_types.add((tokens_params[0], tokens_params[1]))
                elif (len(tokens_params) > 2 and 
                      tokens_params[0] in ("virtual", "static") and 
                      tokens_params[1] in ("class", "struct", "enum", "union") and 
                      tokens_params[2] != target_class):
                    custom_types.add((tokens_params[1], tokens_params[2]))
                    
            # Process template parameters in chunk
            extract_custom_types_from_template(chunk, target_class, custom_types)

    return custom_types

def search_for_class_definition(project_folder, full_class_name):
    """Search project for existing class definitions."""
    # Stubbed out - always returns False in this version
    return False

# -----------------------------------------------------------------------------
# Forward declaration generation
# -----------------------------------------------------------------------------

def build_namespace_tree(custom_types):
    """
    Build a namespace tree from custom types.
    Returns a tree structure representing nested namespaces.
    """
    tree = {"decls": [], "children": {}}
    
    for decl, full_name in custom_types:
        parts = full_name.split("::")
        
        if len(parts) == 1:
            # No namespace: add to root declarations
            tree["decls"].append((decl, full_name))
        else:
            namespaces = parts[:-1]  # All but the last part
            type_name = parts[-1]    # The last part is the type name
            
            # Navigate to the correct namespace node
            node = tree
            for ns in namespaces:
                node = node["children"].setdefault(ns, {"decls": [], "children": {}})
                
            # Add the declaration to this namespace
            node["decls"].append((decl, type_name))
            
    return tree

def generate_namespace_tree_code(tree, indent=""):
    """
    Generate code for a namespace tree with proper indentation.
    """
    lines = []
    
    # Generate child namespace blocks first
    children = sorted(tree.get("children", {}).items(), key=lambda x: x[0])
    for idx, (ns, subtree) in enumerate(children):
        lines.append(f"{indent}namespace {ns} {{")
        child_code = generate_namespace_tree_code(subtree, indent + "    ")
        lines.append(child_code)
        lines.append(f"{indent}}}")
        
        # Add blank line between top-level namespaces
        if indent == "" and idx < len(children) - 1:
            lines.append("")
            
    # Add blank line between namespaces and declarations
    if children and tree.get("decls", []):
        lines.append("")
        
    # Output direct declarations in the current namespace
    for decl, name in sorted(tree.get("decls", []), key=lambda x: x[1]):
        lines.append(f"{indent}{decl} {name};")
        
    return "\n".join(lines)

def build_forward_declarations(custom_types):
    """Generate forward declaration code from custom types."""
    tree = build_namespace_tree(custom_types)
    return generate_namespace_tree_code(tree)

# -----------------------------------------------------------------------------
# Recursive class generation for missing types
# -----------------------------------------------------------------------------

# Track classes we've already processed to avoid circular dependencies
processed_classes = set()

def should_generate_class_definition(class_name):
    """
    Determine if a class should have a full definition generated.
    Returns True if the class has virtual or exported functions.
    Uses the cached function signatures for efficiency.
    """
    # Check if class has a vtable
    vtable_entries = get_vtable_order(class_name)
    if vtable_entries:
        return True
        
    # Check if class has any functions in our parsed functions cache
    if class_name in parsed_functions_by_class and parsed_functions_by_class[class_name]:
        return True
    
    return False

def process_missing_classes(missing_types):
    """
    Process missing types to generate class definitions when appropriate.
    Returns a tuple: (class_definitions, remaining_forward_decls)
    """
    class_definitions = []
    remaining_forward_decls = set()
    
    for decl, full_name in missing_types:
        # Only process classes, not enums, structs, etc.
        if decl != "class":
            remaining_forward_decls.add((decl, full_name))
            continue
            
        # Skip if already processed to prevent infinite recursion
        if full_name in processed_classes:
            remaining_forward_decls.add((decl, full_name))
            continue
            
        # Extract the simple class name (without namespace)
        class_name = full_name.split("::")[-1]
        
        # Check if we should generate a full definition
        if should_generate_class_definition(class_name):
            processed_classes.add(full_name)
            
            # Collect functions for this class
            parsed_functions = collect_function_signatures(class_name)
            
            if parsed_functions:
                # Generate the class definition
                class_def = generate_header_for_class(class_name, parsed_functions)
                class_definitions.append(class_def)
                
                # Note: this might recursively handle dependencies of this class
            else:
                # No functions found, just forward declare
                remaining_forward_decls.add((decl, full_name))
        else:
            # No virtual or exported functions, just forward declare
            remaining_forward_decls.add((decl, full_name))
    
    return class_definitions, remaining_forward_decls

# -----------------------------------------------------------------------------
# Header generation
# -----------------------------------------------------------------------------

first_non_virtual = False

def generate_class_method_code(func):
    """Generate code for a single class method."""
    const_str = " const" if func["const"] else ""
    rt = clean_type(func['return_type'])
    if rt:
        rt += " "
        
    params = clean_type(func['parameters'])
    if params.strip() == "void":
        params = ""
        
    return f"    {rt}{func['function_name']}({params}){const_str};"

def generate_class_definition(target_class, combined_methods):
    """Generate a class definition from a list of methods."""
    global first_non_virtual
    first_non_virtual = False
    
    # Build the class definition
    class_lines = [f"class __declspec(dllimport) {target_class} {{", "public:"]
    
    for func in combined_methods:
        # Add spacing before first non-virtual method
        if 'virtual' not in func['return_type'] and not first_non_virtual:
            first_non_virtual = True
            class_lines.append("")
            
        class_lines.append(generate_class_method_code(func))
        
    class_lines.append("};")
    return "\n".join(class_lines)

def generate_header_for_class(target_class, parsed_functions):
    """
    Generate a C++ header file for the target class.
    Organizes methods with vtable order first, then remaining methods.
    Also handles dependencies by generating classes for missing types.
    """
    # Get vtable methods
    vtable_entries = get_vtable_order(target_class)
    vtable_methods = []
    if vtable_entries:
        vtable_methods = parse_vtable_functions(vtable_entries, target_class)
    else:
        idaapi.msg(f"No vtable entries found for {target_class}. Generating normal header.\n")

    # Get non-vtable methods
    vtable_names = {m["function_name"] for m in vtable_methods}
    normal_methods = [
        f for f in parsed_functions
        if f["type"] == "method"
        and f["class_name"] == target_class
        and f["function_name"] not in vtable_names
        and "virtual" not in f["return_type"]
    ]

    # Combine methods: vtable first, then the rest
    combined_methods = vtable_methods + normal_methods

    # Generate the class definition
    class_def = generate_class_definition(target_class, combined_methods)

    # Extract custom types used by this class
    custom_types = extract_custom_types_from_functions(combined_methods, target_class)
    missing_types = {
        (decl, full_name) 
        for (decl, full_name) in custom_types 
        if not search_for_class_definition(PROJECT_FOLDER, full_name)
    }
    
    # Process missing classes - generate definitions for those with virtual/exported functions
    dependent_class_defs, types_to_forward_declare = process_missing_classes(missing_types)
    
    # Generate forward declarations for remaining types
    forward_decls = build_forward_declarations(types_to_forward_declare)

    # Combine all parts of the header
    header_parts = ["#pragma once"]
    
    # Add dependent class definitions
    if dependent_class_defs:
        for class_def in dependent_class_defs:
            # Remove the #pragma once from dependent class definitions
            clean_def = class_def.replace("#pragma once", "").strip()
            if clean_def:
                header_parts.append("\n" + clean_def)
    
    # Add forward declarations
    if forward_decls:
        header_parts.append("\n" + forward_decls)
        
    # Add the main class definition
    header_parts.append("\n" + class_def)
    
    return "".join(header_parts)

# -----------------------------------------------------------------------------
# Main functionality
# -----------------------------------------------------------------------------

# Global cache of parsed functions by class name
parsed_functions_by_class = {}
all_functions_parsed = False

def collect_function_signatures(target_class=None):
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsed_functions_by_class, all_functions_parsed
    
    # If we haven't parsed all functions yet, do it now
    if not all_functions_parsed:
        # Iterate over all functions in the database
        for ea in idautils.Functions():
            # Get function name and try to demangle it
            name = idc.get_name(ea)
            demangled = idaapi.demangle_name(name, idaapi.MNG_LONG_FORM)
            
            if demangled is None:
                continue
                
            signature = demangled.strip()
            parsed_func = parse_function_signature(signature)
            
            if parsed_func and "class_name" in parsed_func:
                class_name = parsed_func["class_name"]
                if class_name not in parsed_functions_by_class:
                    parsed_functions_by_class[class_name] = []
                parsed_functions_by_class[class_name].append(parsed_func)
        
        all_functions_parsed = True
    
    # Return the requested functions
    if target_class is None:
        # Return all parsed functions
        all_functions = []
        for funcs in parsed_functions_by_class.values():
            all_functions.extend(funcs)
        return all_functions
    else:
        # Return only functions for the specified class
        return parsed_functions_by_class.get(target_class, [])

def export_class_header(target_class):
    """Generate and save a C++ header file for the target class."""
    global processed_classes
    
    # Reset the processed classes set
    processed_classes = set()
    
    # Add the target class to processed classes to prevent recursion
    processed_classes.add(target_class)
    
    # Get parsed function signatures
    parsed_functions = collect_function_signatures(target_class)
    
    if not parsed_functions:
        ida_kernwin.msg("No matching function signatures were found in the database.\n")
        return False
        
    # Generate header code
    header_code = generate_header_for_class(target_class, parsed_functions)
    
    # Write header to file
    output_filename = f"{target_class}.h"
    try:
        with open(output_filename, 'w') as header_file:
            header_file.write(header_code)
        ida_kernwin.msg(f"Header file '{output_filename}' created successfully.\n")
        
        # Report how many additional classes were included
        additional_classes = len(processed_classes) - 1
        if additional_classes > 0:
            ida_kernwin.msg(f"Included definitions for {additional_classes} additional dependent classes.\n")
            
        return True
    except Exception as e:
        ida_kernwin.msg(f"Error writing header file '{output_filename}': {e}\n")
        return False

def main():
    """Main entry point for the script."""
    # Default target class for testing
    target_class = "IModelObject"
    
    # Uncomment to ask user for target class
    #target_class = ida_kernwin.ask_str("", 0, "Enter target class name:")
    
    if not target_class:
        ida_kernwin.msg("No target class specified. Aborting.\n")
        return
    
    breakpoint()
    export_class_header(target_class)

# -----------------------------------------------------------------------------
# IDA plugin integration
# -----------------------------------------------------------------------------

class ExportClassToCPPH(idaapi.plugin_t):
    """IDA Pro plugin for exporting C++ class definitions to header files."""
    flags = idaapi.PLUGIN_UNL
    comment = "Extract exported and virtual functions and generate a C++ header"
    help = "Extracts exported and virtual functions and generates a C++ header"
    wanted_name = "Export Class to C++ Header"
    wanted_hotkey = "Shift-F"
    
    def init(self):
        idaapi.msg(f"[DEBUG] Initializing \"{self.wanted_name}\" Plugin\n")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        idaapi.msg(f"[DEBUG] Running \"{self.wanted_name}\" Plugin\n")
        main()

    def term(self):
        idaapi.msg(f"[DEBUG] Terminating \"{self.wanted_name}\" Plugin\n")
        pass

def PLUGIN_ENTRY():
    idaapi.msg(f"[DEBUG] Creating \"Export Class to C++ Header\" Plugin entry\n")
    return ExportClassToCPPH()

if __name__ == "__main__":
    main()