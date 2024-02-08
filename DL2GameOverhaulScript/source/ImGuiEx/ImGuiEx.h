#pragma once
#include "..\core.h"
#include "Animation.h"

namespace ImGui {
	extern void StyleScaleAllSizes(ImGuiStyle* style, float scale_factor);
	extern bool Checkbox(const char* label, Option* v);
	extern bool CheckboxHotkey(const char* label, KeyBindOption* v);
	extern void TextCentered(const char* text);
	extern void TextCenteredColored(const char* text, const ImU32& col);
	extern bool ButtonCentered(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f));
	extern void SeparatorTextColored(const char* text, const ImU32& col);
	extern void Spacing(const ImVec2& size, const bool& customPosOffset = false);
}