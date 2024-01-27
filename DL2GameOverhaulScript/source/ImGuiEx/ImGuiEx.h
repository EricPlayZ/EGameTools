#pragma once
#include "..\core.h"

namespace ImGui {
	bool Checkbox(const char* label, Option* v);
	void BeginDisabled(bool disabled, Option* v);
}