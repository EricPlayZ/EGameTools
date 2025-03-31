import os
import json
import ida_kernwin
import idc
import cProfile

from ExportClassH import Config, HeaderGen

def SetConfigVars(settings):
    Config.PROJECT_INCLUDES_PATH = settings["PROJECT_INCLUDES_PATH"]
    Config.OUTPUT_PATH = settings["OUTPUT_PATH"]
    Config.LAST_CLICKED_RADIO = settings["LAST_CLICKED_RADIO"]

    Config.HEADER_OUTPUT_PATH = os.path.join(Config.OUTPUT_PATH, "generated")

# Load settings from file
def LoadConfig() -> dict:
    Config.INPUT_MD5 = idc.retrieve_input_file_md5()
    if not Config.INPUT_MD5:
        raise Exception("Failed retreiving input file MD5")
    if os.path.exists(Config.CONFIG_FILE):
        with open(Config.CONFIG_FILE, "r") as f:
            loadedJson = json.load(f)
            SetConfigVars(loadedJson)
            return loadedJson
    return Config.DEFAULT_CONFIG

# Save settings to file
def SaveConfig(settings = {}):
    if not settings:
        settings = {
            "PROJECT_INCLUDES_PATH": Config.PROJECT_INCLUDES_PATH,
            "OUTPUT_PATH": Config.OUTPUT_PATH,
            "LAST_CLICKED_RADIO": Config.LAST_CLICKED_RADIO
        }

    with open(Config.CONFIG_FILE, "w") as f:
        json.dump(settings, f, indent=4)
    
    SetConfigVars(settings)

class SettingsDialog(ida_kernwin.Form):
    def __init__(self, current_config):
        self.config = current_config
        ida_kernwin.Form.__init__(self, r"""STARTITEM 0
BUTTON YES* Save
BUTTON CANCEL Cancel
Settings

Modify the paths below:

<##Project Includes Path:{i_project_path}>
<##Output Path:{i_output_path}>
""", {
            'i_project_path': ida_kernwin.Form.StringInput(swidth=50, value=self.config["PROJECT_INCLUDES_PATH"]),
            'i_output_path': ida_kernwin.Form.StringInput(swidth=50, value=self.config["OUTPUT_PATH"]),
        })

    def GetValues(self):
        return {
            "PROJECT_INCLUDES_PATH": self.i_project_path.value,
            "OUTPUT_PATH": self.i_output_path.value,
            "LAST_CLICKED_RADIO": Config.LAST_CLICKED_RADIO
        }
    
    def OnFormChange(self, fid):
        return 1  # Required for form functionality

class MainDialog(ida_kernwin.Form):
    def __init__(self):
        ida_kernwin.Form.__init__(self, r"""STARTITEM 0
BUTTON YES* OK
BUTTON CANCEL Cancel
Export Class to C++ Header

{FormChangeCb}

<##Update Project:{r_update}>
<##Generate Classes:{r_classesGenerate}>
<##Generate Class:{r_classGenerate}>
<##Settings:{r_settings}>{radioGroup}>
""", {
            'radioGroup': ida_kernwin.Form.RadGroupControl(("r_update", "r_classesGenerate", "r_classGenerate", "r_settings")),
            'FormChangeCb': ida_kernwin.Form.FormChangeCb(self.OnFormChange),
        })

    def OnFormChange(self, fid):
        return 1  # Required for form functionality

def OpenSettingsDlg():
    """Opens the Settings Dialog and returns to Main Dialog on Cancel"""
    settings = LoadConfig()
    settingsDlg = SettingsDialog(settings)
    settingsDlg.Compile()
    result = settingsDlg.Execute()

    if result == 1:  # Save clicked
        newSettings = settingsDlg.GetValues()
        SaveConfig(newSettings)  # Save new settings to file
        print("[INFO] Settings saved successfully!")

    settingsDlg.Free()
    
    # Always return to the main dialog after closing settings
    OpenMainDlg()

def OpenMainDlg():
    """Reopens the Main Dialog"""
    LoadConfig()

    mainDlg = MainDialog()
    mainDlg.Compile()
    mainDlg.radioGroup.value = Config.LAST_CLICKED_RADIO
    result = mainDlg.Execute()

    if result == 1:  # OK clicked
        selectedOption = mainDlg.radioGroup.value
        Config.LAST_CLICKED_RADIO = selectedOption
        SaveConfig()

        if selectedOption == 0:
            print("[INFO] Update Project selected!")
            #ProjectManager.ProcessExistingHeaders()
        elif selectedOption == 1:
            print("[INFO] Generate Classes selected!")
            cProfile.runctx('HeaderGen.ExportClassHeaders()', globals(), locals(), 'cProfiler-data.dat')
        elif selectedOption == 2:
            print("[INFO] Generate Class selected!")
            targetClass = ida_kernwin.ask_str("", 0, "Enter target class name:")
            if not targetClass:
                print("No target class specified. Aborting.")
                return
            cProfile.runctx('HeaderGen.ExportClassHeader(targetClass)', globals(), locals(), 'cProfiler-data.dat')
        elif selectedOption == 3:
            print("[INFO] Settings selected!")
            OpenSettingsDlg()  # Open settings when selected
    else:
        print("[INFO] User clicked Cancel. No action taken.")

    mainDlg.Free()