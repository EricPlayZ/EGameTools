#pragma once
namespace Config {
	extern const char playerVars[];

	extern void SaveConfig();
	extern void InitConfig();
	extern void ConfigLoop();
	extern void ConfigSaveLoop();
}