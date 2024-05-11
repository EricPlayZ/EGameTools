#pragma once
#include "menu.h"
	
namespace Menu {
	namespace Teleport {
		struct TeleportLocation {
			std::string name;
			Vector3 pos;
		};

		extern std::vector<TeleportLocation> savedTeleportLocations;

		extern KeyBindOption teleportToSelectedLocation;
		extern KeyBindOption teleportToCoords;

		extern void UpdateTeleportLocationVisualNames();
		extern std::vector<TeleportLocation> ParseTeleportLocations(const std::string& input);
		extern std::string ConvertTeleportLocationsToStr(const std::vector<TeleportLocation>& teleportLocations);

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Teleport", 3) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}