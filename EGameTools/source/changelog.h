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
		R"(- Added a message box that warns the user of shortcut creation failure

- Fixed frequent crashes when using DX12, sometimes DX11 too.
- Fixed frequent crashing at game startup
- Fixed crashing when trying to use ".model" mods with the custom file loading system; PLEASE keep in mind that I haven't found a fix for this yet! Custom ".model" files will not get loaded from "UserModFiles"
- Fixed Slow Motion not changing back to the original game speed when deactivated
- (Hopefully) fixed certain features like Use TPP Model or Player Variables not working at all for some users (if it still happens, please let me know!))" },
		{ "v1.1.2",
		R"(- Added compatibility with v1.15.2 "Firearms" update
- If you get a message pop-up, or an error in the console window about a problem creating a shortcut, please open Windows Settings and, for Windows 11, go to System -> For developers and enable "Developer Mode", or for Windows 10, go to Update & Security -> For developers and enable "Developer Mode"; after doing this, restart the game and there should be no issues with shortcut creation anymore (blame Microsoft, I have no idea why you have to enable this setting just to create some shortcuts!)

Bug fixes for various features will come in later updates, please be patient!

Sorry for the long wait! I haven't had enough time on my hands to update the mod, and I had some issues with how I would be handling updates from now on. I wanted a way to make the latest mod version support older game versions too.
So e.g. 5 months from now, I'd want the latest mod version to support the latest game version (e.g. v1.19.1) AND support every other older version, down to v1.14.x, this way people could use the latest features of this mod, with older game versions.
Unfortunately, I haven't found a way to do this yet, so I will be setting this aside for some other time. For now, I'll release this compatibility patch!

You should also expect less updates in the future, as I'm going to be pretty busy this year, with my final exam, driving school, and so many other things!
Thank you everyone for the support <3)" },
		{ "v1.1.3",
		R"(- Added compatibility with v1.16.0 "Nightmare Mode" update
- Fixed really fast player speed when executing a slow motion kill, dodging or ground pounding at 1.00x game speed (will still happen at any other game speed unfortunately)
- Added function hooking timeout for easier debugging
- Fetch game version using Windows' API instead of using the game's function)" },
		{ "v1.2.0",
		R"(- Added compatibility with v1.16.1 hotfix update
- Added "Player Immunity" slider (Player)
- Added "Unlimited Immunity" (Player)
- Added "Invisible to Enemies" (Player)
- Added "Increase Data PAKs Limit" (Misc; requires game restart to apply) - you can now add more than 8 data PAKs, e.g. data8.pak, data9.pak, data10.pak, etc, up to 200 PAKs in total
- Added "Disable Data PAKs CRC Check" (Misc; requires game restart to apply) - stops the game from detecting data PAKs, which allows you to use data PAK mods in multiplayer as well
- Added "Disable Savegame CRC Check" (Misc; requires game restart to apply) - stops the game from falsely saying your savegame is corrupt whenever you modify it

- Fixed "God Mode" (Player) staying enabled after toggling FreeCam off
- Fixed player variables saving and loading using old version of player_variables.scr (which made Max Health drop to negative infinite)
- Fixed long paths to mods inside UserModFiles causing a game crash or causing the mods to not load at all
- Fixed "God Mode" (Player) not working properly or at all in multiplayer
- Fixed volatiles still being able to kill you when they jump on top of you while "God Mode" (Player) is enabled
- Fixed "Disable Out of Bounds Timer" (Player) not working in missions
- Changed the config system to only write to the config file whenever there's a change in the mod menu)" }
	};
}