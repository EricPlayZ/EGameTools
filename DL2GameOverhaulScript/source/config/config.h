#pragma once
#include <windows.h>

namespace Config {
	extern const char playerVars[];
	
	enum ValueType {
		OPTION,
		Float,
		String
	};

	extern void SaveConfig();
	extern void InitConfig();
	extern void ConfigLoop();
	extern void ConfigSaveLoop();
}