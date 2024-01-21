#pragma once
#include <InputUtil.h>
#include "camera.h"
#include "player.h"
#include "world.h"

namespace Menu {
	extern KeyBindToggle toggleKey;
	extern bool isOpen;
	extern float transparency;

	extern void Render();
}