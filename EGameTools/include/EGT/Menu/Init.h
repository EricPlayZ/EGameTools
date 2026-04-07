#pragma once

struct ImFont;

namespace EGT::Menu {
	extern void FirstTimeRunning();

	extern void InitImGui();

	/// Rebuild font atlas at `Menu::scale` and invalidate GPU font texture. Call once per frame before `ImGui_ImplDX11/DX12_NewFrame()`.
	extern void SyncMenuFontsBeforeImGuiNewFrame();

	/// Slightly larger Ruda font for the version label (same atlas; null before first sync).
	extern ImFont* MenuVersionFont();
}