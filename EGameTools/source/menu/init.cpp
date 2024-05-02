#include <pch.h>
#include "..\changelog.h"
#include "..\game\GamePH\gameph_misc.h"
#include "menu.h"

namespace Menu {
    static std::string welcomeTitle{};
    static std::string changelogTitle{};

    static constexpr ImGuiWindowFlags welcomeWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    static constexpr ImVec2 defMinWndSize = ImVec2(800.0f, 0.0f);
    static ImVec2 minWndSize = defMinWndSize;
    static constexpr ImVec2 defMaxWndSize = ImVec2(800.0f, 700.0f);
    static ImVec2 maxWndSize = defMaxWndSize;

    static Utils::Time::Timer timePassedFromWelcomeScreen{};

    static void CreateChangelogFile() {
        try {
            const std::string localAppDataDir = Utils::Files::GetLocalAppDataDir();
            if (localAppDataDir.empty())
                return;
            const std::string dirPath = std::string(localAppDataDir) + "\\EGameTools\\";
            std::filesystem::create_directories(dirPath);

            const std::string finalPath = dirPath + MOD_VERSION_STR + ".changelog";
            if (!std::filesystem::exists(finalPath)) {
                std::ofstream outFile(finalPath.c_str(), std::ios::binary);
                if (!outFile.is_open())
                    return;
                outFile.close();
            }
        } catch (const std::exception& e) {
            spdlog::error("Exception thrown while trying to create changelog file: {}", e.what());
        }
    }
    static bool DoesChangelogFileExist() {
        const std::string localAppDataDir = Utils::Files::GetLocalAppDataDir();
        if (localAppDataDir.empty())
            return false;
        return std::filesystem::exists(std::string(localAppDataDir) + "\\EGameTools\\" + MOD_VERSION_STR + ".changelog");
    }

