#include <vector>
#include <mutex>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <spdlog\spdlog.h>
#include <ImGui\imgui.h>
#include <ImGui\backends\imgui_impl_dx12.h>
#include <ImGui\backends\imgui_impl_win32.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\Hook.h>
#include <EGT\ImGui_impl\Win32_impl.h>
#include <EGT\ImGui_impl\DeferredActions.h>
#include <EGT\ImGui_impl\NextFrameTask.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Init.h>
#include <EGT\ImGui_impl\D3D12_MicaBlur.h>

namespace EGT::ImGui_impl {
	namespace D3D12 {
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
		static void CleanupRenderTarget() {
			D3D12_MicaBlur::ReleaseGpuResources();
			if (!frameContext)
				return;

			for (UINT i = 0; i < buffersCounts; ++i) {
				if (frameContext[i].main_render_target_resource) {
					frameContext[i].main_render_target_resource->Release();
					frameContext[i].main_render_target_resource = NULL;
				}
			}
		}

		static void InitImGuiRendering(IDXGISwapChain3* pSwapChain) {
			static bool init = false;

			if (!init) {
				DXGI_SWAP_CHAIN_DESC desc{};
				pSwapChain->GetDesc(&desc);
				pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device);
				if (!d3d12Device)
					return;

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
				if (d3d12Device->CreateDescriptorHeap(&descriptorTextures, IID_PPV_ARGS(&d3d12DescriptorHeapTextures)) != S_OK)
					return;

				for (size_t i = 0; i < buffersCounts; i++) {
					ID3D12CommandAllocator* allocator = nullptr;
					if (d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
						return;

					frameContext[i].commandAllocator = allocator;
				}

				if (d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContext[0].commandAllocator, NULL, IID_PPV_ARGS(&d3d12CommandList)) != S_OK || d3d12CommandList->Close() != S_OK)
					return;

				Win32::Init(desc.OutputWindow);

				ImGui::CreateContext();
				ImGui::GetIO().IniFilename = nullptr;

				ImGui_ImplWin32_Init(desc.OutputWindow);
				ImGui_ImplDX12_Init(d3d12Device, buffersCounts, DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender, d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());
				ImGui_ImplDX12_CreateDeviceObjects();

				Menu::InitImGui();
				ImGui_ImplDX12_InvalidateDeviceObjects();

				D3D12_MicaBlur::EnsureInitialized(d3d12Device);

				init = true;
			}
		}
		static void RenderImGui(IDXGISwapChain3* pSwapChain) {
			InitImGuiRendering(pSwapChain);

			if (!frameContext[0].main_render_target_resource)
				CreateRenderTarget(pSwapChain);
			if (!d3d12CommandQueue || !frameContext[0].main_render_target_resource)
				return;

			Menu::SyncMenuFontsBeforeImGuiNewFrame();
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			Menu::UpdateMenuVisibilityAnimFromPoll();
			ImGui::NewFrame();

			Menu::FirstTimeRunning();
			if (Menu::MenuAnimNeedsFrame())
				Menu::Render();

			ImGui::Render();

			DeferredActions::Process();
			NextFrameTask::ExecuteTasks();

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

			{
				float mx0 = 0.0f, my0 = 0.0f, mx1 = 0.0f, my1 = 0.0f;
				if (Menu::GetMenuBackdropRectPx(&mx0, &my0, &mx1, &my1)) {
					D3D12_MicaBlur::EnsureInitialized(d3d12Device);
					DXGI_SWAP_CHAIN_DESC scd{};
					pSwapChain->GetDesc(&scd);
					const UINT bbW = scd.BufferDesc.Width;
					const UINT bbH = scd.BufferDesc.Height;
					const ImGuiIO& io = ImGui::GetIO();
					const float dsx = io.DisplaySize.x > 1.0f ? io.DisplaySize.x : 1.0f;
					const float dsy = io.DisplaySize.y > 1.0f ? io.DisplaySize.y : 1.0f;
					const float sx = static_cast<float>(bbW) / dsx;
					const float sy = static_cast<float>(bbH) / dsy;
					mx0 *= sx;
					mx1 *= sx;
					my0 *= sy;
					my1 *= sy;
					D3D12_MicaBlur::RenderBackdrop(d3d12CommandList, frameContext[backBufferIdx].main_render_target_resource, frameContext[backBufferIdx].main_render_target_descriptor, bbW, bbH, mx0, my0, mx1, my1, Menu::GetMicaBackdropFade());
				}
			}

			d3d12CommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeapImGuiRender);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			d3d12CommandList->ResourceBarrier(1, &barrier);
			d3d12CommandList->Close();

			d3d12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&d3d12CommandList));
		}

		static EGSDK::Utils::Hook::MHook<void*, HRESULT(*)(IDXGISwapChain3*, UINT, UINT), IDXGISwapChain3*, UINT, UINT> DXPresentHook{ "DX12Present", &EGSDK::OffsetManager::Get_DXPresent, [](IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) -> HRESULT {
			__try {
				RenderImGui(pSwapChain);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				SPDLOG_ERROR("Exception thrown rendering ImGui in DX12");
			}

			return DXPresentHook.ExecuteOriginal(pSwapChain, SyncInterval, Flags);
		}, false };
		static EGSDK::Utils::Hook::MHook<void*, HRESULT(*)(IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*), IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*> DX12Present1Hook{ "DX12Present1", &EGSDK::OffsetManager::Get_DX12Present1, [](IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) -> HRESULT {
			__try {
				RenderImGui(pSwapChain);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				SPDLOG_ERROR("Exception thrown rendering ImGui in DX12");
			}

			return DX12Present1Hook.ExecuteOriginal(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
		}, false };

		static EGSDK::Utils::Hook::MHook<void*, HRESULT(*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT), IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT> DXResizeBuffersHook{ "DX12ResizeBuffers", &EGSDK::OffsetManager::Get_DXResizeBuffers, [](IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) -> HRESULT {
			CleanupRenderTarget();

			return DXResizeBuffersHook.ExecuteOriginal(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		}, false };
		static EGSDK::Utils::Hook::MHook<void*, HRESULT(*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*), IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*> DX12ResizeBuffers1Hook{ "DX12ResizeBuffers1", &EGSDK::OffsetManager::Get_DX12ResizeBuffers1, [](IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) -> HRESULT {
			CleanupRenderTarget();

			return DX12ResizeBuffers1Hook.ExecuteOriginal(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
		}, false };

		static EGSDK::Utils::Hook::MHook<void*, void(*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*), ID3D12CommandQueue*, UINT, ID3D12CommandList* const*> DX12ExecuteCommandListsHook{ "DX12ExecuteCommandLists", &EGSDK::OffsetManager::Get_DX12ExecuteCommandLists, [](ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) -> void {
			d3d12CommandQueue = queue;
			DX12ExecuteCommandListsHook.ExecuteOriginal(queue, NumCommandLists, ppCommandLists);
		}, false };

		void Init() {
			assert(DXPresentHook.TryHooking());
			assert(DX12Present1Hook.TryHooking());
			assert(DXResizeBuffersHook.TryHooking());
			assert(DX12ResizeBuffers1Hook.TryHooking());
			assert(DX12ExecuteCommandListsHook.TryHooking());
		}
	}
}