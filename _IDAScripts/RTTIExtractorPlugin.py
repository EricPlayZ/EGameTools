import idaapi
import idautils
import idc
import os
import re

# List of unwanted type names
UNWANTED_TYPES = {
    "bool", "double", "float", "unsigned __int64", "__int64", 
    "unsigned short", "short", "unsigned char", "char", "void"
}

def get_rtti_classes():
    """ Extracts RTTI class names and their hierarchy from the binary. """
    rtti_classes = {}
    idaapi.msg("[DEBUG] Starting RTTI class extraction\n")
    
    for seg_ea in idautils.Segments():
        idaapi.msg(f"[DEBUG] Processing segment at {hex(seg_ea)}\n")
        for addr in idautils.Heads(seg_ea, idc.get_segm_end(seg_ea)):
            name = idc.get_name(addr)
            if name and name.startswith("??_R0"):  # Standard RTTI naming convention
                idaapi.msg(f"[DEBUG] Found RTTI name: {name} at {hex(addr)}\n")
                class_name = clean_rtti_name(name)
                # Skip empty names or names that are in the unwanted list.
                if class_name and class_name not in UNWANTED_TYPES:
                    idaapi.msg(f"[DEBUG] Cleaned class name: {class_name}\n")
                    base_classes = get_base_classes(addr)
                    idaapi.msg(f"[DEBUG] Base classes for {class_name}: {base_classes}\n")
                    rtti_classes[class_name] = base_classes
    
    return rtti_classes

def clean_rtti_name(name):
    """ Cleans and demangles RTTI class names from RTTI symbol names. """
    idaapi.msg(f"[DEBUG] Demangling name: {name}\n")
    demangled = idaapi.demangle_name(name, idaapi.MNG_NOTYPE)
    if demangled:
        # Remove RTTI descriptors or array parts
        demangled = re.sub(r'::`RTTI Base Class Descriptor at \(.*\)', '', demangled)
        demangled = re.sub(r'`RTTI Type Descriptor', '', demangled)
        demangled = re.sub(r'::`RTTI Base Class Array at \(.*\)', '', demangled)
        demangled = demangled.strip()
        # Remove any "class" or "struct" prefixes (fixing the escape issue)
        demangled = re.sub(r'\b(?:class|struct)\s+', '', demangled)
        # Remove any stray backticks or single quotes
        demangled = re.sub(r"[`']", "", demangled)
        demangled = demangled.strip()
        idaapi.msg(f"[DEBUG] Demangled name: {demangled}\n")
        return demangled
    # Fallback: try to extract from mangled name if demangling fails
    match = re.search(r'\?([\w:<>]+)@', name)
    return match.group(1) if match else name

