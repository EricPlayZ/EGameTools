#include <EGT\GamePH\Misc\MiscRuntime_Internal.h>
#include <EGT\GamePH\Misc\MiscRuntime.h>

namespace EGT::GamePH::Misc {
	void UpdateRuntimeState() {
		UpdateMiscCVarsRuntime();
		UpdateMiscHudAndAfkRuntime();
	}
}
