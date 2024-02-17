#pragma once
#include "..\core.h"

namespace Menu {
    class MenuTab {
    public:
        MenuTab(std::string_view name, int tabIndex) : tabName(name), tabIndex(tabIndex) { GetInstances()->insert({ tabIndex, this}); };
        ~MenuTab() { GetInstances()->erase({ tabIndex, this }); }
        static std::set<std::pair<int, MenuTab*>>* GetInstances() { static std::set<std::pair<int, MenuTab*>> instances{}; return &instances; };

        virtual void Render() {};
        virtual void Update() {};

        std::string_view tabName{};
    private:
        int tabIndex{};
    };

    extern const std::string title;
    extern ImGuiStyle defStyle;
    extern ImTextureID EGTLogoTexture;

	extern KeyBindOption menuToggle;
	extern float opacity;
	extern float scale;

    extern Option firstTimeRunning;
    extern Option hasSeenChangelog;

	extern void Render();
}