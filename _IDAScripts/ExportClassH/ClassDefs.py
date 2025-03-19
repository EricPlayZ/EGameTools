from typing import Optional, List, Dict, get_type_hints
from prodict import Prodict
    
class ParsedParam(Prodict):
    type: str
    name: str
    parsedClassParam: Optional["ParsedClass"]

    def init(self):
        self.type = ""
        self.name = ""
        self.parsedClassParam = None

class ParsedClass(Prodict):
    type: str
    parentNamespaces: List[str]
    parentClasses: List[str]
    name: str
    templateParams: List["ParsedParam"]
    fullClassName: str
    childClasses: Prodict
    functions: List["ParsedFunction"]

    def init(self):
        self.type = ""
        self.parentNamespaces = []
        self.parentClasses = []
        self.name = ""
        self.templateParams = []
        self.fullClassName = ""
        self.childClasses = Prodict()
        self.functions = []

class ParsedFunction(Prodict):
    type: str
    funcType: str
    access: str
    returnTypes: List[ParsedParam]
    parentNamespaces: List[str]
    parentClasses: List[str]
    fullClassName: str
    funcName: str
    params: List[ParsedParam]
    const: bool
    fullFuncSig: str

    def init(self):
        self.type = "function"
        self.funcType = "function"
        self.access = "public"
        self.returnTypes = []
        self.parentNamespaces = []
        self.parentClasses = []
        self.fullClassName = ""
        self.funcName = ""
        self.params = []
        self.const = False
        self.fullFuncSig = ""
    
ParsedParam.__annotations__ = get_type_hints(ParsedParam)
ParsedParam.__annotations__["parsedClassParam"] = ParsedClass
ParsedClass.__annotations__ = get_type_hints(ParsedClass)
ParsedFunction.__annotations__ = get_type_hints(ParsedFunction)