    static void RenderWelcomeScreen() {
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        if (ImGui::BeginPopupModal(welcomeTitle.c_str(), nullptr, welcomeWindowFlags)) {
            ImGui::TextCenteredColored("PLEASE read the following text!", IM_COL32(230, 0, 0, 255));
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            const std::string thankYou = "Thank you for downloading EGameTools (" + std::string(MOD_VERSION_STR) + ")!";
            ImGui::TextCentered(thankYou.c_str());
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            const std::string gameCompat = "This version of the mod is compatible with game version v" + GamePH::GameVerToStr(GAME_VER_COMPAT) + ".";
            ImGui::TextCentered(gameCompat.c_str());
            const std::string gameVer = "The game version you are currently running is v" + GamePH::GameVerToStr(Core::gameVer) + ".";
            ImGui::TextCentered(gameVer.c_str());
            if (Core::gameVer != GAME_VER_COMPAT) {
                const std::string gameNotCompat = "Please note that your game version has not been officially tested with this mod, therefore expect bugs, glitches or the mod to completely stop working. If so, please " + std::string(Core::gameVer > GAME_VER_COMPAT ? "wait for a new patch." : "upgrade your game version to one that the mod supports.");
                ImGui::TextCenteredColored(gameNotCompat.c_str(), IM_COL32(200, 0, 0, 255));
            }
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCentered("I will not bore you with what this mod is about, so let's get right to teaching you how to use it!");

            ImGui::SeparatorTextColored("Menu Toggle", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("The default key for opening/closing the menu is F5. You can use your mouse to navigate the menu.");
            ImGui::TextCentered("To change it, you can open up the menu and change the hotkey by clicking the hotkey button for \"Menu Toggle Key\" and then pressing a key on your keyboard.");

            ImGui::SeparatorTextColored("FreeCam", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("While using FreeCam, you can press Shift or Alt to boost your speed or slow you down respectively.");
            ImGui::TextCentered("You can also use the scroll wheel to change FreeCam speed while FreeCam is enabled.");

            ImGui::SeparatorTextColored("Menu Sliders", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("To manually change the value of a slider option, hold \"CTRL\" while clicking the slider.");
            ImGui::TextCentered("This will let you input a value manually into the slider, which can also go beyond the option's slider limit, given I allow the option to do so.");

            ImGui::SeparatorTextColored("Custom File Loading", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("The mod always creates a folder \"EGameTools\\UserModFiles\" inside the same folder as the game executable (exe) or in the same folder as the mod file.");
            ImGui::TextCentered("This folder is used for custom file loading. This has only been tested with a few mods that change some .scr files, gpufx files, and other files included inside .pak game archives, or files like .rpack files.");
            ImGui::TextCentered("Files in this folder must have the same names as the ones from the game files, otherwise the game won't know it should load those files. Files in subfolders of the \"EGameTools\\UserModFiles\" folder will automatically be detected, so you can sort all your mods in different folders!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCentered("The game will reload a lot of the files upon a load of your savegame, so if you want to edit those files and reload them without having to restart the game, just reload your savegame and the game should automatically reload most of those files!");
            ImGui::TextCentered("Just make sure that if you add new, additional files while you're in-game, please wait AT LEAST 5 seconds before reloading your savegame, otherwise additional files will not get detected.");
            ImGui::TextCentered("Also, if there are multiple files of the same exact name, the game will pick the first instance of that file it finds in the folder.");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCentered("The gist of it is, you now don't have to use dataX.pak mods anymore! You can open the pak files, extract their files in the \"EGameTools\\UserModFiles\" folder and start the game!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCenteredColored("FOR MOD DEVELOPERS", IM_COL32(200, 0, 0, 255));
            ImGui::TextCentered("If you want to make mods for EGameTools to load, please try to use as few folders as you possibly can. For example, your mod should only have one folder, something like \"EGameTools\\UserModFiles\\2019 Weather Mod\".");
            ImGui::TextCentered("The reason is, my mod continuously checks for new files in the directory, and many folders can slow down the process, and therefore slow down game loading times. So just keep this in mind!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCentered("If you want to officially include one of your mods as part of EGameTools, please contact me on NexusMods or on Discord (@EricPlayZ).");

            ImGui::SeparatorTextColored("Game Variables Reloading", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("You can also reload Player Variables from a file specified by you, or reload Jump Parameters from \"EGameTools\\UserModFiles\".");

            ImGui::SeparatorTextColored("Hotkeys", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("Most mod menu options are toggleable by a hotkey that you can change by clicking the hotkey button for the respective option and then pressing a key on your keyboard.");
            ImGui::TextCentered("To change those hotkeys through the config file, visit the \"Virtual-Key Codes\" page from Microsoft which contains a list of all virtual key codes. Simply write the name of the keycode you want to use for each hotkey and save the config file.");

            ImGui::SeparatorTextColored("Config", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("A config file \"EGameTools.ini\" is stored in the same folder as the game executable (exe) or in the same folder as the mod file.");
            ImGui::TextCentered("The config file stores the mod menu's options and hotkeys.");

            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            ImGui::TextCentered("Changes to the mod menu or to the config file are always automatically saved or refreshed respectively.");
            ImGui::TextCentered("You DO NOT NEED to restart the game for the changes in the config to be applied!");
            ImGui::TextCentered("If you want to regenerate the config file, delete it and it will automatically be regenerated.");

            ImGui::SeparatorTextColored("Logging", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("Log files will be stored in the EGameTools folder as \"log.x.txt\", x being the number of the previous log file. The most recent log file will be called \"log.txt\".");

            ImGui::SeparatorTextColored("Multiplayer", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("Currently, this mod has been designed with singleplayer in mind. That means certain features might glitch out or completely stop working in multiplayer.");
            ImGui::TextCentered("If that happens, please open a bug report!");

            ImGui::Separator();
            ImGui::NewLine();
            ImGui::TextCentered("Finally, if you've got any issue, no matter how small, please make sure to report it! I will try my best to fix it. I want this mod to be polished and enjoyable to use!");
            ImGui::TextCentered("If you've got any suggestions for how I could improve the mod, in terms of UI, features, among other things, please let me know!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            ImGui::BeginDisabled(!timePassedFromWelcomeScreen.DidTimePass());
            {
                const std::string btnText = "Let me play!" + (!timePassedFromWelcomeScreen.DidTimePass() ? (" (" + std::to_string(10 - (timePassedFromWelcomeScreen.GetTimePassed() / 1000)) + ")") : "");
                if (ImGui::ButtonCentered(btnText.c_str(), ImVec2(0.0f, 30.0f) * scale)) {
                    ImGui::CloseCurrentPopup();
                    firstTimeRunning.Set(false);
                    if (hasSeenChangelog.GetValue())
                        menuToggle.SetChangesAreDisabled(false);
                }
                ImGui::EndDisabled();
            }
            ImGui::EndPopup();
        }
    }
    static void RenderChangelog() {
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        if (ImGui::BeginPopupModal(changelogTitle.c_str(), nullptr, welcomeWindowFlags)) {
            const std::string subTitle = "This is what the " + std::string(MOD_VERSION_STR) + " update brings:";
            ImGui::TextCenteredColored(subTitle.c_str(), IM_COL32(230, 0, 0, 255), false);
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            std::istringstream iss(Changelog::changelogs[MOD_VERSION_STR]);
            std::string line{};
            while (std::getline(iss, line)) {
                if (line.empty()) {
                    ImGui::NewLine();
                    continue;
                }
                ImGui::TextCentered(line.c_str(), false);
            }

            if (ImGui::ButtonCentered("Close", ImVec2(0.0f, 30.0f) * scale)) {
                ImGui::CloseCurrentPopup();
                menuToggle.SetChangesAreDisabled(false);
                hasSeenChangelog.Set(true);
                firstTimeRunning.Set(false);
                CreateChangelogFile();
            }
            ImGui::EndPopup();
        }
    }
    void FirstTimeRunning() {
        static bool firstTimeRunningFunc = true;
        if (firstTimeRunningFunc)
            hasSeenChangelog.SetBothValues(hasSeenChangelog.GetValue() && DoesChangelogFileExist());

        if (hasSeenChangelog.GetValue() && !firstTimeRunning.GetValue()) {
            firstTimeRunningFunc = false;
            return;
        }

        ImGui::StyleScaleAllSizes(&ImGui::GetStyle(), scale, &defStyle);
        ImGui::GetIO().FontGlobalScale = scale;
        minWndSize = defMinWndSize * scale;
        maxWndSize = defMaxWndSize * scale;

        if (firstTimeRunningFunc) {
            firstTimeRunningFunc = false;
            timePassedFromWelcomeScreen = Utils::Time::Timer(10000);
            menuToggle.SetChangesAreDisabled(true);

            welcomeTitle = std::string(title + " - Welcome!");
            changelogTitle = std::string(title + " - Update Changelog");

            if (firstTimeRunning.GetValue())
                ImGui::OpenPopup(welcomeTitle.c_str());
            else if (!hasSeenChangelog.GetValue())
                ImGui::OpenPopup(changelogTitle.c_str());
        }
        RenderWelcomeScreen();

        static bool changelogPopupOpened = false;
        if (!hasSeenChangelog.GetValue() && !firstTimeRunning.GetValue() && !changelogPopupOpened) {
            changelogPopupOpened = true;
            ImGui::OpenPopup(changelogTitle.c_str());
        }
        RenderChangelog();
    }

    void InitImGui() {
        ImGuiStyle* style = &ImGui::GetStyle();

        style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style->WindowPadding = ImVec2(15, 15);
        style->WindowRounding = 5.0f;
        style->ChildRounding = 4.0f;
        style->FramePadding = ImVec2(5, 5);
        style->FrameRounding = 4.0f;
        style->ItemSpacing = ImVec2(12, 8);
        style->ItemInnerSpacing = ImVec2(8, 6);
        style->IndentSpacing = 25.0f;
        style->ScrollbarSize = 15.0f;
        style->ScrollbarRounding = 9.0f;
        style->GrabMinSize = 5.0f;
        style->GrabRounding = 3.0f;

        style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
        style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style->Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
        style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4705f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
        style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.13f, 0.12f, 0.15f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.5882f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4705f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style->Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.4705f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_Header] = ImVec4(0.5882f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.4705f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_Tab] = ImVec4(0.5882f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_TabHovered] = ImVec4(0.4705f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_TabActive] = ImVec4(1.0f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5882f, 0.0784f, 0.1176f, 1.00f);
        style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

        defStyle = *style;

        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig fontConfig{};
        fontConfig.FontDataOwnedByAtlas = false;
        io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)g_FontRudaBold, sizeof(g_FontRudaBold), 12.0f + 6.0f, &fontConfig);
        io.Fonts->Build();

        spdlog::warn("Loading EGameTools logo texture");
        EGTLogoTexture = Utils::Texture::LoadImGuiTexture(g_EGTWhiteLogo, sizeof(g_EGTWhiteLogo));
        spdlog::info("Loaded EGameTools logo");
    }
}