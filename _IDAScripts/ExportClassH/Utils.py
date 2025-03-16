import re
from functools import lru_cache
from typing import Tuple
import ida_nalt
import ida_bytes
import idaapi
import idautils
import idc

IDA_NALT_ENCODING = ida_nalt.get_default_encoding_idx(ida_nalt.BPU_1B)

def FixTypeSpacing(type: str) -> str:
    """Fix spacing for pointers/references, commas, and angle brackets."""
    type = re.sub(r'\s+([*&])', r'\1', type)             # Remove space before '*' or '&'
    type = re.sub(r'([*&])(?![\s*&])', r'\1 ', type)     # Ensure '*' or '&' is followed by one space if it's not already.
    type = re.sub(r'\s*,\s*', ', ', type)                # Ensure comma followed by one space
    type = re.sub(r'<\s+', '<', type)                    # Remove space after '<'
    type = re.sub(r'\s+>', '>', type)                    # Remove space before '>'
    type = re.sub(r'\s+([\),])', r'\1', type)
    type = re.sub(r'\s+', ' ', type)                     # Collapse multiple spaces
    return type.strip()

def CleanDoubleSpaces(str: str) -> str:
    return " ".join(str.split())

def CleanEndOfClassStr(clsStr: str) -> str:
    clsStr = clsStr.removesuffix("const")
    while clsStr and clsStr[-1] in {')', ',', '&', '*'}:
        clsStr = clsStr[:-1]
    clsStr = clsStr.removesuffix("const")
    return clsStr

def CleanType(type: str) -> str:
    """Remove unwanted tokens from a type string, then fix spacing."""
    type = re.sub(r'\b(__cdecl|__fastcall|__ptr64)\b', '', type)
    return FixTypeSpacing(type)

def ReplaceIDATypes(type: str) -> str:
    """Replace IDA types with normal ones"""
    return type.replace("unsigned __int64", "uint64_t").replace("_QWORD", "uint64_t").replace("__int64", "int64_t").replace("unsigned int", "uint32_t")

@lru_cache(maxsize=None)
def ExtractTypeTokensFromString(types: str) -> list[str]:
    """Extract potential type names from a string, properly handling template types."""
    if not types:
        return []
    
    types = FixTypeSpacing(types)
    result: list[str] = []
    currentWord = ""
    templateDepth = 0
    
    for char in types:
        if char == '<':
            templateDepth += 1
            currentWord += char
        elif char == '>':
            templateDepth -= 1
            currentWord += char
        elif char.isspace() and templateDepth == 0:
            # Only split on spaces outside of templates
            if currentWord:
                result.append(currentWord)
                currentWord = ""
        else:
            currentWord += char
    
    # Add the last word if there is one
    if currentWord:
        result.append(currentWord)
    
    # Filter out empty strings
    return [word.strip() for word in result if word]

@lru_cache(maxsize=None)
def SplitByCommaOutsideTemplates(params: str) -> list[str]:
    parts = []
    current = []
    depth = 0
    i = 0

    while i < len(params):
        if params[i] == '<':
            depth += 1
        elif params[i] == '>':
            # It's good to check for consistency:
            if depth > 0:
                depth -= 1
                
        # If we see a , at top level, split here.
        if params[i] == ',' and depth == 0:
            parts.append(''.join(current).strip())
            current = []
            i += 1
        else:
            current.append(params[i])
            i += 1

    # Append any remaining characters as the last parameter.
    if current:
        parts.append(''.join(current).strip())
    return parts

@lru_cache(maxsize=None)
def SplitByClassSeparatorOutsideTemplates(params: str) -> list[str]:
    parts = []
    current = []
    depth = 0
    i = 0

    while i < len(params):
        if params[i] == '<':
            depth += 1
        elif params[i] == '>':
            # It's good to check for consistency:
            if depth > 0:
                depth -= 1

        # If we see a :: at top level, split here.
        if params[i] == ':' and params[i + 1] == ":" and depth == 0:
            parts.append(''.join(current).strip())
            current = []
            i += 2
        else:
            current.append(params[i])
            i += 1

    # Append any remaining characters as the last parameter.
    if current:
        parts.append(''.join(current).strip())
    return parts

@lru_cache(maxsize=None)
def FindLastSpaceOutsideTemplates(s: str) -> int:
    """Return the index of the last space in s that is not inside '<' and '>'."""
    depth = 0
    for i in range(len(s) - 1, -1, -1):
        ch = s[i]
        if ch == '>':
            depth += 1
        elif ch == '<':
            depth -= 1
        elif depth == 0 and ch == ' ':
            return i
    return -1

@lru_cache(maxsize=None)
def FindLastClassSeparatorOutsideTemplates(s: str) -> int:
    """Return the index of the last occurrence of "::" in s that is not inside '<' and '>'."""
    depth = 0
    # iterate backwards, but check for two-character substring
    for i in range(len(s) - 1, -1, -1):
        if s[i] == '>':
            depth += 1
        elif s[i] == '<':
            depth -= 1
        # Only if we're not inside a template.
        if depth == 0 and i > 0 and s[i-1:i+1] == "::":
            return i - 1  # return the index of the first colon
    return -1

# -----------------------------------------------------------------------------
# IDA util functions
# -----------------------------------------------------------------------------

def DemangleSig(sig: str) -> str:
    return idaapi.demangle_name(sig, idaapi.MNG_LONG_FORM)

def GetMangledTypePrefix(namespaces: tuple[str], className: str) -> str:
    """
    Get the appropriate mangled type prefix for a class name.
    For class "X" this would be ".?AVX@@"
    For class "NS::X" this would be ".?AVX@NS@@"
    For templated classes, best to use get_mangled_name_for_template instead.
    """
    if not namespaces:
        return f".?AV{className}@@"
    
    # For namespaced classes, the format is .?AVClassName@Namespace@@
    # For nested namespaces, they are separated with @ in reverse order
    mangledNamespaces = "@".join(reversed(namespaces))
    return f".?AV{className}@{mangledNamespaces}@@"

# -----------------------------------------------------------------------------
# IDA pattern search utilities
# -----------------------------------------------------------------------------

@lru_cache(maxsize=None)
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