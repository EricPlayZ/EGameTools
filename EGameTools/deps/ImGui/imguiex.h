#pragma once
#include <ImGui\imgui_internal.h>
#include <ImGui\imgui_hotkey.h>

namespace ImGui {
	extern void StyleScaleAllSizes(ImGuiStyle* style, const float scale_factor, ImGuiStyle* defStyle = nullptr);
	extern void SpanNextTabAcrossWidth(const float width, const size_t tabs = 1);
	extern void EndTabBarEx();
	/// Standard `Button` with smoothed hover/active frame colors (same signature as Dear ImGui `Button`).
	extern bool ButtonSmooth(const char* label, const ImVec2& size = ImVec2(0, 0));
	/// Tooltip with short fade-in after hover delay (uses `HoveredIdTimer`).
	extern void SetItemTooltipAnimated(const char* fmt, ...);
	extern bool Button(const char* label, const char* tooltip, const ImVec2& size = ImVec2(0, 0));
	extern bool ButtonHotkey(const char* label, KeyBindOption* v, const char* tooltip = nullptr, const ImVec2& size = ImVec2(0, 0));
	extern bool Checkbox(const char* label, bool* v, const char* tooltip);
	extern bool Checkbox(const char* label, Option* v);
	extern bool Checkbox(const char* label, Option* v, const char* tooltip);
	extern bool CheckboxHotkey(const char* label, KeyBindOption* v, const char* tooltip = nullptr);
	extern bool SliderInt(const char* label, const char* tooltip, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
	extern bool SliderFloat(const char* label, const char* tooltip, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	/// Form helpers: visible label (text before ##) on the left, widget on the same row using remaining width (internal id uses ##… only).
	extern bool SliderFloatStacked(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const char* tooltip = nullptr);
	extern bool SliderFloat3Stacked(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	extern bool SliderIntStacked(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, const char* tooltip = nullptr);
	extern bool DragIntStacked(const char* label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
	extern bool ComboStacked(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
	extern bool InputFloat3Stacked(const char* label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
	extern void TextCentered(const char* fmt, ...);
	extern void TextCenteredColored(const char* fmt, ImU32 col, ...);
	extern bool ButtonCentered(const char* label, const ImVec2 size = ImVec2(0.0f, 0.0f));
	extern void SeparatorTextColored(const char* text, ImU32 col);
	/// `SeparatorText` with optional small gap before it (use `leadingGap = false` for the first header in a tab).
	extern void SeparatorTextSection(const char* label, bool leadingGap = true);
	extern void SeparatorTextSectionColored(const char* label, ImU32 col, bool leadingGap = true);
	extern void DisplaySimplePopupMessage(const char* popupTitle, const char* fmt, ...);
	extern void DisplaySimplePopupMessageCentered(const char* popupTitle, const char* fmt, ...);
	extern void DisplaySimplePopupMessage(float itemWidth, float scale, const char* popupTitle, const char* fmt, ...);
	extern void DisplaySimplePopupMessageCentered(float itemWidth, float scale, const char* popupTitle, const char* fmt, ...);
	extern void Spacing(const ImVec2 size, const bool customPosOffset = false);
}