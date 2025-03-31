import os

INTERNAL_SCRIPT_NAME = "ExportClassH"

PROJECT_INCLUDES_PATH = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"
OUTPUT_PATH = r"D:\PROJECTS\Visual Studio\EGameSDK\_IDAScripts"
HEADER_OUTPUT_PATH = os.path.join(OUTPUT_PATH, "generated")
INPUT_MD5 = bytes()
LAST_CLICKED_RADIO = 0

DEFAULT_CONFIG = {
    "PROJECT_INCLUDES_PATH": PROJECT_INCLUDES_PATH,
    "OUTPUT_PATH": OUTPUT_PATH,
    "LAST_CLICKED_RADIO": LAST_CLICKED_RADIO
}
CONFIG_FILE = os.path.join(os.path.join(os.path.dirname(__file__), os.pardir), "ExportClassH.json")
PARSED_CLASSES_OUTPUT_FILE = os.path.join(HEADER_OUTPUT_PATH, "parsed-classes.json")