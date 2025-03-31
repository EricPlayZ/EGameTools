from __future__ import annotations
from typing import Optional, List
from pydantic import BaseModel, SkipValidation

def DefaultPydanticSerializer(obj):
    if hasattr(obj, "model_dump"):
        return obj.model_dump(exclude_none=True)
    raise TypeError(f"Object of type {obj.__class__.__name__} is not JSON serializable")

class ParsedParam(BaseModel):
    type: str = ""
    name: str = ""
    parsedClassParam: Optional[SkipValidation[ParsedClass]] = None

class ParsedClass(BaseModel):
    type: str = ""
    parentNamespaces: List[str] = []
    parentClasses: List[str] = []
    classDependencies: List[str] = []
    name: str = ""
    templateParams: List[ParsedParam] = []
    fullClassName: str = ""
    childClasses: dict = {}
    classVars: List[ParsedClassVar] = []
    virtualFunctions: List[ParsedFunction] = []
    functions: List[ParsedFunction] = []

class ParsedFunction(BaseModel):
    type: str = "function"
    funcType: str = "function"
    access: str = "public"
    returnTypes: List[ParsedParam] = []
    parentNamespaces: List[str] = []
    parentClasses: List[str] = []
    fullClassName: str = ""
    funcName: str = ""
    params: List[ParsedParam] = []
    const: bool = False
    fullFuncSig: str = ""

class ParsedClassVar(BaseModel):
    type: str = "classVar"
    access: str = "public"
    varTypes: List[ParsedParam] = []
    parentNamespaces: List[str] = []
    parentClasses: List[str] = []
    fullClassName: str = ""
    varName: str = ""
    fullClassVarSig: str = ""

ParsedParam.model_rebuild()
ParsedClass.model_rebuild()
ParsedFunction.model_rebuild()
ParsedClassVar.model_rebuild()