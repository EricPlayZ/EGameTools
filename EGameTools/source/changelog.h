#pragma once
#include <map>
#include <string>

namespace Changelog {
	std::map<std::string_view, std::string> changelogs = {
		{ "v1.1.0",
R"(- You can now load custom mod files from "EGameTools\UserModFiles"! Please read the new "Welcome" screen which explains how to use this feature and how to use the rest of the mod menu
- By using the directory mentioned earlier for mod files, you can reload most of them by just reloading the savegame!
- Mod menu UI/UX revamp

- Added "Reload Jump Params", using the directory mentioned earlier (Player)
- Added "One-handed Mode" (Player)
- Added "Nightrunner Mode", default button is "F9" (Player)
- Added "Disable Game Pause While AFK" (Misc)
- Added "Freeze Time" (World)
- Added "Game Speed" slider (World)
- Added "Slow Motion", default button is "4" (World)
- Added a "Debug" menu tab (mainly used for troubleshooting)
- Added logging to file (mainly used for troubleshooting)

- Changed "Menu Transparency" to "Menu Opacity"
- Fixed having a weird offset of the entire map view when FreeCam is enabled
- Fixed player dying from switching FreeCam off after flying to high altitudes/through walls with "Teleport Player to Camera" option
- Fixed FOV slider not changing FOV while using FreeCam

That's it for this update! The next few updates will include some more bug fixes rather than new, big features, so stay tuned!)" },
		{ "v1.1.1",
		R"(- Fixed frequent crashes when using DX12, sometimes DX11 too.
- Fixed frequent crashing at game startup
- Fixed crashing when trying to use ".model" mods with the custom file loading system; PLEASE keep in mind that I haven't found a fix for this yet! Custom ".model" files will not get loaded from "UserModFiles"
- Fixed Slow Motion not changing back to the original game speed when deactivated
- Fixed certain features like Use TPP Model or Player Variables not working at all for some users)" }
	};
}