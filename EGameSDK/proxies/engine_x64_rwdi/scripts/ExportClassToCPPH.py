import os
import idc
import idaapi
import idautils
import ida_kernwin
import ida_nalt
import ida_bytes
import ida_ida
import struct
import re
from typing import Optional, Tuple
from dataclasses import dataclass, field

IDA_NALT_ENCODING = ida_nalt.get_default_encoding_idx(ida_nalt.BPU_1B)
CLASS_TYPES = ("class", "struct", "enum", "union")
FUNC_QUALIFIERS = ("virtual", "static")
NON_VALID_VIRTUAL_FUNCS = ("SetPlatform", "RunUnitTests", "_purecall")

# Configuration
INTERNAL_SCRIPT_NAME = "ExportClassToCPPH"
PROJECT_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"
OUTPUT_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\proxies\engine_x64_rwdi\scripts\generated"
CACHE_FOLDER = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\proxies\engine_x64_rwdi\scripts\cache"
GENERATE_CLASS_DEFS_MISSING_TYPES = False   # Flag to generate full class definitions for missing types, or just forward declare if false
SEARCH_CLASS_DEFS_IN_PROJECT_FOLDER = False # Flag to search for missing class types in the PROJECT_FOLDER

virtualFuncPlaceholderCounter: int = 0 # Counter for placeholder virtual functions

def PrintMsg(*args):
    #ida_kernwin.msg(f"[{INTERNAL_SCRIPT_NAME}] {args}")
    print(f"[{INTERNAL_SCRIPT_NAME}] {args}")

# -----------------------------------------------------------------------------
# String and type formatting utilities
# -----------------------------------------------------------------------------

def FixTypeSpacing(type: str) -> str:
    """Fix spacing for pointers/references, commas, and angle brackets."""
    type = re.sub(r'\s+([*&])', r'\1', type)             # Remove space before '*' or '&'
    type = re.sub(r'\s*,\s*', ', ', type)                # Ensure comma followed by one space
    type = re.sub(r'<\s+', '<', type)                    # Remove space after '<'
    type = re.sub(r'\s+>', '>', type)                    # Remove space before '>'
    type = re.sub(r'\s+', ' ', type)                     # Collapse multiple spaces
    return type.strip()

def CleanType(type: str) -> str:
    """Remove unwanted tokens from a type string, then fix spacing."""
    type = re.sub(r'\b(__cdecl|__ptr64|class|struct|enum|union)\b', '', type)
    return FixTypeSpacing(type)

def ExtractTypesFromString(types: str) -> list[str]:
    """Extract potential type names from a string."""
    # Remove pointer/reference symbols and qualifiers
    cleanedTypes: str = types.replace("*", " ").replace("&", " ")
    cleanedTypesList: list[str] = re.findall(r"[A-Za-z_][\w:]*", cleanedTypes)
    return cleanedTypesList

# -----------------------------------------------------------------------------
# Various class implementations
# -----------------------------------------------------------------------------

@dataclass(frozen=True)
class ClassName:
    """Split a potentially namespaced class name into namespace parts and class name."""
    namespaces: tuple[str] = field(default_factory=tuple)
    name: str = ""
    fullName: str = ""

    def __init__(self, fullName: str):
        object.__setattr__(self, "namespaces", ())
        object.__setattr__(self, "name", "")
        object.__setattr__(self, "fullName", "")
        
        fullName = (fullName or "").strip()
        if not fullName:
            return
        
        object.__setattr__(self, "fullName", fullName)
        types = ExtractTypesFromString(fullName)
        if len(types) > 1:
            if (types[0] in CLASS_TYPES):
                fullName = types[1]
            elif (len(types) > 2 and types[0] in FUNC_QUALIFIERS and types[1] in CLASS_TYPES):
                fullName = types[2]
            else:
                return

        parts = fullName.split("::")
        if len(parts) == 1:
            object.__setattr__(self, "name", parts[0])
        else:
            object.__setattr__(self, "namespaces", tuple(parts[:-1]))
            object.__setattr__(self, "name", parts[-1])

