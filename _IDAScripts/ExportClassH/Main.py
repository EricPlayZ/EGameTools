import importlib

from ExportClassH import UI

def Main():
    """Main entry point for the script."""
    UI.OpenMainDlg()
    
    # Reload modules to apply any changes
    from ExportClassH import Config, Utils, IDAUtils, ClassDefs, JSONGen#, RTTIAnalyzer, ClassParser, HeaderGen, ProjectManager
    importlib.reload(Config)
    importlib.reload(Utils)
    importlib.reload(IDAUtils)
    importlib.reload(ClassDefs)
    importlib.reload(JSONGen)
    # importlib.reload(RTTIAnalyzer)
    # importlib.reload(ClassParser)
    # importlib.reload(HeaderGen)
    # importlib.reload(ProjectManager)
    importlib.reload(UI)