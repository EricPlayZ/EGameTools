#define IMGUI_DEFINE_MATH_OPERATORS
#include <Hotkey.h>
#include <ImGuiEx.h>
#include "..\game_classes.h"
#include "menu.h"

namespace Menu {
	static constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar;
	static constexpr ImGuiWindowFlags welcomeWindowFlags = (windowFlags | ImGuiWindowFlags_NoMove) & ~ImGuiWindowFlags_HorizontalScrollbar;
    static constexpr ImVec2 minWelcomeWndSize = ImVec2(700.0f, 0.0f);
    static constexpr ImVec2 defMaxWelcomeWndSize = ImVec2(700.0f, 700.0f);

    static constexpr ImVec2 minWndSize = ImVec2(0.0f, 0.0f);
    static constexpr ImVec2 defMaxWndSize = ImVec2(900.0f, 675.0f);
    static ImVec2 maxWndSize = defMaxWndSize;

    KeyBindOption menuToggle = KeyBindOption(VK_F5);
    float transparency = 99.0f;
    float scale = 1.0f;

    static Utils::Timer timePassedFromWelcomeScreen{};
    static bool firstTimeRunningFunc = true;
    Option firstTimeRunning{};

    void FirstTimeRunning() {
        if (!firstTimeRunning.GetValue())
            return;
        if (firstTimeRunningFunc) {
            firstTimeRunningFunc = false;
            timePassedFromWelcomeScreen = Utils::Timer(10000);
            menuToggle.SetChangesAreDisabled(true);
            ImGui::OpenPopup("EGameTools - Welcome!");
        }

        ImGui::SetNextWindowSizeConstraints(minWelcomeWndSize, defMaxWelcomeWndSize);
        if (ImGui::BeginPopupModal("EGameTools - Welcome!", nullptr, welcomeWindowFlags)) {
            ImGui::TextCenteredColored("PLEASE read the following text!", IM_COL32(230, 0, 0, 255));
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            const std::string thankYou = "Thank you for downloading EGameTools (" + std::string(MOD_VERSION_STR) + ")!";
            ImGui::TextCentered(thankYou.c_str());
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            const std::string gameCompat = "This version of the mod is compatible with game version " + std::string(GAME_VER_COMPAT_STR) + ".";
            ImGui::TextCentered(gameCompat.c_str());
            const std::string gameVer = "The game version you are currently running is v" + GamePH::GetCurrentGameVersionStr() + ".";
            ImGui::TextCentered(gameVer.c_str());
            const std::string gameTestedVer = "This mod has been tested with version v" + GamePH::GameVerToStr(GAME_VER_COMPAT) + ".";
            ImGui::TextCentered(gameTestedVer.c_str());
            if (GamePH::GetCurrentGameVersion() < 11400 || GamePH::GetCurrentGameVersion() > GAME_VER_COMPAT) {
                const std::string gameNotCompat = "Please note that your game version has not been officially tested with this mod, therefore expect bugs, glitches or the mod to completely stop working. If so, please " + std::string(GamePH::GetCurrentGameVersion() > GAME_VER_COMPAT ? "wait for a new patch." : "upgrade your game version to one that the mod supports.");
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
            ImGui::TextCentered("This will let you input a value manually into the slider, which can even surpass the option's slider limit, given I allowed the option to do so.");

            ImGui::SeparatorTextColored("Custom File Loading", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("The mod always creates a folder \"EGameTools\\FilesToLoad\" inside the same folder as the game executable (exe) or in the same folder as the mod file.");
            ImGui::TextCentered("This folder is used for custom file loading. This has only been tested with a few mods that change some .scr files, GPUfx files, and other files included inside .pak game archives, or files like .rpack files.");
            ImGui::TextCentered("Files in this folder must have the same names as the ones from the game files, otherwise the game won't know it should load those files. Files in subfolders of the \"EGameTools\\FilesToLoad\" folder will automatically be detected, so you can sort all your mods in different folders!");
            ImGui::TextCentered("The game will reload a lot of the files upon a load of your savegame, so if you want to edit those files and reload them without having to restart the game, just reload your savegame and the game should automatically reload most of those files!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));
            ImGui::TextCentered("The gist of it is, you now don't have to use dataX.pak mods anymore! You can open the pak files, extract their files in the \"EGameTools\\FilesToLoad\" folder and start the game!");

            ImGui::SeparatorTextColored("Game Variables Reloading", IM_COL32(200, 0, 0, 255));
            ImGui::NewLine();
            ImGui::TextCentered("You can also reload Player Variables from a file specified by you, or reload Jump Parameters from \"EGameTools\\FilesToLoad\".");

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

            ImGui::Separator();
            ImGui::NewLine();
            ImGui::TextCentered("Finally, if you've got any issue, no matter how small, please make sure to report it! I will try my best to fix it. I want this mod to be polished and enjoyable to use!");
            ImGui::TextCentered("If you've got any suggestions for how I could improve the mod, in terms of UI, features, among other things, please let me know!");
            ImGui::Spacing(ImVec2(0.0f, 5.0f));

            ImGui::BeginDisabled(!timePassedFromWelcomeScreen.DidTimePass()); {
                const std::string btnText = "Let me play!" + (!timePassedFromWelcomeScreen.DidTimePass() ? (std::to_string(10 - (timePassedFromWelcomeScreen.GetTimePassed() / 1000)) + ")") : "");
                if (ImGui::ButtonCentered(btnText.c_str(), ImVec2(0.0f, 30.0f))) {
                    ImGui::CloseCurrentPopup();
                    menuToggle.SetChangesAreDisabled(false);
                    firstTimeRunning.Set(false);
                }
                ImGui::EndDisabled();
            }
            ImGui::EndPopup();
        }
    }
	void Render() {
        ImGuiStyle* style = &ImGui::GetStyle();
        style->Colors[ImGuiCol_WindowBg] = ImVec4(style->Colors[ImGuiCol_WindowBg].x, style->Colors[ImGuiCol_WindowBg].y, style->Colors[ImGuiCol_WindowBg].z, static_cast<float>(transparency) / 100.0f);

        maxWndSize = defMaxWndSize * scale;
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        ImGui::Begin("EGameTools", &menuToggle.value, windowFlags); {
            if (ImGui::BeginTabBar("##MainTabBar")) {
                for (auto& tab : *MenuTab::GetInstances()) {
                    if (ImGui::BeginTabItem(tab.second->tabName.data())) {
                        tab.second->Render();
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();

            ImGui::Hotkey("Menu Toggle Key", menuToggle);
            ImGui::SliderFloat("Menu Transparency", &transparency, 0.0f, 100.0f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::SliderFloat("Menu Scale", &scale, 1.0f, 2.5f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp)) {
                ImGui::StyleScaleAllSizes(style, scale);
                ImGui::GetIO().FontGlobalScale = scale;
            }
            ImGui::End();
        }
	}
}