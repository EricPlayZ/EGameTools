![EGameTools - an ASI mod for Dying Light 2﻿](https://i.imgur.com/8b0smkU.jpeg)
# EGameTools - an ASI mod for Dying Light 2﻿

## What is EGameTools?
EGameTools (short for Eric's Game Tools) is an ASI mod menu developed in C++, using ImGui, which aims to help ease mod developers in the process of creating mods and which also aims to add new, additional features that the game does not provide by default, such as FreeCam, Weather and Time control, etc.

## What is an ASI?
An ASI is basically just a DLL file with a different file extension. There's a certain ASI loader, in this case `winmm.dll`, which gets loaded by the game, and the ASI loader then scans the game folder for ASI files and injects them into the game process.

## Features
Player:
- God Mode
- Freeze Player
- Player Variables editing (with the ability to save and load to/from a `player_variables.scr`, restore to default values)

Camera:
- FreeCam
- Teleport Player to Camera
﻿- Third Person Mode
- FOV slider
- Disable PhotoMode Limits
- Disable Safezone FOV Reduction

World:
- Time slider
- Weather control

**Check [EGameTools Trello](https://trello.com/b/oRaJQEOi/egametools-dying-light-2)﻿ to see what features or bugfixes I'm working on/planning to add!**

## Installation
Download the latest archive from the [Releases](https://github.com/EricPlayZ/DL2GameOverhaul/releases) page and extract the files from inside the archive to the game's exe folder (`Dying Light 2\ph\work\bin\x64`).

## Uninstallation
Delete `winmm.dll`, `EGameTools.asi` and `EGameTools.ini` from the game's exe folder (`Dying Light 2\ph\work\bin\x64`).

## How do I use it?
When you launch the game, a console window will appear. When it does, **DON'T CLOSE IT!** Closing it will also close the game. This console is meant for debugging/troubleshooting purposes later down the road when the mod gets more complex.

By default, to toggle the mod menu, in-game press **F5** on your keyboard. The same key will open and close the menu. You can use your mouse to navigate the menu.
A config file `EGameTools.ini` is stored in the same folder as the mod file. In here, there will be a value called `ToggleKey` where `VK_F5` is specified. This is the key for toggling the mod menu. To change it, visit [this link from Microsoft](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) which contains all virtual key codes. Simply write the name of the keycode you want to use and save the config file.

**YOU DON'T HAVE TO REOPEN THE GAME FOR CHANGES TO THE CONFIG FILE TO TAKE EFFECT!** Changes made to the config file are constantly refreshed in the mod, so you can edit the config file while the game is running. If you want to regenerate the config file, delete it and it will automatically create a new file.

For now, this config file is not very useful for most people as it saves some useless information such as last paths used in the Player Variables saving/loading dialogs, or toggles such as Disable PhotoMode Limits/Safezone FOV Reduction.

## Bugs/issues
**Visit [EGameTools Trello](https://trello.com/b/oRaJQEOi/egametools-dying-light-2)﻿ to check for bugs/issues currently present in the mod, as well as features that I'm working on or planning to add!**

## Does this work with Multiplayer?
It should mostly work, but there's some issues with God Mode for example not working properly, the code is made with singleplayer in mind and doesn't care about other players which can cause bugs. If you encounter bugs, please open a bug report, I will gladly take a look at it and see if I can find a workaround for multiplayer too.

## This isn't working anymore, what can I do?
This mod is supposed to survive game updates, but sometimes it doesn't if there's a huge game update. If it's a big game update that broke the mod, I'm most likely already aware and already working on a fix. If not, please open a bug report!

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)

[ImGui](https://github.com/ocornut/imgui)

[ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

[minhook](https://github.com/TsudaKageyu/minhook)

[kiero](https://github.com/Rebzzel/kiero)

[OpenGameCamera](https://github.com/coltonon/OpenGameCamera)

And **@yeeeeeeee.** for helping me with certain issues I encountered <3
Big thanks to **@coltychen** as well! <3

## License
This mod is licensed under the MIT License (included with the download and source code).
