#pragma once
#include "menu.h"
	
namespace Menu {
	namespace Teleport {
		struct TeleportLocation {
			std::string name;
			Vector3 pos;
		};

		extern std::vector<TeleportLocation> savedTeleportLocations;

		class Tab : MenuTab {
		public:
			Tab() : MenuTab("Teleport", 3) {}
			void Update() override;
			void Render() override;

			static Tab instance;
		};
	}
}