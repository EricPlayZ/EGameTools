import re
from functools import cache

_RE_BEFORE_POINTER = re.compile(r'\s+([*&])')
_RE_AFTER_POINTER = re.compile(r'([*&])(?![\s*&])')
_RE_COMMA = re.compile(r'\s*,\s*')
_RE_AFTER_ANGLE = re.compile(r'<\s+')
_RE_BEFORE_ANGLE = re.compile(r'\s+>')
_RE_BEFORE_PUNC = re.compile(r'\s+([\),])')
_RE_SPACES = re.compile(r'\s+')

@cache
def FixTypeSpacing(type_str: str) -> str:
    type_str = _RE_BEFORE_POINTER.sub(r'\1', type_str)
    type_str = _RE_AFTER_POINTER.sub(r'\1 ', type_str)
    type_str = _RE_COMMA.sub(', ', type_str)
    type_str = _RE_AFTER_ANGLE.sub('<', type_str)
    type_str = _RE_BEFORE_ANGLE.sub('>', type_str)
    type_str = _RE_BEFORE_PUNC.sub(r'\1', type_str)
    type_str = _RE_SPACES.sub(' ', type_str)
    return type_str.strip()

@cache
def CleanDoubleSpaces(str: str) -> str:
    return " ".join(str.split())

@cache
def CleanEndOfClassStr(clsStr: str) -> str:
    clsStr = clsStr.removesuffix("const")
    while clsStr and clsStr[-1] in {')', ',', '&', '*'}:
        clsStr = clsStr[:-1]
    clsStr = clsStr.removesuffix("const")
    return clsStr

@cache
def CleanType(type: str) -> str:
    """Remove unwanted tokens from a type string, then fix spacing."""
    type = re.sub(r'\b(__cdecl|__fastcall|__ptr64)\b', '', type)
    return FixTypeSpacing(type)

@cache
def ReplaceIDATypes(type: str) -> str:
    """Replace IDA types with normal ones"""
    return type.replace("unsigned __int64", "uint64_t").replace("_QWORD", "uint64_t").replace("__int64", "int64_t").replace("unsigned int", "uint32_t")

@cache
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
            if templateDepth > 0:
                templateDepth -= 1
            currentWord += char
        elif char == ' ' and templateDepth == 0:
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

@cache
def ExtractParamNames(params: str) -> str:
    paramsList: list[str] = [param.strip() for param in params.split(',') if param.strip()]
    if len(paramsList) == 1 and paramsList[0] == "void":
        return ""
    
    paramNames: list[str] = [param.split(" ")[-1].strip() for param in paramsList]
    newParams: str = ", ".join(paramNames)
    return newParams

@cache
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

@cache
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

@cache
def FindLastSpaceOutsideTemplates(s: str) -> int:
    """Return the index of the last space in s that is not inside '<' and '>'."""
    depth = 0
    for i in range(len(s) - 1, -1, -1):
        ch = s[i]
        if ch == '>':
            depth += 1
        elif ch == '<':
            if depth > 0:
                depth -= 1
        elif depth == 0 and ch == ' ':
            return i
    return -1

@cache
def FindLastClassSeparatorOutsideTemplates(s: str) -> int:
    """Return the index of the last occurrence of "::" in s that is not inside '<' and '>'."""
    depth = 0
    # iterate backwards, but check for two-character substring
    for i in range(len(s) - 1, -1, -1):
        if s[i] == '>':
            depth += 1
        elif s[i] == '<':
            if depth > 0:
                depth -= 1
        # Only if we're not inside a template.
        if depth == 0 and i > 0 and s[i-1:i+1] == "::":
            return i - 1  # return the index of the first colon
    return -1