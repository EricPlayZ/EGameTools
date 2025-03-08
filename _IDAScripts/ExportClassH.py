import idaapi
from ExportClassH.Main import Main

class ExportClassH_Plugin(idaapi.plugin_t):
    """IDA Pro plugin for exporting C++ class definitions to header files."""
    flags = idaapi.PLUGIN_UNL
    comment = "Extract exported and virtual functions and generate a C++ header"
    help = "Extracts exported and virtual functions and generates a C++ header"
    wanted_name = "Export Class to C++ Header"
    wanted_hotkey = "Shift-F"
    
    def init(self):
        print(f"Initializing \"{self.wanted_name}\" Plugin")
        return idaapi.PLUGIN_OK

    def run(self, arg):
        print(f"Running \"{self.wanted_name}\" Plugin")
        Main()

    def term(self):
        print(f"Terminating \"{self.wanted_name}\" Plugin")
        pass

def PLUGIN_ENTRY():
    print(f"Creating \"Export Class to C++ Header\" Plugin entry")
    return ExportClassH_Plugin()

if __name__ == "__main__":
    Main()