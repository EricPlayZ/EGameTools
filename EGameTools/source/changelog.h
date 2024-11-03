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
		R"(Before you read the changelog of this mod, I want to make a few statements, regarding the future of Dying Light 2 and modding, as well as to raise attention to other modders.
Recently, some of you, or many of you might have noticed that one of the most popular and known mods for Dying Light 2, or the second most popular on NexusMods, IALR, aka "I Am Legion Reborn", has been very unfortunately discontinued.
I'm not going to dive into it too much as it has been explained very well by the author himself, DocOnHoliday, and I recommend you check out his latest video from YouTube (@DocOnHoliday) and also the description and the articles pages of the NexusMods page, where everything is explained in detail.

The short version of the situation is that a while ago, almost a year ago in fact, Techland reached out to Doc in order to try and support his mod officially, seeing how well it did independently.
After the entire ordeal with Tencent's acquisition of the majority of Techland, Techland stopped reaching out to Doc for unknown reasons. Some time passed, and some EULA changes were made, effectively granting Techland the ability of practically owning your mods and being able to do anything with them.
I quote the EULA: "Techland will in particular have the right to copy, reproduce, fix, modify, adapt, translate, prepare derivative works, publish, distribute, license, sublicense, transfer, publicly display, publicly perform, provide access to, and broadcast Mods as well as all modified and derivative works thereof."
Assuming this change was made due to Tencent's entry into the field, everyone was mad, and rightfully so. Doc still hasn't received any sort of communication from Techland ever since, and recently, after so long, someone from Techland reached out to him.
It was unfortunate news for Doc. He was asked to take down his Patreon (effectively a verbal cease & desist, which technically is not a cease & desist but I'll get more into it later) because of a report made by someone in the community, stating that a paywall to his mod was made on Patreon, implying that Doc was paywalling IALR.
That statement is completely false, as what Doc was doing at the time was simply offering Patreon supporters a special "thank you" gift, and I quote Doc: "It in no way contained anything that was meant to be paywall restricted, rather was access to a few tools I used for playtesting the mods content, and immediate access to all of the new armors, weapons, and other content for playtesting themselves."
Clearly, Doc was not paywalling his mod, IALR, and he was only offering access to special tools that the user would not need for the mod to work or to provide the full experience of the mod.
But, obviously, Techland being a business (not just a game dev studio, remember that the gaming industry has become a business industry nowadays), and especially after the whole Tencent ordeal, they've still decided to ask Doc to take down his Patreon for really hard to understand reasons, regardless of whether he was actually truly paywalling his mod or not (which I state again, he was not).
Doc made a very informed and hard decision to stop further development for his mod completely, even if Techland was only threatening his Patreon.
It was very obvious that Techland might try to pull other shenanigans in the future, regardless of him taking down his Patreon, which keep in mind was (still is, but not for IALR anymore) his main source of donations. Yes, donations, nobody was paying him to get access to a mod... They were paying him to support him and his work.

The reason I'm writing this is because it is important for anyone else working on mods for this game, or even for games made by Techland, that you need to be aware of possible consequences of modding.
A company can make the most unexpected change or decision that you would never think of, as seen with Doc's example. My mod breaks a few EULA agreements, and I'm afraid for the future of this mod knowing that it's only going to grow, because Techland can have the mod removed in the blink of an eye if they add PvP into Dying Light 2 and decide that my mod is problematic, especially with the mod being open-source and even if I did implement an anti-cheat system, anyone could download the source code, modify it and redistribute it because of open-source licensing. Unfortunate world we live in...
So either I would have to take down my mod from all sources, or Techland would implement an anti-cheat system which also prevents my mod from properly working in multiplayer. Even if I didn't try to circumvent the anti-cheat system, others would because the mod is open-source.
Besides, the entire Patreon problem is ridiculous. I understand that Techland would want to set better modding precedents regarding distribution and development of mods, such as stopping developers from paywalling their mods (for months or even years!).
But Doc in this scenario had nothing to do with that, which is what worries me about what other future decisions Techland can and might take towards other mods, especially after seeing their behavior with Doc, who only wanted the game's community good by making his mod more official, polished and easier to maintain.
He only had great goals for the game in mind, and so did Techland as you've seen in the previous few paragraphs, until Tencent came into play. I feel like he was completely disregarded for so long, and to make the final blow, they did that with his Patreon.

So, if you have a long-term modding project in your mind for Dying Light 2, please take a step back and think about it before fully committing your life to it. It's not official (unless Techland reaches out to you and wants to make it official and doesn't ghost you lol), you don't own anything and you agreed to Techland's EULA when you purchased the game.
The people working at Techland may be nice, but remember, it's a business, and business decisions will be made, whether they're good for the community of this game or not. Usually (not always), when companies make money, they don't care, period.

If anyone at Techland is reading this, you have a great dev team and you've made a thriving game series that everyone enjoys, and that I still enjoy to this day. We all have our ups and downs, but these decisions make me feel opposite to what I feel for the Dying Light series, about the state of the company.
The game leans too much into monetization... You've done so many harsh changes ever since Tencent came, many micro-transactions, EULA changes specific to modding, and seeing this whole Patreon ordeal, and the lack of communication with the modding community, makes me think you don't always completely wish this community the best.
Whether this is Tencent's intention or not, you are the ones who agreed to the deal.