@dataclass(frozen=True)
class ParsedFunction:
    """Parse a demangled function signature and return a ParsedFunction instance."""
    type: str = ""
    access: str = ""
    returnType: Optional[ClassName] = None
    className: Optional[ClassName] = None
    funcName: str = ""
    params: str = ""
    const: bool = False

    def __init__(self, signature: str, onlyVirtualFuncs: bool):
        global virtualFuncPlaceholderCounter

        object.__setattr__(self, "type", "")
        object.__setattr__(self, "access", "")
        object.__setattr__(self, "returnType", None)
        object.__setattr__(self, "className", None)
        object.__setattr__(self, "funcName", "")
        object.__setattr__(self, "params", "")
        object.__setattr__(self, "const", False)

        signature = signature.strip()
        remainingInput: str = signature
        
        access: str = ""
        if signature.startswith("public:"):
            access = "public"
        elif signature.startswith("protected:"):
            access = "protected"
        elif signature.startswith("private:"):
            access = "private"
        remainingInput = signature[len(access)+1:].strip()
        
        # Find parameters and const qualifier
        paramsOpenParenIndex: int = remainingInput.find('(')
        paramsCloseParenIndex: int = remainingInput.rfind(')')
        
        if paramsOpenParenIndex != -1 and paramsCloseParenIndex != -1:
            params: str = remainingInput[paramsOpenParenIndex + 1:paramsCloseParenIndex]

            remainingInputBeforeParamsParen: str = remainingInput[:paramsOpenParenIndex].strip()
            remainingInputAfterParamsParen: str = remainingInput[paramsCloseParenIndex + 1:].strip()
            const: str = "const" if "const" in remainingInputAfterParamsParen else ""
            
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
                returnType: str = remainingInputBeforeParamsParen[:lastSpaceIndex].strip()
                if onlyVirtualFuncs and "virtual" not in returnType:
                    returnType = "virtual " + returnType

                classAndFuncName: str = remainingInputBeforeParamsParen[lastSpaceIndex + 1:].strip()
                className: str = "::".join(classAndFuncName.split("::")[:-1])
                funcName: str = classAndFuncName.split("::")[-1]
            else:
                returnType: str = ""
                
                templateDepth = 0
                # Find the last class separator outside of angle brackets
                for i in range(len(remainingInputBeforeParamsParen)):
                    if remainingInputBeforeParamsParen[i] == '<':
                        templateDepth += 1
                    elif remainingInputBeforeParamsParen[i] == '>':
                        templateDepth -= 1
                    elif templateDepth == 0 and remainingInputBeforeParamsParen[i:i+2] == '::':
                        lastClassSeparatorIndex = i
            
                classAndFuncName: str = remainingInputBeforeParamsParen.strip()
                className: str = classAndFuncName[:lastClassSeparatorIndex]
                funcName: str = classAndFuncName[lastClassSeparatorIndex + 2:]

            if lastSpaceIndex != -1 or lastClassSeparatorIndex != -1:
                object.__setattr__(self, "type", "vfunc" if onlyVirtualFuncs else "func")
                object.__setattr__(self, "access", access if access else "public")
                object.__setattr__(self, "returnType", ClassName(returnType))
                object.__setattr__(self, "className", ClassName(className))
                object.__setattr__(self, "funcName", funcName)
                object.__setattr__(self, "params", params)
                object.__setattr__(self, "const", bool(const))
                return

        # Generate a simple virtual void function
        if onlyVirtualFuncs:
            object.__setattr__(self, "type", "stripped_vfunc")
            returnType = "virtual void"
            if signature in NON_VALID_VIRTUAL_FUNCS:
                signature = f"StrippedVFunc{virtualFuncPlaceholderCounter}"
                virtualFuncPlaceholderCounter += 1
            object.__setattr__(self, "returnType", ClassName(returnType))
            object.__setattr__(self, "funcName", signature)

# Global caches
parsedFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
parsedVTableFuncsByClass: dict[ClassName, list[ParsedFunction]] = {} # Cache of parsed functions by class name
allFunctionsAreParsed = False # Flag to indicate if all functions have been parsed
processedClasses: set[ClassName] = set() # Track classes we've already processed to avoid recursion
template_class_info = {} # Store information about detected template classes

# -----------------------------------------------------------------------------
# IDA util functions
# -----------------------------------------------------------------------------

def DemangleFuncSig(funcSig: str) -> str:
    return idaapi.demangle_name(funcSig, idaapi.MNG_LONG_FORM)

# -----------------------------------------------------------------------------
# Namespace and template handling utilities
# -----------------------------------------------------------------------------

def count_template_params(template_str):
    """
    Count the number of top-level template parameters while respecting nesting.
    
    Example: 
    - "bool" -> 1
    - "bool, int" -> 2
    - "bool, ttl::vector_allocators::heap_allocator<bool>, 8" -> 3
    """
    if not template_str:
        return 0
    
    # Count top-level commas (ignoring those in nested templates)
    count = 1  # Start with 1 since n params have n-1 commas between them
    bracket_depth = 0
    
    for char in template_str:
        if char == '<':
            bracket_depth += 1
        elif char == '>':
            bracket_depth -= 1
        elif char == ',' and bracket_depth == 0:
            count += 1
            
    return count

def parse_template_class(full_name):
    """
    Parse a possibly templated class name.
    Returns (base_name, template_params, is_templated)
    
    Examples:
    - "X" -> ("X", [], False)
    - "X<T>" -> ("X", ["T"], True)
    - "X<T, U>" -> ("X", ["T", "U"], True)
    """
    template_params = []
    is_templated = False
    base_name = full_name
    
    # Handle nested templates with balanced bracket matching
    if '<' in full_name:
        # Find the position of the first '<'
        start_pos = full_name.find('<')
        if start_pos > 0:
            base_name = full_name[:start_pos]
            is_templated = True
            
            # Extract the entire template parameter string
            template_str = ""
            bracket_depth = 0
            
            for i in range(start_pos, len(full_name)):
                char = full_name[i]
                if char == '<':
                    bracket_depth += 1
                    if bracket_depth == 1:
                        continue  # Skip the opening bracket of the main template
                elif char == '>':
                    bracket_depth -= 1
                    if bracket_depth == 0:
                        break  # End of template parameters
                
                if bracket_depth > 0:
                    template_str += char
            
            # Split parameters at top level
            if template_str:
                bracket_depth = 0
                param_start = 0
                
                for i in range(len(template_str)):
                    char = template_str[i]
                    if char == '<':
                        bracket_depth += 1
                    elif char == '>':
                        bracket_depth -= 1
                    elif char == ',' and bracket_depth == 0:
                        # Extract the parameter
                        param = template_str[param_start:i].strip()
                        if param:  # Only add non-empty parameters
                            template_params.append(param)
                        param_start = i + 1
                
                # Add the last parameter
                if param_start < len(template_str):
                    param = template_str[param_start:].strip()
                    if param:  # Only add non-empty parameters
                        template_params.append(param)
    
    return base_name, template_params, is_templated

