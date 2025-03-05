import os

INTERNAL_SCRIPT_NAME = "ExportClassToCPPH"
PROJECT_PATH = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\include"
OUTPUT_PATH = r"D:\PROJECTS\Visual Studio\EGameSDK\EGameSDK\proxies\engine_x64_rwdi\scripts"
HEADER_OUTPUT_PATH = os.path.join(OUTPUT_PATH, "generated")
CACHE_OUTPUT_PATH = os.path.join(OUTPUT_PATH, "cache")
PARSED_VARS_CACHE_FILENAME = os.path.join(CACHE_OUTPUT_PATH, "parsedClassVarsByClass.cache")
PARSED_FUNCS_CACHE_FILENAME = os.path.join(CACHE_OUTPUT_PATH, "parsedFuncsByClass.cache")