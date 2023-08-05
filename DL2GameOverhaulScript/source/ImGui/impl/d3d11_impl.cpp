#include <d3d11.h>
#include <assert.h>
#include <imgui.h>
#include <backends\imgui_impl_win32.h>
#include <backends\imgui_impl_dx11.h>
#include "..\..\menu\menu.h"
#include "..\..\kiero.h"
#include "d3d11_impl.h"
#include "win32_impl.h"

HRESULT(__stdcall* oPresent)(IDXGISwapChain*, UINT, UINT);
HRESULT __stdcall hkPresent11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	static bool init = false;

	if (!init)
	{
		DXGI_SWAP_CHAIN_DESC desc{};
		pSwapChain->GetDesc(&desc);

		ID3D11Device* device = nullptr;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);

		ID3D11DeviceContext* context = nullptr;
		device->GetImmediateContext(&context);

		impl::win32::init(desc.OutputWindow);

		ImGui::CreateContext();
		ImGui::GetIO().IniFilename = nullptr;

		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX11_Init(device, context);

		init = true;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (Menu::isOpen)
		Menu::Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return oPresent(pSwapChain, SyncInterval, Flags);
}

void impl::d3d11::init() {
	assert(kiero::bind(8, (LPVOID*)&oPresent, hkPresent11) == kiero::Status::Success);
}