def extract_template_info(template_class_name):
    """
    Extract template information from a template class name.
    
    Returns:
    - base_name: The class name without template parameters
    - param_count: Number of template parameters
    """
    base_name, template_params, is_templated = parse_template_class(template_class_name)
    
    if is_templated:
        # Register the template with its parameter count
        register_template_class(base_name, len(template_params))
        return base_name, len(template_params)
    
    return template_class_name, 0

def is_template_class(class_name):
    """Check if a class name contains template parameters."""
    return '<' in class_name and '>' in class_name

def extract_template_class_name(full_template_name):
    """
    Extract the base class name from a template class name.
    Example: "ttl::vector<int>" -> "ttl::vector"
    """
    template_start = full_template_name.find('<')
    if template_start > 0:
        return full_template_name[:template_start]
    return full_template_name

def generate_template_param_placeholders(num_params):
    """
    Generate template parameter placeholders based on the number of parameters.
    For one parameter, use T. For multiple, use T1, T2, etc.
    """
    if num_params == 0:
        return []
    elif num_params == 1:
        return ["typename T"]
    else:
        return [f"typename T{i+1}" for i in range(num_params)]

def register_template_class(full_name, params_count):
    """Register a template class with the number of parameters it takes."""
    base_name = extract_template_class_name(full_name)
    template_class_info[base_name] = params_count
    
    # Also store the full namespaced version if it differs
    if base_name != full_name and "::" in full_name:
        template_class_info[full_name] = params_count

def GetMangledTypePrefix(targetClass: ClassName) -> str:
    """
    Get the appropriate mangled type prefix for a class name.
    For class "X" this would be ".?AVX@@"
    For class "NS::X" this would be ".?AVX@NS@@"
    For templated classes, best to use get_mangled_name_for_template instead.
    """
    if not targetClass.namespaces:
        return f".?AV{targetClass.name}@@"
    
    # For namespaced classes, the format is .?AVClassName@Namespace@@
    # For nested namespaces, they are separated with @ in reverse order
    mangledNamespaces = "@".join(reversed(targetClass.namespaces))
    return f".?AV{targetClass.name}@{mangledNamespaces}@@"

def get_mangled_name_for_template(namespace, class_name, template_params):
    """
    Get the appropriate mangled name for a templated class.
    Much simplified implementation that only handles specific known cases.
    
    Example:
    - ttl::string_const<char> -> .?AV?$string_const@D@ttl@@
    """
    # Map of C++ types to MSVC mangled codes
    type_to_code = {
        'char': 'D',
        'int': 'H',
        'unsigned int': 'I',
        'long': 'J',
        'unsigned long': 'K',
        'float': 'M',
        'double': 'N',
        # Add more mappings as needed
    }
    
    # Convert template parameters to their mangled codes
    param_codes = []
    for param in template_params:
        if param in type_to_code:
            param_codes.append(type_to_code[param])
        else:
            # For user-defined types or unknown types, just use the name
            # (This is a simplification, real MSVC mangling is more complex)
            param_codes.append(param)
    
    # Create the mangled name
    param_part = "@".join(param_codes)
    
    if not namespace:
        return f".?AV?${class_name}@{param_part}@@"
    
    # Handle the namespace
    if isinstance(namespace, list):
        namespace_part = "@".join(reversed(namespace))
    else:
        namespace_part = namespace
    
    return f".?AV?${class_name}@{param_part}@{namespace_part}@@"

# -----------------------------------------------------------------------------
# IDA pattern search utilities
# -----------------------------------------------------------------------------

def BytesToIDAPattern(data: bytes) -> str:
    """Convert bytes to IDA-friendly hex pattern string."""
    return " ".join("{:02X}".format(b) for b in data)

def GetSectionInfo(sectionName: str) -> Tuple[int, int]:
    """Get start address and size of a specified section."""
    for seg_ea in idautils.Segments():
        if idc.get_segm_name(seg_ea) == sectionName:
            start = seg_ea
            end = idc.get_segm_end(seg_ea)
            return start, end - start
    return 0, 0

