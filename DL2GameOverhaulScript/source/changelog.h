#pragma once
#include <map>
#include <string>

namespace Changelog {
	std::map<std::string_view, std::string> changelogs = {
		{ "v1.1.0",
R"(- You can now load custom mod files from "EGameTools\UserModFiles"! Please read the new "Welcome" screen which explains how to use this feature and how to use the rest of the mod menu
- By using the directory mentioned earlier for mod files, you can reload most of them by just reloading the savegame!

- Added "Reload Jump Params", using the directory mentioned earlier (Player)
- Added "One-handed Mode" (Player)
- Added "Nightrunner Mode" (Player)
- Added "Disable Game Pause While AFK" (Misc)
- Added "Freeze Time" (World)
- Added "Game Speed" slider (World)
- Added "Slow Motion" (World)

- Fixed having a weird offset of the entire map view when FreeCam is enabled
- Fixed player dying from switching FreeCam off after flying to high altitudes/through walls with “Teleport Player to Camera” option)" }
	};
}