def get_base_classes(addr):
    """ Attempts to retrieve multiple base classes from the RTTI structure. """
    base_classes = []
    
    base_class_descriptor_addr = 0
    for xAddr in idautils.DataRefsTo(addr):
        idaapi.msg(f"[DEBUG] Trying xAddr for {hex(xAddr)}\n")
        name = idc.get_name(xAddr)
        if name and name.startswith("??_R1A"):  # Standard RTTI naming convention
            idaapi.msg(f"[DEBUG] Found RTTI base class descriptor name: {name} at {hex(xAddr)}\n")
            demangled = idaapi.demangle_name(name, idaapi.MNG_NOTYPE)
            idaapi.msg(f"[DEBUG] Demangled name: {demangled}\n")
            if demangled and "RTTI Base Class Descriptor" in demangled:
                base_class_descriptor_addr = xAddr
                idaapi.msg(f"[DEBUG] Found base class descriptor: {hex(base_class_descriptor_addr)}\n")
                break
        else:
            idaapi.msg(f"[DEBUG] Not found name for {name}\n")
    
    base_class_array_addr = 0
    for xAddr in idautils.DataRefsTo(base_class_descriptor_addr):
        idaapi.msg(f"[DEBUG] Trying xAddr for {hex(xAddr)}\n")
        name = idc.get_name(xAddr)
        if name and name.startswith("??_R2"):  # Standard RTTI naming convention
            idaapi.msg(f"[DEBUG] Found RTTI base class array name: {name} at {hex(xAddr)}\n")
            demangled = idaapi.demangle_name(name, idaapi.MNG_NOTYPE)
            idaapi.msg(f"[DEBUG] Demangled name: {demangled}\n")
            if demangled and "RTTI Base Class Array" in demangled:
                base_class_array_addr = xAddr
                idaapi.msg(f"[DEBUG] Found base class array: {hex(base_class_array_addr)}\n")
                break
        else:
            idaapi.msg(f"[DEBUG] Not found name for {name}\n")

    idaapi.msg(f"[DEBUG] Fetching base classes from {hex(addr)}\n")
    idaapi.msg(f"[DEBUG] Initial base_info_addr: {hex(base_class_array_addr) if base_class_array_addr else 'None'}\n")
    
    base_class_info_addr = base_class_array_addr + 4 if base_class_array_addr else 0

    if not base_class_info_addr or not idc.is_loaded(base_class_info_addr):
        idaapi.msg("[DEBUG] No valid base class info found. Skipping.\n")
        return base_classes

    while base_class_info_addr and idc.is_loaded(base_class_info_addr):
        idaapi.msg(f"[DEBUG] Processing base_info_addr: {hex(base_class_info_addr)}\n")

        base_class_name_addr = idc.get_wide_dword(base_class_info_addr)
        if not base_class_name_addr:
            idaapi.msg("[DEBUG] No valid base class info found first. Returning!\n")
            return base_classes
        base_class_name = idc.get_name(base_class_name_addr)
        idaapi.msg(f"[DEBUG] Base class entry at {hex(base_class_name_addr)}: {base_class_name}\n")

        if base_class_name:
            demangled_name = clean_rtti_name(base_class_name)
            if demangled_name and demangled_name not in UNWANTED_TYPES:
                base_classes.append(demangled_name)

        # Move to the next base class (advance by 4 bytes)
        prev_addr = base_class_info_addr  # Store previous address for debugging
        base_class_info_addr += 4
        
        next_dword = idc.get_wide_dword(base_class_info_addr)
        if not next_dword or not idc.is_loaded(next_dword):
            idaapi.msg("[DEBUG] No valid base class info found anymore. Returning!\n")
            return base_classes

        if base_class_info_addr == prev_addr:  # Prevent infinite loops
            idaapi.msg("[ERROR] Infinite loop detected while traversing base classes! Breaking out.\n")
            break

    idaapi.msg(f"[DEBUG] Final base classes: {base_classes}\n")
    return base_classes

def generate_header(rtti_classes, output_path):
    """ Generates a C++ header file with extracted RTTI class definitions. """
    idaapi.msg(f"[DEBUG] Generating C++ header at {output_path}\n")
    with open(output_path, 'w') as f:
        f.write("// Generated C++ class definitions from IDA RTTI\n\n")
        f.write("#pragma once\n\n")
        
        for class_name, base_classes in rtti_classes.items():
            # If there are base classes, join them with "public"
            if base_classes:
                bases = ", ".join(f"public {b}" for b in base_classes)
                inheritance = f" : {bases}"
            else:
                inheritance = ""
            idaapi.msg(f"[DEBUG] Writing class {class_name}{inheritance}\n")
            f.write(f"class {class_name}{inheritance} {{}};\n")

def main():
    """ Main plugin execution. """
    output_path = os.path.join(os.getcwd(), "rtti_classes.h")
    idaapi.msg("[DEBUG] Running main function\n")
    rtti_classes = get_rtti_classes()
    generate_header(rtti_classes, output_path)
    idaapi.msg(f"RTTI header file generated at: {output_path}\n")

class RTTIExtractorPlugin(idaapi.plugin_t):
    flags = idaapi.PLUGIN_UNL
    comment = "Extract RTTI classes and generate a C++ header"
    help = "Extracts RTTI class hierarchy from binaries"
    wanted_name = "RTTI Extractor"
    wanted_hotkey = "Alt-R"
    
    def init(self):
        idaapi.msg("[DEBUG] Initializing RTTI Extractor Plugin\n")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        idaapi.msg("[DEBUG] Running RTTI Extractor Plugin\n")
        main()

    def term(self):
        idaapi.msg("[DEBUG] Terminating RTTI Extractor Plugin\n")
        pass

def PLUGIN_ENTRY():
    idaapi.msg("[DEBUG] Creating RTTI Extractor Plugin entry\n")
    return RTTIExtractorPlugin()