def FindAllPatternsInRange(pattern: str, start: int, size: int) -> list[int]:
    """Find all occurrences of a pattern within a memory range."""
    addresses: list[int] = []
    ea: int = start
    end: int = start + size
    
    while ea < end:
        compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
        errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, pattern, 16, IDA_NALT_ENCODING)
        if errorParsingIDAPattern:
            return []
            
        patternAddr: int = ida_bytes.bin_search(ea, end, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
        if patternAddr == idc.BADADDR:
            break
            
        addresses.append(patternAddr)
        ea = patternAddr + 8  # advance past found pattern
        
    return addresses

# -----------------------------------------------------------------------------
# Inheritance tracking
# -----------------------------------------------------------------------------

# Dictionary to track inheritance relationships: class_name -> [base_classes]
inheritance_map = {}

def register_inheritance(derived_class, base_class):
    """Register an inheritance relationship between derived and base class."""
    if derived_class not in inheritance_map:
        inheritance_map[derived_class] = []
    
    if base_class not in inheritance_map[derived_class]:
        inheritance_map[derived_class].append(base_class)
        ida_kernwin.msg(f"Registered inheritance: {derived_class} inherits from {base_class}\n")

def get_base_classes(class_name):
    """Get the direct base classes for a given class."""
    return inheritance_map.get(class_name, [])

def get_all_base_classes(class_name, visited=None):
    """
    Get all base classes for a given class, including indirect base classes.
    Returns a list of class names.
    """
    if visited is None:
        visited = set()
    
    if class_name in visited:
        return []
    
    visited.add(class_name)
    base_classes = get_base_classes(class_name)
    all_bases = base_classes.copy()
    
    for base in base_classes:
        indirect_bases = get_all_base_classes(base, visited)
        for indirect_base in indirect_bases:
            if indirect_base not in all_bases:
                all_bases.append(indirect_base)
    
    return all_bases

# -----------------------------------------------------------------------------
# RTTI and vtable analysis
# -----------------------------------------------------------------------------

def GetVTablePtr(targetClass: ClassName, targetClassRTTIName: str = "") -> int:
    """
    Find vtable pointer for a class using RTTI information.
    Supports both simple class names and namespaced class names.
    For templated classes, you can directly provide the rtti_name pattern.
    
    Returns the vtable pointer (an integer) or 0 if not found.
    """
    baseDLLAddr: int = idaapi.get_imagebase()
    
    # Use provided RTTI name if available (for templates), otherwise generate it
    if not targetClassRTTIName:
        # Check if this is a templated class
        typeDescriptorName: str = GetMangledTypePrefix(targetClass)
    else:
        # Use the provided RTTI name directly
        typeDescriptorName: str = targetClassRTTIName
    
    # Search for the RTTI type descriptor
    typeDescriptorBytes: bytes = typeDescriptorName.encode('ascii')
    idaPattern: str = BytesToIDAPattern(typeDescriptorBytes)
    
    # Search in .rdata
    rdataStartAddr, rdataSize = GetSectionInfo(".rdata")
    if not rdataStartAddr:
        return 0
        
    # Look for the type descriptor
    compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
    errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, idaPattern, 16, IDA_NALT_ENCODING)
    if errorParsingIDAPattern:
        return 0
        
    typeDescriptorPatternAddr: int = ida_bytes.bin_search(rdataStartAddr, ida_ida.cvar.inf.max_ea, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
    if typeDescriptorPatternAddr == idc.BADADDR:
        PrintMsg(f"Type descriptor pattern '{typeDescriptorName}' not found for {targetClass.fullName}.\n")
        return 0
        
    # Adjust to get RTTI type descriptor
    rttiTypeDescriptorAddr: int = typeDescriptorPatternAddr - 0x10
    
    # Compute offset relative to base address
    rttiTypeDescriptorOffset: int = rttiTypeDescriptorAddr - baseDLLAddr
    rttiTypeDescriptorOffsetBytes: bytes = struct.pack("<I", rttiTypeDescriptorOffset)
    rttiTypeDescriptorOffsetPattern: str = BytesToIDAPattern(rttiTypeDescriptorOffsetBytes)
    
    # Search for references to this offset
    xrefs: list[int] = FindAllPatternsInRange(rttiTypeDescriptorOffsetPattern, rdataStartAddr, rdataSize)
    
    # Analyze each reference to find the vtable
    for xref in xrefs:
        xref: int

        # Check offset from class
        offsetFromClass: int = idc.get_wide_dword(xref - 8)
        if offsetFromClass:
            continue
            
        # Get object locator
        objectLocatorOffsetAddr: int = xref - 0xC
        
        # Look for references to the object locator
        objectLocatorBytes: bytes = struct.pack("<Q", objectLocatorOffsetAddr)
        objectLocatorPattern: str = BytesToIDAPattern(objectLocatorBytes)
        
        compiledIDAPattern = ida_bytes.compiled_binpat_vec_t()
        errorParsingIDAPattern = ida_bytes.parse_binpat_str(compiledIDAPattern, 0, objectLocatorPattern, 16, IDA_NALT_ENCODING)
        if errorParsingIDAPattern:
            continue
            
        objectLocatorAddr: int = ida_bytes.bin_search(rdataStartAddr, ida_ida.cvar.inf.max_ea, compiledIDAPattern, ida_bytes.BIN_SEARCH_FORWARD)
        if objectLocatorAddr == idc.BADADDR:
            continue
            
        # Vtable pointer is at (objectLocatorAddr + 0x8)
        vtableAddr: int = objectLocatorAddr + 8
        if vtableAddr <= 8:
            continue
            
        return vtableAddr
        
    PrintMsg(f"Failed to locate vtable pointer for {targetClass.fullName}.\n")
    return 0

def GetDemangledVTableFuncSigs(targetClass: ClassName, targetClassRTTIName: str = "") -> list[str]:
    """
    Get the ordered list of function names from a class's vtable.
    For templated classes, you can provide the rtti_name pattern.
    """
    vtablePtr: int = GetVTablePtr(targetClass, targetClassRTTIName)
    if not vtablePtr:
        idaapi.msg(f"Vtable pointer not found for {targetClass.fullName}.\n")
        return []
        
    demangledVTableFuncSigsList: list[str] = []
    segmEnd: int = idc.get_segm_end(vtablePtr)
    ea: int = vtablePtr
    
    while ea < segmEnd:
        ptr: int = idc.get_qword(ea)
        if not ptr:
            break
        seg = idaapi.getseg(ptr)
        if seg is None or seg.type != idaapi.SEG_CODE:
            break
            
        funcSig: str = idc.get_func_name(ptr) or idc.get_name(ptr)
        if not funcSig:
            ea += 8
            continue
        demangledFuncSig: str = DemangleFuncSig(funcSig)
        demangledVTableFuncSigsList.append(demangledFuncSig if demangledFuncSig else funcSig)
        ea += 8
        
    return demangledVTableFuncSigsList

def GetParsedVTableFuncs(targetClass: ClassName) -> list[ParsedFunction]:
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedVTableFuncsByClass
    
    if targetClass not in parsedVTableFuncsByClass:
        parsedVTableFuncsByClass[targetClass] = []

        for demangledVTableFuncSig in GetDemangledVTableFuncSigs(targetClass):
            demangledVTableFuncSig: str

            parsedFunc: ParsedFunction = ParsedFunction(demangledVTableFuncSig, True)
            if not parsedFunc.className:
                object.__setattr__(parsedFunc, "className", targetClass)
            
            parsedVTableFuncsByClass[targetClass].append(parsedFunc)
        
    return parsedVTableFuncsByClass.get(targetClass, [])

# -----------------------------------------------------------------------------
# Type extraction and processing
# -----------------------------------------------------------------------------

def extract_template_type(template_type):
    """
    Extract the base class name and template parameters from a template type.
    
    Example:
    - "ttl::vector<int>" -> ("ttl::vector", ["int"])
    - "ttl::vector_allocators::heap_allocator<class ttl::string_base<char>>, 1>" 
      -> ("ttl::vector_allocators::heap_allocator", ["class ttl::string_base<char>", "1"])
    """
    try:
        base_name, template_params, is_templated = parse_template_class(template_type)
        if is_templated:
            return base_name, template_params
        return template_type, []
    except Exception as e:
        idaapi.msg(f"Error parsing template type {template_type}: {str(e)}\n")
        return template_type, []

def extract_custom_types_from_template(content, target_class, custom_types):
    """Extract custom types from template parameters."""
    if '<' in content and '>' in content:
        # Try to extract template info
        try:
            base_name, template_params, is_templated = parse_template_class(content)
            if is_templated:
                # Extract and register template class information
                full_template_class = extract_template_class_name(content)
                register_template_class(full_template_class, len(template_params))
                
                # Add the base class as a custom type
                tokens = ExtractTypesFromString(base_name)
                for token in tokens:
                    if token not in ("class", "struct", "enum", "union", "typename", "template", "virtual", "static"):
                        # Check if token is a class, not primitive type or keyword
                        if token not in ("int", "char", "bool", "float", "double", "void", "unsigned", "signed", "long", "short"):
                            if ':' in token:  # Namespaced class
                                custom_types.add(("class", token))
                
                # Process each template parameter
                for param in template_params:
                    param = param.strip()
                    
                    # Check if the parameter itself is a templated type
                    if '<' in param and '>' in param:
                        # Recursively process nested templates
                        extract_custom_types_from_template(param, target_class, custom_types)
                    else:
                        tokens = ExtractTypesFromString(param)
                        for i, token in enumerate(tokens):
                            if token not in ("class", "struct", "enum", "union", "typename", "template", "virtual", "static", 
                                             "int", "char", "bool", "float", "double", "void", "unsigned", "signed", "long", "short"):
                                if i > 0 and tokens[i-1] in ("class", "struct", "enum", "union"):
                                    custom_types.add((tokens[i-1], token))
                                elif ':' in token:  # Namespaced class
                                    custom_types.add(("class", token))
        except Exception as e:
            idaapi.msg(f"Error processing template: {content} - {str(e)}\n")

def ExtractCustomTypesFromFuncs(functions, target_class):
    """Extract all custom types from a list of function dictionaries."""
    custom_types = set()
    
    for func in functions:
        # Process return type
        raw_rt = func.get("return_type", "")
        tokens_rt = ExtractTypesFromString(raw_rt)
        
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
            tokens_params = ExtractTypesFromString(chunk)
            
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
    """
    Searches the given project folder recursively for a file that contains a definition
    for the specified class/struct/enum/union. This function uses a regex that matches
    'struct', 'class', 'enum', or 'union' followed by any non-space tokens and then the
    target name, which is then followed by either a semicolon or an opening brace.
    For namespaced classes, only the last part is used.
    """
    if not SEARCH_CLASS_DEFS_IN_PROJECT_FOLDER:
        return False
    
    search_name = full_class_name.split("::")[-1]
    pattern = re.compile(r'\b(?:struct|class|enum|union)\s+(?:\S+\s+)*' + re.escape(search_name) + r'\b(?=\s*[;{])', re.MULTILINE)
    for root, dirs, files in os.walk(project_folder):
        for file in files:
            if file.endswith('.h'):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        if pattern.search(content):
                            return True
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")
    return False

# -----------------------------------------------------------------------------
# Forward declaration generation
# -----------------------------------------------------------------------------

def is_template_class_name(name):
    """Check if a class name represents a template class (has been registered)."""
    # First, remove any namespace
    if '::' in name:
        _, class_name = name.split('::')[-2:]
    else:
        class_name = name
    
    # Check in template_class_info or if it's explicitly a template
    return name in template_class_info or class_name in template_class_info or is_template_class(name)

def get_template_param_count(class_name):
    """Get the number of template parameters for a registered template class."""
    # Try the full name first
    if class_name in template_class_info:
        return template_class_info[class_name]
    
    # Try without namespace
    if '::' in class_name:
        _, simple_name = class_name.split('::')[-2:]
        if simple_name in template_class_info:
            return template_class_info[simple_name]
    
    # Default to 1 parameter if we don't know
    return 1

def build_namespace_tree(custom_types):
    """
    Build a namespace tree from custom types.
    Returns a tree structure representing nested namespaces.
    """
    tree = {"decls": [], "children": {}}
    
    for decl, full_name in custom_types:
        # Check if this might be a template class
        is_template = is_template_class_name(full_name)
        
        # Extract the template param count if it's a template
        param_count = get_template_param_count(full_name) if is_template else 0
        
        parts = full_name.split("::")
        
        if len(parts) == 1:
            # No namespace: add to root declarations
            tree["decls"].append((decl, full_name, is_template, param_count))
        else:
            namespaces = parts[:-1]  # All but the last part
            type_name = parts[-1]    # The last part is the type name
            
            # Navigate to the correct namespace node
            node = tree
            for ns in namespaces:
                node = node["children"].setdefault(ns, {"decls": [], "children": {}})
                
            # Add the declaration to this namespace
            node["decls"].append((decl, type_name, is_template, param_count))
            
    return tree

def generate_namespace_tree_code(tree, indent=""):
    """
    Generate code for a namespace tree with proper indentation.
    Now includes template declarations where appropriate.
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
    for decl, name, is_template, param_count in sorted(tree.get("decls", []), key=lambda x: x[1]):
        if is_template:
            # Generate template declaration with appropriate placeholder params
            template_params = generate_template_param_placeholders(param_count)
            template_header = f"template <{', '.join(template_params)}>"
            lines.append(f"{indent}{template_header}")
            lines.append(f"{indent}{decl} {name};")
        else:
            # Regular declaration
            lines.append(f"{indent}{decl} {name};")
        
    return "\n".join(lines)

def build_forward_declarations(custom_types):
    """Generate forward declaration code from custom types."""
    tree = build_namespace_tree(custom_types)
    return generate_namespace_tree_code(tree)

# -----------------------------------------------------------------------------
# Function collection and parsing
# -----------------------------------------------------------------------------

def GetDemangledFuncSigs() -> list[str]:
    """
    Generate a list of demangled function sigs from IDA's database
    """
    cachedFilePath = os.path.join(CACHE_FOLDER, "demangled_func_sigs_cache.txt")
    
    if os.path.exists(cachedFilePath):
        try:
            with open(cachedFilePath, "r") as cachedFile:
                return [line.strip() for line in cachedFile if line.strip()]
        except Exception as e:
            PrintMsg(f"Error opening '{cachedFilePath}' for reading: {e}\n")
        
    demangledFuncSigsList: list[str] = []

    for i in range(idc.get_entry_qty()):
        ea: int = idc.get_entry(i)
        funcSig: str = idc.get_func_name(ea) or idc.get_name(ea)
        if not funcSig:
            continue
        demangledFuncSig: str = DemangleFuncSig(funcSig)
        if not demangledFuncSig:
            continue

        if demangledFuncSig not in demangledFuncSigsList:
            demangledFuncSigsList.append(demangledFuncSig)

    os.makedirs(CACHE_FOLDER, exist_ok=True)
    try:
        with open(cachedFilePath, "w") as cachedFile:
            for demangledFuncSig in demangledFuncSigsList:
                cachedFile.write(demangledFuncSig + "\n")
    except Exception as e:
        PrintMsg(f"Error opening '{cachedFilePath}' for writing: {e}\n")
    
    return demangledFuncSigsList

def GetParsedFuncs(targetClass: Optional[ClassName] = None) -> list[ParsedFunction]:
    """
    Collect and parse all function signatures from the IDA database.
    If target_class is provided, only return functions for that class.
    Caches results for better performance on subsequent calls.
    """
    global parsedFuncsByClass, allFunctionsAreParsed
    
    if not allFunctionsAreParsed:
        for demangledFuncSig in GetDemangledFuncSigs():
            demangledFuncSig: str

            # Skip invalid functions
            if demangledFuncSig.endswith("::$TSS0") or "::`vftable'" in demangledFuncSig or "virtual" in demangledFuncSig:
                continue

            parsedFunc: ParsedFunction = ParsedFunction(demangledFuncSig, False)
            if not parsedFunc.type or not parsedFunc.className:
                PrintMsg(f"Failed parsing func sig: \"{demangledFuncSig}\"")
                continue
            
            # Check if this is from a template class and extract template info
            # if "<" in class_name and "<" in class_name:
            #     base_class_name, param_count = extract_template_info(class_name)
            #     idaapi.msg(f"Detected template class: {base_class_name} with {param_count} parameters\n")
            
            # Also extract templates from parameters and return types
            # if 'parameters' in parsed_func:
            #     params = parsed_func['parameters']
            #     if '<' in params and '>' in params:
            #         extract_custom_types_from_template(params, class_name, set())
            
            # if 'return_type' in parsed_func:
            #     return_type = parsed_func['return_type']
            #     if '<' in return_type and '>' in return_type:
            #         extract_custom_types_from_template(return_type, class_name, set())
            
            if parsedFunc.className not in parsedFuncsByClass:
                parsedFuncsByClass[parsedFunc.className] = []
            parsedFuncsByClass[parsedFunc.className].append(parsedFunc)
        
        allFunctionsAreParsed = True
    
    # Return the requested functions
    if not targetClass:
        # Return all parsed functions
        all_functions: list[ParsedFunction] = []
        for funcs in parsedFuncsByClass.values():
            funcs: list[ParsedFunction]
            all_functions.extend(funcs)
        return all_functions
    else:
        # Return only functions for the specified class
        return parsedFuncsByClass.get(targetClass, [])

def ShouldGenerateClassDef(class_name):
    """
    Determine if a class should have a full definition generated.
    Returns True if the class has virtual or exported functions.
    Uses the cached function signatures for efficiency.
    """
    # Extract base name if this is a template
    if is_template_class(class_name):
        class_name = extract_template_class_name(class_name)
    
    # Check if class has a vtable
    vtable_entries = GetDemangledVTableFuncSigs(class_name)
    if vtable_entries:
        return True
        
    # Check if class has any functions in our parsed functions cache
    if class_name in parsedFuncsByClass and parsedFuncsByClass[class_name]:
        return True
    
    return False

def ProcessMissingTypes(missing_types):
    """
    Process missing types to generate class definitions when appropriate.
    Returns a tuple: (class_definitions, remaining_forward_decls)
    """
    class_definitions = []
    remaining_forward_decls = set()
    
    for decl, full_name in missing_types:          
        # Skip if already processed to prevent infinite recursion
        if full_name in processedClasses:
            #remaining_forward_decls.add((decl, full_name))
            continue
            
        # Extract the simple class name (without namespace)
        class_name = full_name.split("::")[-1]
        
        # Check if we should generate a full definition
        if GENERATE_CLASS_DEFS_MISSING_TYPES and ShouldGenerateClassDef(class_name):
            processedClasses.add(full_name)
            
            # Collect functions for this class
            parsed_functions = GetParsedFuncs(class_name)
            
            # if parsed_functions:
            #     # Generate the class definition
            #     class_def = GenerateHeaderCode(class_name, parsed_functions)
            #     class_definitions.append(class_def)
                
            #     # Note: this might recursively handle dependencies of this class
            # else:
            #     # No functions found, just forward declare
            #     remaining_forward_decls.add((decl, full_name))
        else:
            # No virtual or exported functions, just forward declare
            remaining_forward_decls.add((decl, full_name))
    
    return class_definitions, remaining_forward_decls

# -----------------------------------------------------------------------------
# Header generation
# -----------------------------------------------------------------------------

def GenerateClassFuncCode(func: ParsedFunction) -> str:
    """Generate code for a single class method."""
    const: str = " const" if func.const else ""
    stripped_vfunc: str = " = 0" if func.type == "stripped_vfunc" else ""
    if func.returnType:
        returnType: str = CleanType(func.returnType.fullName)
        if returnType:
            returnType += " "
    else:
        returnType: str = ""
        
    params: str = CleanType(func.params)
    if params == "void":
        params = ""
        
    return f"    {returnType}{func.funcName}({params}){const}{stripped_vfunc};"

def GenerateClassDefinition(targetClass: ClassName, allParsedClassFuncs: tuple[list[ParsedFunction], list[ParsedFunction]]) -> str:
    """Generate a class definition from a list of methods."""
    # Build the class definition
    classLines: list[str] = [f"class __declspec(dllimport) {targetClass.name} {{", "public:"]
    
    for vTableFunc in allParsedClassFuncs[0]:
        classLines.append(GenerateClassFuncCode(vTableFunc))
    if allParsedClassFuncs[0]:
        classLines.append("")
    for func in allParsedClassFuncs[1]:
        classLines.append(GenerateClassFuncCode(func))
        
    classLines.append("};")
    return "\n".join(classLines)

def GenerateHeaderCode(targetClass: ClassName, allParsedClassFuncs: tuple[list[ParsedFunction], list[ParsedFunction]]) -> str:
    """
    Generate a C++ header file for the target class.
    Organizes methods with vtable order first, then remaining methods.
    Also handles dependencies by generating classes for missing types.
    """
    # Generate the class definition
    classDefinition: str = GenerateClassDefinition(targetClass, allParsedClassFuncs)

    # Extract custom types used by this class
    custom_types = ExtractCustomTypesFromFuncs(targetClass, allParsedClassFuncs)
    missing_types = {
        (decl, full_name) 
        for (decl, full_name) in custom_types 
        if not search_for_class_definition(PROJECT_FOLDER, full_name)
    }
    
    # # Process missing classes - generate definitions for those with virtual/exported functions
    # dependent_class_defs, types_to_forward_declare = ProcessMissingTypes(missing_types)
    
    # # Generate forward declarations for remaining types
    # forward_decls = build_forward_declarations(types_to_forward_declare)

    # Combine all parts of the header
    header_parts = ["#pragma once"]
    
    # Add dependent class definitions
    # if dependent_class_defs:
    #     for classDefinition in dependent_class_defs:
    #         # Remove the #pragma once from dependent class definitions
    #         clean_def = classDefinition.replace("#pragma once", "").strip()
    #         if clean_def:
    #             header_parts.append("\n" + clean_def)
    
    # # Add forward declarations
    # if forward_decls:
    #     header_parts.append("\n" + forward_decls + "\n")
        
    # Add the main class definition
    header_parts.append("\n" + classDefinition)
    
    return "".join(header_parts)

# -----------------------------------------------------------------------------
# Main functionality
# -----------------------------------------------------------------------------

def GetAllParsedClassFuncs(targetClass: ClassName) -> tuple[list[ParsedFunction], list[ParsedFunction]]:
    parsedVTableClassFuncs: list[ParsedFunction] = GetParsedVTableFuncs(targetClass)
    if not parsedVTableClassFuncs:
        PrintMsg(f"No matching VTable function signatures were found for {targetClass.fullName}.\n")

    parsedClassFuncs: list[ParsedFunction] = GetParsedFuncs(targetClass)
    if not parsedClassFuncs:
        PrintMsg(f"No matching function signatures were found for {targetClass.fullName}.\n")
    # Get non-vtable methods
    finalParsedClassFuncs: list[ParsedFunction] = [
        parsedFunc for parsedFunc in parsedClassFuncs
        if parsedFunc not in parsedVTableClassFuncs
    ]
    
    return (parsedVTableClassFuncs, finalParsedClassFuncs)

def WriteHeaderToFile(targetClass: ClassName, headerCode: str) -> bool:
    if targetClass.namespaces:
        # Create folder structure for namespaces
        classFolderPath: str = os.path.join(*targetClass.namespaces)
        outputFolderPath: str = os.path.join(OUTPUT_FOLDER, classFolderPath)
        
        # Create directory if it doesn't exist
        os.makedirs(outputFolderPath, exist_ok=True)
        
        # Output file path is inside the namespace folder
        outputFilePath: str = os.path.join(classFolderPath, f"{targetClass.name}.h")
    else:
        # No namespace, just save in current directory
        outputFilePath: str = f"{targetClass.name}.h"
    
    outputFilePath: str = os.path.join(OUTPUT_FOLDER, outputFilePath)

    try:
        with open(outputFilePath, 'w') as headerFile:
            headerFile.write(headerCode)
        PrintMsg(f"Header file '{outputFilePath}' created successfully.\n")
        
        return True
    except Exception as e:
        PrintMsg(f"Error writing header file '{outputFilePath}': {e}\n")
        return False

def ExportClassHeader(targetClass: ClassName) -> bool:
    """
    Generate and save a C++ header file for the target class.
    For namespaced classes, creates appropriate folder structure.
    """
    global processedClasses
    processedClasses = set()
    # Add the target class to processed classes to prevent recursion
    processedClasses.add(targetClass)

    allParsedClassFuncs: tuple[list[ParsedFunction], list[ParsedFunction]] = GetAllParsedClassFuncs(targetClass)

    headerCode: str = GenerateHeaderCode(targetClass, allParsedClassFuncs)
    return WriteHeaderToFile(targetClass, headerCode)

def Main():
    """Main entry point for the script."""
    # Ask user for target class
    #targetClass = ida_kernwin.ask_str("IModelObject", 0, "Enter target class name (supports namespaces and templates):")
    targetClassName: str = "IModelObject"
    if not targetClassName:
        PrintMsg("No target class specified. Aborting.\n")
        return
    breakpoint()
    targetClass: ClassName = ClassName(targetClassName)

    ExportClassHeader(targetClass)

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
        PrintMsg(f"Initializing \"{self.wanted_name}\" Plugin\n")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        PrintMsg(f"Running \"{self.wanted_name}\" Plugin\n")
        Main()

    def term(self):
        PrintMsg(f"Terminating \"{self.wanted_name}\" Plugin\n")
        pass

def PLUGIN_ENTRY():
    idaapi.msg(f"Creating \"Export Class to C++ Header\" Plugin entry\n")
    return ExportClassToCPPH()

if __name__ == "__main__":
    Main()