#pragma once

#include <string>
#include <EGSDK\vec2.h>
#include <EGSDK\vec3.h>

namespace EGT::GamePH::Teleport {
	extern std::string GetFormattedPosition(const vec3* position);
	extern bool IsTeleportationDisabled();
	extern void SyncPlayerCoordsToTPCoords();
	extern bool TeleportPlayerTo(const vec3& pos, const vec2& orientation = {});
	extern void UpdateRuntimeState();
}
