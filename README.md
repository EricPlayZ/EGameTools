<p align="center">
<img src="https://i.imgur.com/s6ZV1cz.png"/><br>
<b>EGameTools (short for Eric's Game Tools) is a mod menu developed in C++ which aims to add additional features such as FreeCam, Third Person, Weather and Time control, etc. which the game does not provide by default and also to help ease mod developers in the process of creating mods.</b><br><br>
<b>Menu Preview:</b><br>
<img src="https://i.imgur.com/rQpPtT3.png"/><br>
<img src="https://i.imgur.com/M9VP0cv.png"/><br>
<b>PLAYER</b><br>
- Player Health slider<br>
- Player Immunity slider<br>
- Old World Money slider<br>
- God Mode<br>
- Freeze Player<br>
- Unlimited Immunity<br>
- Unlimited Stamina<br>
- Unlimited Items<br>
- One-Hit Kill<br>
- Invisible to Enemies<br>
- Disable Out of Bounds Timer<br>
- Nightrunner Mode<br>
- One-handed Mode<br>
- Allow Grapple Hook in Safezone<br>
- Disable Air Control<br>
- Reload Jump Parameters<br>
- Player Variables editing (with the ability to save and load to/from a <b>`player_variables.scr`</b>, restore to default values)<br><br>
<b>WEAPON</b><br>
- Weapon Durability slider (for equipped weapon)<br>
- Unlimited Durability<br>
- Unlimited Ammo<br>
- No Spread<br>
- No Recoil<br>
- Instant Reload<br><br>
<b>CAMERA</b><br>
- FreeCam<br>
- Teleport Player to Camera<br>
- Third Person<br>
- FOV slider<br>
- Lens Distortion slider<br>
- GoPro Mode<br>
- Disable PhotoMode Limits<br>
- Disable Safezone FOV Reduction<br><br>
<b>TELEPORT</b><br>
- Saved Locations list, with the ability of saving, deleting and teleporting to said locations<br>
- Teleport to Coords with X, Y, Z inputs<br>
- Teleport to Waypoint<br><br>
<b>MISC</b><br>
- Disable Game Pause While AFK<br>
- Disable HUD<br>
- Disable Savegame CRC Check<br>
- Disable Data PAKs CRC Check<br>
- Increase Data PAKs Limit<br><br>
<b>WORLD</b><br>
- Time slider<br>
- Game Speed slider<br>
- Freeze Time<br>
- Slow Motion<br>
- Weather control<br><br>
<b>Check <a href="https://trello.com/b/oRaJQEOi/egametools-dying-light-2">EGameTools' Trello﻿﻿</a> page to see what features or bug fixes I'm working on/planning to add!</b><br>
<img src="https://i.imgur.com/kOyrZYC.png"/><br>
<b>STEP 1: </b>Download the archive from the <b><a href="https://github.com/EricPlayZ/EGameTools/releases">Releases</a></b> section<br>
<b>STEP 2: </b>Extract the files from inside the archive to the game's exe folder (<b>`Dying Light 2\ph\work\bin\x64`</b>).<br><br>
To uninstall, delete <b>`winmm.dll`</b>, <b>`EGameTools.asi`</b> and <b>`EGameTools.ini`</b> from the game's exe folder (<b>`Dying Light 2\ph\work\bin\x64`</b>).<br>
<img src="https://i.imgur.com/4mlvwfv.gif"/><br>
<img src="https://i.imgur.com/CRAQXjm.png"/><br>
﻿﻿When you launch the game, a console window will appear. When it does, <b>DON'T CLOSE IT!</b> Closing it will also close the game.<br><br>
<b>MENU TOGGLE</b><br>
The default key for opening/closing the menu is <b>F5</b>. You can use your mouse to navigate the menu.<br>
To change it, you can open up the menu and change the hotkey by clicking the hotkey button for <b>`Menu Toggle Key`</b> and then pressing a key on your keyboard.<br><br>
<b>FREECAM</b><br>
While using FreeCam, you can press <b>Shift</b> or <b>Alt</b> to boost your speed or slow you down respectively.<br>
You can also use the <b>scroll wheel</b> to change FreeCam speed while FreeCam is enabled.<br><br>
<b>MENU SLIDERS</b><br>
To manually change the value of a slider option, <b>hold CTRL</b> while clicking the slider.<br>
This will let you input a value manually into the slider, which can also go beyond the option's slider limit, given I allow the option to do so.<br><br>
<b>CUSTOM FILE LOADING</b><br>
<b>Example of mod installation:</b><br>
<img src="https://i.imgur.com/bXgW0xE.gif"/><br>
The mod always creates a folder <b>`EGameTools\UserModFiles`</b> inside the same folder as the game executable (exe) or in the same folder as the mod file.<br>
This folder is used for custom file loading. It can load <b>.PAKs</b> and it can also load files extracted from these <b>.PAKs</b>. The latter has only been tested with a few mods that change some <b>.scr</b> files, <b>.gpufx</b> files, and other files included inside <b>.PAK</b> game archives, or files such as <b>.rpack</b> files.<br>
If you put <b>.PAKs</b> inside this folder, they can be named whatever. If you put any other kind of files, they must have the same names as the ones from the game files, otherwise the game won't know it should load those files. Files in subfolders of the <b>`EGameTools\UserModFiles`</b> folder will automatically be detected, so you can sort all your mods in different folders!<br><br>
The game will reload a lot of the files upon a load of your savegame, so if you want to edit those files and reload them without having to restart the game, just reload your savegame and the game should automatically reload most of those files!<br>
Just make sure that if you add new, additional files while you're in-game, please <b>wait AT LEAST 5 seconds</b> before reloading your savegame, otherwise additional files will not get detected.<br>
Also, if there are multiple files of the same exact name, the game will pick the first instance of that file it finds in the folder.<br><br>
The gist of it is, you now don't have to use <b>dataX.pak</b> mods anymore! Or if you do, you can now name them whatever you want! You can open the <b>.PAK</b> files, extract their files in the <b>`EGameTools\UserModFiles`</b> folder and start the game!<br>
Any files put inside this folder will also bypass any of the game file checks, so you can use these mods in co-op too!<br><br>
<b>NOTE:</b> Any mods that are put inside <b>`EGameTools\UserModFiles`</b> as a regular file (<b>.scr</b> or any other file that is usually present in <b>.PAK</b> mods) and NOT a <b>.PAK</b> file, will make the game ignore the same files that are present in any of the <b>.PAK</b> mods inside <b>`EGameTools\UserModFiles`</b>. I recommend using <b>.PAK</b> for most mods. If you run into issues, try extracting the files inside the <b>.PAK</b> into the folder directly.
<b>FOR MOD DEVELOPERS</b><br>
If you want to make mods for <b>EGameTools</b> to load, please try to use as few folders as you possibly can. For example, your mod should only have one folder, something like <b>`EGameTools\UserModFiles\2019 Weather Mod`</b>.<br>
The reason is, my mod continuously checks for new files in the directory, and many folders can slow down the process, and therefore slow down game loading times. So just keep this in mind!<br><br>
If you want to officially include one of your mods as part of <b>EGameTools</b>, please contact me on <b><a href="https://www.nexusmods.com/dyinglight2/mods/1098">NexusMods</a></b> or on <b>Discord (@EricPlayZ)</b>.<br><br>
<b>GAME VARIABLES RELOADING</b><br>
You can also reload <b>Player Variables</b> from a file specified by you, or reload <b>Jump Parameters</b> from <b>`EGameTools\UserModFiles`</b>.<br><br>
<b>HOTKEYS</b><br>
Most mod menu options are toggleable by a hotkey that you can change by clicking the hotkey button for the respective option and then pressing a key on your keyboard.<br>
To change those hotkeys through the config file, visit the <b><a href="https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes">Virtual-Key Codes</a>﻿</b> page from Microsoft which contains a list of all virtual key codes. Simply write the name of the keycode you want to use for each hotkey and save the config file.<br><br>
<b>CONFIG</b><br>
A config file <b>`EGameTools.ini`</b> is stored in the same folder as the game executable (exe) or in the same folder as the mod file.<br>
The config file stores the mod menu's options and hotkeys.<br><br>
Changes to the mod menu or to the config file are always automatically saved or refreshed respectively.<br>
You <b>DO NOT NEED</b> to restart the game for the changes in the config to be applied!<br>
If you want to regenerate the config file, delete it and it will automatically be regenerated.<br><br>
<b>LOGGING</b><br>
Log files will be stored in the <b>EGameTools</b> folder as <b>`log.x.txt`</b>, <b>x</b> being the number of the previous log file. The most recent log file will be called <b>`log.txt`</b>.<br><br>
<b>--------------------</b><br>
Finally, if you've got any issue, no matter how small, please make sure to report it! I will try my best to fix it. I want this mod to be polished and enjoyable to use!<br>
If you've got any suggestions for how I could improve the mod, in terms of UI/UX, features, among other things, please let me know!<br>
<img src="https://i.imgur.com/m0X9akl.png"/><br>
<b>Check <a href="https://trello.com/b/oRaJQEOi/egametools-dying-light-2">EGameTools' Trello﻿﻿</a> page to see what features or bug fixes I'm working on/planning to add.</b><br><br>
<b>DO NOT</b> use the discussions tab for bug reports.<br>
If there is a bug you're encountering, open a bug report, otherwise please refrain from using the discussions tab unless you need help or want to request additional features!<br><br>
Currently, this mod has been designed with <b>singleplayer</b> in mind. That means certain features might glitch out or completely stop working in <b>multiplayer</b>.<br>
If that happens, please open a bug report!<br><br>
This mod is supposed to survive game updates, but sometimes it doesn't if there's a huge game update. If it's a big game update that broke the mod, I'm most likely already aware and already working on a fix. If not, please open a bug report!<br>
<img src="https://i.imgur.com/nbipzX1.png"/><br>
<a href="https://github.com/ThirteenAG/Ultimate-ASI-Loader">Ultimate ASI Loader</a><br>
<a href="https://github.com/ocornut/imgui">ImGui</a><br>
<a href="https://github.com/aiekick/ImGuiFileDialog">﻿ImGuiFileDialog</a><br>
<a href="https://github.com/TsudaKageyu/minhook">minhook</a><br>
<a href="https://github.com/Rebzzel/kiero">kiero</a><br>
<a href="https://github.com/coltonon/OpenGameCamera">OpenGameCamera</a><br>
<a href="https://github.com/xvorost/ImGui-Custom-HotKeys">ImGui-Custom-Hotkeys﻿</a><br>
<a href="https://github.com/gabime/spdlog">spdlog</a><br>
<a href="https://github.com/tronkko/dirent">dirent</a><br>
<a href="https://github.com/nothings/stb">﻿stb</a><br>
<a href="https://github.com/freetype/freetype">﻿FreeType﻿</a><br><br>
And <b>@yeeeeeeee.</b> for helping me with certain issues I encountered <3<br>
Big thanks to <b>@coltychen</b> as well! <3<br>
Thanks to <b>@deleted_user_c6caff3e (aka Synsteric)</b> for help with certain things <3<br><br>
This mod is licensed under the MIT License (included with the download and source code).
</p>
