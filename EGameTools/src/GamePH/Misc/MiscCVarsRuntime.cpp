#include <EGSDK\Engine\CVars.h>
#include <EGT\GamePH\Misc\MiscRuntime_Internal.h>
#include <EGT\Menu\Misc.h>

namespace EGT::GamePH::Misc {
	void UpdateMiscCVarsRuntime() {
		EGSDK::Engine::CVars::ManageVarByBool("i_pp_taa_on", 0, 1, Menu::Misc::disableTAA.GetValue());
		EGSDK::Engine::CVars::ManageVarByBool("i_pp_jitter_on", 0, 1, Menu::Misc::disableTAA.GetValue());
	}
}
