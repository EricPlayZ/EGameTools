#include <pch.h>
#define STB_IMAGE_IMPLEMENTATION
#include <misc\stb\stb_image.h>

namespace impl {
    namespace d3d11 {
        extern ID3D11Device* d3d11Device;
    }
    namespace d3d12 {
        extern ID3D12Device* d3d12Device;
        extern ID3D12DescriptorHeap* d3d12DescriptorHeapTextures;
    }
}
namespace Core {
    extern int rendererAPI;
}

namespace Utils {
    namespace Texture {
        static int descriptor_index = 1;
        
        static ImTextureID LoadImGuiTextureD3D11(const unsigned char* rawData, const int rawSize) {
            int image_width = 0;
            int image_height = 0;
            unsigned char* image_data = stbi_load_from_memory(rawData, rawSize, &image_width, &image_height, nullptr, 4);
            if (!image_data)
                return nullptr;

            D3D11_TEXTURE2D_DESC desc{};
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = image_width;
            desc.Height = image_height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            ID3D11Texture2D* pTexture = nullptr;
            D3D11_SUBRESOURCE_DATA subResource{};
            subResource.pSysMem = image_data;
            subResource.SysMemPitch = desc.Width * 4;
            subResource.SysMemSlicePitch = 0;
            impl::d3d11::d3d11Device->CreateTexture2D(&desc, &subResource, &pTexture);
            if (!pTexture)
                return nullptr;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;

            ID3D11ShaderResourceView* my_texture = nullptr;
            impl::d3d11::d3d11Device->CreateShaderResourceView(pTexture, &srvDesc, &my_texture);
            
            pTexture->Release();
            stbi_image_free(image_data);

            return static_cast<ImTextureID>(my_texture);
        }
        static ImTextureID LoadImGuiTextureD3D12(const unsigned char* rawData, const int rawSize) {
            int image_width = 0;
            int image_height = 0;
            unsigned char* image_data = stbi_load_from_memory(rawData, rawSize, &image_width, &image_height, nullptr, 4);
            if (!image_data)
                return nullptr;
            
            D3D12_HEAP_PROPERTIES props{};
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC desc{};
            ZeroMemory(&desc, sizeof(desc));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = image_width;
            desc.Height = image_height;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            ID3D12Resource* pTexture = nullptr;
            impl::d3d12::d3d12Device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pTexture));

            const UINT uploadPitch = (image_width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
            const UINT uploadSize = image_height * uploadPitch;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = uploadSize;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            ID3D12Resource* uploadBuffer = nullptr;
            HRESULT hr = impl::d3d12::d3d12Device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
            IM_ASSERT(SUCCEEDED(hr));

            LPVOID mapped = nullptr;
            D3D12_RANGE range = { 0, uploadSize };
            hr = uploadBuffer->Map(0, &range, &mapped);
            IM_ASSERT(SUCCEEDED(hr));
            for (int y = 0; y < image_height; y++)
                memcpy(reinterpret_cast<LPVOID>((reinterpret_cast<ULONG64>(mapped) + y * static_cast<ULONG64>(uploadPitch))), image_data + y * image_width * 4, static_cast<size_t>(image_width) * 4);
            uploadBuffer->Unmap(0, &range);

            D3D12_TEXTURE_COPY_LOCATION srcLocation{};
            srcLocation.pResource = uploadBuffer;
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = image_width;
            srcLocation.PlacedFootprint.Footprint.Height = image_height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

            D3D12_TEXTURE_COPY_LOCATION dstLocation{};
            dstLocation.pResource = pTexture;
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pTexture;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            ID3D12Fence* fence = nullptr;
            hr = impl::d3d12::d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
            IM_ASSERT(SUCCEEDED(hr));

            HANDLE event = CreateEvent(0, 0, 0, 0);
            IM_ASSERT(event);

            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 1;

            ID3D12CommandQueue* cmdQueue = nullptr;
            hr = impl::d3d12::d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
            IM_ASSERT(SUCCEEDED(hr));

            ID3D12CommandAllocator* cmdAlloc = nullptr;
            hr = impl::d3d12::d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
            IM_ASSERT(SUCCEEDED(hr));

            ID3D12GraphicsCommandList* cmdList = nullptr;
            hr = impl::d3d12::d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
            IM_ASSERT(SUCCEEDED(hr));

            cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            cmdList->ResourceBarrier(1, &barrier);

            hr = cmdList->Close();
            IM_ASSERT(SUCCEEDED(hr));

            cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&cmdList));
            hr = cmdQueue->Signal(fence, 1);
            IM_ASSERT(SUCCEEDED(hr));

            fence->SetEventOnCompletion(1, event);
            WaitForSingleObject(event, INFINITE);

            cmdList->Release();
            cmdAlloc->Release();
            cmdQueue->Release();
            CloseHandle(event);
            fence->Release();
            uploadBuffer->Release();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            const UINT handle_increment = impl::d3d12::d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle = impl::d3d12::d3d12DescriptorHeapTextures->GetCPUDescriptorHandleForHeapStart();
            my_texture_srv_cpu_handle.ptr += (static_cast<ULONG64>(handle_increment) * descriptor_index);
            D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle = impl::d3d12::d3d12DescriptorHeapTextures->GetGPUDescriptorHandleForHeapStart();
            my_texture_srv_gpu_handle.ptr += (static_cast<ULONG64>(handle_increment) * descriptor_index);
            impl::d3d12::d3d12Device->CreateShaderResourceView(pTexture, &srvDesc, my_texture_srv_cpu_handle);

            descriptor_index += 1;
            stbi_image_free(image_data);

            return reinterpret_cast<ImTextureID>(my_texture_srv_gpu_handle.ptr);
        }

        ImTextureID LoadImGuiTexture(const unsigned char* rawData, const int rawSize) {
            return Core::rendererAPI == 11 ? LoadImGuiTextureD3D11(rawData, rawSize) : LoadImGuiTextureD3D12(rawData, rawSize);
        }
    }
}