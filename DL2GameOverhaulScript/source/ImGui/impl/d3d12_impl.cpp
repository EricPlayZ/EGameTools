#include <pch.h>
#include "..\menu\menu.h"

namespace impl {
	namespace d3d12 {
		struct FrameContext {
			ID3D12CommandAllocator* commandAllocator = nullptr;
			ID3D12Resource* main_render_target_resource = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE main_render_target_descriptor{};
		};

		static UINT buffersCounts = -1;
		static FrameContext* frameContext = nullptr;

		ID3D12Device* d3d12Device = nullptr;
		ID3D12DescriptorHeap* d3d12DescriptorHeapTextures = nullptr;
		static ID3D12DescriptorHeap* d3d12DescriptorHeapBackBuffers = nullptr;
		static ID3D12DescriptorHeap* d3d12DescriptorHeapImGuiRender = nullptr;
		static ID3D12GraphicsCommandList* d3d12CommandList = nullptr;
		static ID3D12CommandQueue* d3d12CommandQueue = nullptr;

		static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
			for (UINT i = 0; i < buffersCounts; i++) {
				ID3D12Resource* pBackBuffer = nullptr;
				pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				if (pBackBuffer) {
					D3D12_RENDER_TARGET_VIEW_DESC descTarget{};
					descTarget.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					descTarget.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

					d3d12Device->CreateRenderTargetView(pBackBuffer, &descTarget, frameContext[i].main_render_target_descriptor);
					frameContext[i].main_render_target_resource = pBackBuffer;
				}
			}
		}

		static void RenderImGui_DX12(IDXGISwapChain3* pSwapChain) {
			static bool init = false;

			if (!init) {
				DXGI_SWAP_CHAIN_DESC desc{};
				pSwapChain->GetDesc(&desc);
				pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device);

				buffersCounts = desc.BufferCount;
				frameContext = new FrameContext[buffersCounts];

				D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers{};
				descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				descriptorBackBuffers.NumDescriptors = buffersCounts;
				descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				descriptorBackBuffers.NodeMask = 1;
				if (d3d12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&d3d12DescriptorHeapBackBuffers)) != S_OK)
					return;

				SIZE_T rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();
				for (UINT i = 0; i < buffersCounts; ++i) {
					frameContext[i].main_render_target_descriptor = rtvHandle;
					rtvHandle.ptr += rtvDescriptorSize;
				}

				D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender{};
				descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				descriptorImGuiRender.NumDescriptors = 1;
				descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (d3d12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&d3d12DescriptorHeapImGuiRender)) != S_OK)
					return;

				D3D12_DESCRIPTOR_HEAP_DESC descriptorTextures{};
				descriptorTextures.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				descriptorTextures.NumDescriptors = 100;
				descriptorTextures.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (impl::d3d12::d3d12Device->CreateDescriptorHeap(&descriptorTextures, IID_PPV_ARGS(&d3d12DescriptorHeapTextures)) != S_OK)
					return;

				for (size_t i = 0; i < buffersCounts; i++) {
					ID3D12CommandAllocator* allocator = nullptr;
					if (d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
						return;

					frameContext[i].commandAllocator = allocator;
				}

				if (d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContext[0].commandAllocator, NULL, IID_PPV_ARGS(&d3d12CommandList)) != S_OK || d3d12CommandList->Close() != S_OK)
					return;

#ifndef LLMH_IMPL_DISABLE_DEBUG
				std::thread([&desc]() { impl::win32::init(desc.OutputWindow); }).detach();
#else 
				impl::win32::init(desc.OutputWindow);
#endif

				ImGui::CreateContext();
				ImGui::GetIO().IniFilename = nullptr;

				ImGui_ImplWin32_Init(desc.OutputWindow);
				ImGui_ImplDX12_Init(d3d12Device, buffersCounts, DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender, d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());
				ImGui_ImplDX12_CreateDeviceObjects();

				Menu::InitImGuiStyle();
				ImGui_ImplDX12_InvalidateDeviceObjects();

				init = true;
			}

			if (!frameContext[0].main_render_target_resource)
				CreateRenderTarget(pSwapChain);
			if (!d3d12CommandQueue || !frameContext[0].main_render_target_resource)
				return;

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			Menu::FirstTimeRunning();
			if (Menu::menuToggle.GetValue())
				Menu::Render();

			ImGui::Render();

			UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
			ID3D12CommandAllocator* commandAllocator = frameContext[backBufferIdx].commandAllocator;
			commandAllocator->Reset();

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = frameContext[backBufferIdx].main_render_target_resource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			d3d12CommandList->Reset(commandAllocator, NULL);
			d3d12CommandList->ResourceBarrier(1, &barrier);

			d3d12CommandList->OMSetRenderTargets(1, &frameContext[backBufferIdx].main_render_target_descriptor, FALSE, NULL);
			d3d12CommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeapImGuiRender);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			d3d12CommandList->ResourceBarrier(1, &barrier);
			d3d12CommandList->Close();

			d3d12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&d3d12CommandList));
		}

		HRESULT(__stdcall* oPresent)(IDXGISwapChain3*, UINT, UINT);
		HRESULT __stdcall hkPresent12(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
			RenderImGui_DX12(pSwapChain);

			return oPresent(pSwapChain, SyncInterval, Flags);
		}

		HRESULT(__stdcall* oPresent1)(IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS* pPresentParameters);
		HRESULT __stdcall hkPresent112(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
			RenderImGui_DX12(pSwapChain);

			return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
		}

		static void CleanupRenderTarget() {
			for (UINT i = 0; i < buffersCounts; ++i) {
				if (frameContext[i].main_render_target_resource) {
					frameContext[i].main_render_target_resource->Release();
					frameContext[i].main_render_target_resource = NULL;
				}
			}
		}

		HRESULT(__stdcall* oResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
		HRESULT hookResizeBuffers12(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
			CleanupRenderTarget();

			return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		}

		HRESULT(__stdcall* oResizeBuffers1)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*);
		HRESULT hookResizeBuffers112(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
			CleanupRenderTarget();

			return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
		}

		void(__stdcall* oExecuteCommandLists)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
		void hookExecuteCommandLists12(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
			if (!d3d12CommandQueue)
				d3d12CommandQueue = queue;

			oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
		}

		void init() {
			assert(kiero::bind(140, (LPVOID*)&oPresent, hkPresent12) == kiero::Status::Success);
			assert(kiero::bind(154, (LPVOID*)&oPresent1, hkPresent112) == kiero::Status::Success);

			assert(kiero::bind(145, (LPVOID*)&oResizeBuffers, hookResizeBuffers12) == kiero::Status::Success);
			assert(kiero::bind(171, (LPVOID*)&oResizeBuffers1, hookResizeBuffers112) == kiero::Status::Success);

			assert(kiero::bind(54, (LPVOID*)&oExecuteCommandLists, hookExecuteCommandLists12) == kiero::Status::Success);
		}
	}
}