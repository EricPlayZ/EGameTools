#include <EGT\GamePH\Player\PlayerRuntime_Internal.h>
#include <EGT\GamePH\Player\PlayerRuntime.h>

namespace EGT::GamePH::Player {
	void UpdateRuntimeState() {
		UpdatePlayerOptionStateRuntime();
		UpdatePlayerPositionRuntime();
		UpdatePlayerStatsRuntime();
		UpdatePlayerRestrictionsRuntime();
		UpdatePlayerVarsRuntime();
	}
}
