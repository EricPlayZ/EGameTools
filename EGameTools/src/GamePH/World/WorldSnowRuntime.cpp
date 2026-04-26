#include <EGSDK\Engine\CVars.h>
#include <EGT\GamePH\World\WorldRuntime_Internal.h>
#include <EGT\Menu\World.h>

namespace EGT::GamePH::World {
	void UpdateWorldSnowRuntime() {
		EGSDK::Engine::CVars::ManageVarByBool("f_snow_on", 1.0f, 0.0f, Menu::World::enableSnow.GetValue());
		EGSDK::Engine::CVars::ManageVarByBool("f_snow_dynamic", 1.0f, 0.0f, Menu::World::dynamicSnow.GetValue());
		if (Menu::World::enableSnow.GetValue()) {
			if (auto ref = EGSDK::Engine::CVars::GetVarRef("f_snow_cover"))
				ref->SetValueDirect(Menu::World::snowCover);
			if (auto ref = EGSDK::Engine::CVars::GetVarRef("f_snow_strength"))
				ref->SetValueDirect(Menu::World::snowStrength);
			if (auto ref = EGSDK::Engine::CVars::GetVarRef("f_snow_cover_additional_max"))
				ref->SetValueDirect(Menu::World::snowCoverAdditionalMax);
			if (auto ref = EGSDK::Engine::CVars::GetVarRef("f_snow_cover_time"))
				ref->SetValueDirect(Menu::World::snowCoverTime);
		}
	}
}
