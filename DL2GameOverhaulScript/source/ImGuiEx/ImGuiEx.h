#pragma once
#include "..\core.h"

namespace ImGui {
	extern void StyleScaleAllSizes(ImGuiStyle* style, float scale_factor);
	extern bool Checkbox(const char* label, Option* v);
}