With all of that said, here is finally what this update brings:

- Added compatibility with v1.16.2 hotfix update
- Added the ability of using .PAK mods inside "EGameTools\UserModFiles"; just drag and drop a .PAK inside the folder, rename it to whatever you like and enjoy! CREDITS TO @12brendon34 on Discord for finding out how to implement this feature!
- Added "Player Immunity" slider (Player)
- Added "Old World Money" slider (Player)
- Added "Unlimited Immunity" (Player)
- Added "Unlimited Stamina" (Player)
- Added "Unlimited Items" (Player) - stops the game from lowering the amount of items such as consumables / throwables when using them, alongside other inventory items such as ammo, lockpicks and other items
  - WARNING: this will not stop the item from getting removed from your inventory if you drop the entire amount; currently, if the amount of item is 1, it will still drop from your inventory unfortunately
- Added "One-Hit Kill" (Player)
- Added "Invisible to Enemies" (Player)
- Added "Allow Grapple Hook in Safezone" (Player)
- Added "Disable Air Control" (Player)
- Added "Current Weapon Durability" slider (Weapon) - currently only works while your weapon is physically equipped in your hand
- Added "Unlimited Durability" (Weapon)
- Added "Unlimited Ammo" (Weapon)
- Added "No Spread" (Weapon)
- Added "No Recoil" (Weapon)
- Added "Instant Reload" (Weapon)
- Added "Lens Distortion" slider (Camera)
- Added "GoPro Mode" (Camera) - this is best used with Head Bob Reduction set to 0 and Player FOV Correction set to 0 in game options; thank you to @c.r.e.x on Discord for the idea of adding this feature!
- Added "Disable Head Correction" (Camera) - disables centering of the player's hands to the center of the camera
- Added "Teleport to Coords" with X, Y, Z inputs (Teleport), with a "Get Player Coords" button which will automatically fill the X, Y and Z inputs
- Added "Teleport to Waypoint" (Teleport)
  - WARNING: if the waypoint is selected to track an object/item on the map, Teleport to Waypoint will not work, if so just set the waypoint nearby instead
  - WARNING: your player height won't change when teleporting, so make sure you catch yourself if you fall under the map because of the teleportation
- Added "Saved Locations" in Teleport menu, with the ability of saving, deleting and teleporting to said locations; these locations are saved in the config file and will contain a name and a set of coordinates for each location; to reset back to the default list, remove the list from inside the config file and go back into the game; thank you to @Synsteric on Discord for helping me make the default list!
- Added "Increase Data PAKs Limit" (Misc; requires game restart to apply) - you can now add more than 8 data PAKs, e.g. data8.pak, data9.pak, data10.pak, etc, up to 200 PAKs in total
- Added "Disable Data PAKs CRC Check" (Misc; requires game restart to apply) - stops the game from scanning data PAKs, which allows you to use data PAK mods in multiplayer as well
- Added "Disable Savegame CRC Check" (Misc; requires game restart to apply) - stops the game from falsely saying your savegame is corrupt whenever you modify it
- Added tooltips when hovering over buttons and sliders inside the mod menu

- Fixed "God Mode" (Player) staying enabled after toggling FreeCam off
- Fixed "God Mode" (Player) not working properly or at all in multiplayer
- Fixed blood overlay still displaying after falling from a great height with "God Mode" (Player) enabled
- Fixed volatiles still being able to kill you when they jump on top of you while "God Mode" (Player) is enabled
- Fixed "Disable Out of Bounds Timer" (Player) not working in missions
- Fixed player variables saving and loading using old version of player_variables.scr (which made Max Health drop to negative infinite)
- Fixed options that make use of player variables not returning back to their original value after disabling them
- Fixed immunity drastically being lowered while rapidly changing the time forward with the "Time" slider (World) at night or while in a dark zone
- Fixed long paths to mods inside UserModFiles causing a game crash or causing the mods to not load at all
- Changed the config system to only write to the config file whenever there's a change in the mod menu
- Changed the way the mod menu gets the list of player variables, meaning the player variables list should self-update, with no manual intervention required even after a game update

NOTE: Any mods that are put inside "EGameTools\UserModFiles" as a regular file (.scr or any other file that is usually present in .PAK mods) and NOT a .PAK file will make the game ignore the same files that are present in any of the .PAK mods inside "EGameTools\UserModFiles". I recommend using .PAK for most mods. If you run into issues, try extracting the files inside the PAK into the folder directly.



I'm soon starting my exams and won't really have the time to update the mod the way I did right now. I had a 2 week leave and so I had plenty of time to further develop this mod.
If anyone is looking to help with development, I'm all eyes and ears! Thank you!)" },
		{ "v1.2.1",
		R"(- Added compatibility with v1.17.2 Tower Raid update
- Added a .PDB file included by default with the mod, for debugging purposes in case the game crashes, now I can more easily detect the cause of a game crash!
- Added a crash handler which handles game crashes and generates a "EGameTools-dump.dmp" file in the game's exe directory for debugging purposes; if you encounter a crash and this file gets generated, please send it to me anywhere you can, for example on Discord, so I can try to find out the cause of the game crash!
- Fixed "Game Speed" (World) not getting applied with the mod menu opened while having another tab selected other than the World tab
I have some things planned for the next updates, but time will decide when I'll be able to work on the updates. I'm almost done with my exams!)" }
	};
}