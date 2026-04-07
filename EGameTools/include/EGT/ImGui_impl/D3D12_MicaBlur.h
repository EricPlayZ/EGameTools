#pragma once
#include <d3d12.h>

namespace EGT::ImGui_impl::D3D12_MicaBlur {
    /// Create PSOs / root signatures (idempotent). Safe to call once after `d3d12Device` is valid.
    bool EnsureInitialized(ID3D12Device* device);

    void ReleaseGpuResources();

    /// Copy back buffer → blur → composite lerp (sharp vs blurred) under the menu rect. Leaves `backBuffer` in RENDER_TARGET.
    void RenderBackdrop(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer, const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferRtv, UINT fullWidth, UINT fullHeight,
        float menuMinX, float menuMinY, float menuMaxX, float menuMaxY, float backdropStrength);
}
