import importlib

from ExportClassH import UI

def Main():
    """Main entry point for the script."""
    UI.OpenMainDlg()
    
    # Reload modules to apply any changes
    from ExportClassH import Utils, Config, ClassDefs, RTTIAnalyzer, ClassParser, HeaderGen, ProjectManager
    importlib.reload(Config)
    importlib.reload(Utils)
    importlib.reload(ClassDefs)
    importlib.reload(RTTIAnalyzer)
    importlib.reload(ClassParser)
    importlib.reload(HeaderGen)
    importlib.reload(ProjectManager)
    importlib.reload(UI)