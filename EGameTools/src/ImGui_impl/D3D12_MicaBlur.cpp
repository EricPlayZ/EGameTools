#include <EGT/ImGui_impl/D3D12_MicaBlur.h>
#include <d3dcompiler.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace EGT::ImGui_impl::D3D12_MicaBlur {
    static const char* micaHlslSource = R"(
struct PSIn { float4 pos : SV_POSITION; };

PSIn vs_full(uint vid : SV_VertexID) {
PSIn o;
float2 uv = float2((vid << 1) & 2, vid & 2);
o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
return o;
}

cbuffer CBDown : register(b0) { float2 sceneTextureSize; float2 padding0; }
Texture2D sceneTexture : register(t0);
SamplerState linearSampler : register(s0);

float4 ps_down(PSIn inp) : SV_Target {
float2 p = inp.pos.xy;
float2 suv = (p * 2.0f + float2(0.5f, 0.5f)) / sceneTextureSize;
return sceneTexture.SampleLevel(linearSampler, suv, 0);
}

cbuffer CBBlur : register(b0) { float4 blurPassParams; } // (invW, invH, dirX, dirY) dir = (1,0) horiz or (0,1) vert
Texture2D blurSourceTexture : register(t0);

float4 ps_blur(PSIn inp) : SV_Target {
float2 invSz = blurPassParams.xy;
float2 dir = blurPassParams.zw;
float2 uv = (inp.pos.xy + float2(0.5f, 0.5f)) * invSz;
float2 off = float2(dir.x * invSz.x, dir.y * invSz.y);
float4 c = blurSourceTexture.SampleLevel(linearSampler, uv, 0) * 0.227027f;
c += blurSourceTexture.SampleLevel(linearSampler, uv + off, 0) * 0.316216f;
c += blurSourceTexture.SampleLevel(linearSampler, uv - off, 0) * 0.316216f;
c += blurSourceTexture.SampleLevel(linearSampler, uv + off * 2.0f, 0) * 0.070270f;
c += blurSourceTexture.SampleLevel(linearSampler, uv - off * 2.0f, 0) * 0.070270f;
return c;
}

cbuffer CBComp : register(b0) { float2 backbufferSize; float blurMix; float menuFade; }
Texture2D sharpBackbufferTexture : register(t0);
Texture2D blurredTexture : register(t1);

float4 ps_comp(PSIn inp) : SV_Target {
float2 uv = inp.pos.xy / backbufferSize;
float3 sharp = sharpBackbufferTexture.SampleLevel(linearSampler, uv, 0).rgb;
float3 blur = blurredTexture.SampleLevel(linearSampler, uv, 0).rgb;
float t = saturate(blurMix * menuFade);
return float4(lerp(sharp, blur, t), 1.0f);
}
    )";

    struct GpuSize {
        UINT width = 0;
        UINT height = 0;
        UINT blurWidth = 0;
        UINT blurHeight = 0;
    };

    static ID3D12Device* micaD3dDevice = nullptr;
    static ID3D12RootSignature* rootSignatureDownsample = nullptr;
    static ID3D12RootSignature* rootSignatureBlur = nullptr;
    static ID3D12RootSignature* rootSignatureComposite = nullptr;
    static ID3D12PipelineState* pipelineStateDownsample = nullptr;
    static ID3D12PipelineState* pipelineStateBlur = nullptr;
    static ID3D12PipelineState* pipelineStateComposite = nullptr;

    static ID3D12Resource* sceneCopyTexture = nullptr;
    static ID3D12Resource* blurTextureA = nullptr;
    static ID3D12Resource* blurTextureB = nullptr;

    static ID3D12DescriptorHeap* srvHeap = nullptr;
    static ID3D12DescriptorHeap* rtvHeap = nullptr;
    static UINT srvDescriptorStride = 0;
    static UINT rtvDescriptorStride = 0;
    static D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvScene{};
    static D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvBlurA{};
    static D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvBlurB{};
    static D3D12_CPU_DESCRIPTOR_HANDLE cpuRtvA{};
    static D3D12_CPU_DESCRIPTOR_HANDLE cpuRtvB{};
    static D3D12_GPU_DESCRIPTOR_HANDLE gpuSrvScene{};
    static D3D12_GPU_DESCRIPTOR_HANDLE gpuSrvBlurA{};
    static D3D12_GPU_DESCRIPTOR_HANDLE gpuSrvBlurB{};

    static GpuSize micaTextureSizes{};
    static bool shadersCompiledOk = false;
    static D3D12_RESOURCE_STATES sceneCopyResourceState = D3D12_RESOURCE_STATE_COMMON;
    static D3D12_RESOURCE_STATES blurTextureAResourceState = D3D12_RESOURCE_STATE_COMMON;
    static D3D12_RESOURCE_STATES blurTextureBResourceState = D3D12_RESOURCE_STATE_COMMON;

    template <typename T>
    static void SafeRelease(T*& p) {
        if (p) {
            p->Release();
            p = nullptr;
        }
    }

    static void ReleaseShaderBlobs(ID3DBlob*& vertexShaderBlob, ID3DBlob*& downsamplePixelBlob, ID3DBlob*& blurPixelBlob, ID3DBlob*& compositePixelBlob) {
        SafeRelease(vertexShaderBlob);
        SafeRelease(downsamplePixelBlob);
        SafeRelease(blurPixelBlob);
        SafeRelease(compositePixelBlob);
    }

    static void ReleasePipelineAndSignatureObjects() {
        SafeRelease(pipelineStateDownsample);
        SafeRelease(pipelineStateBlur);
        SafeRelease(pipelineStateComposite);
        SafeRelease(rootSignatureDownsample);
        SafeRelease(rootSignatureBlur);
        SafeRelease(rootSignatureComposite);
    }

    static bool Compile(const char* entry, const char* target, ID3DBlob** out) {
        ID3DBlob* err = nullptr;
        const HRESULT hr = D3DCompile(micaHlslSource, std::strlen(micaHlslSource), nullptr, nullptr, nullptr, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, out, &err);
        if (FAILED(hr)) {
            if (err) {
                SPDLOG_ERROR("D3D12 Mica: D3DCompile failed ({}): {}", entry, static_cast<const char*>(err->GetBufferPointer()));
                err->Release();
            } else
                SPDLOG_ERROR("D3D12 Mica: D3DCompile failed ({}) HRESULT={:#x}", entry, static_cast<unsigned>(hr));
            return false;
        }
        return true;
    }

    static bool CreateRs1Srv(ID3D12Device* dev, ID3D12RootSignature** outRs) {
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER param[2]{};
        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param[0].Constants.ShaderRegister = 0;
        param[0].Constants.RegisterSpace = 0;
        param[0].Constants.Num32BitValues = 4;
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges = &range;
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = 2;
        desc.pParameters = param;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &samp;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ID3DBlob* ser = nullptr;
        if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &ser, nullptr)))
            return false;
        const HRESULT hr = dev->CreateRootSignature(0, ser->GetBufferPointer(), ser->GetBufferSize(), IID_PPV_ARGS(outRs));
        ser->Release();
        return SUCCEEDED(hr);
    }

    static bool CreateRs2Srv(ID3D12Device* dev, ID3D12RootSignature** outRs) {
        D3D12_DESCRIPTOR_RANGE ranges[2]{};
        for (int i = 0; i < 2; ++i) {
            ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges[i].NumDescriptors = 1;
            ranges[i].BaseShaderRegister = static_cast<UINT>(i);
            ranges[i].RegisterSpace = 0;
            ranges[i].OffsetInDescriptorsFromTableStart = (i == 0) ? 0 : D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }

        D3D12_ROOT_PARAMETER param[2]{};
        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param[0].Constants.ShaderRegister = 0;
        param[0].Constants.RegisterSpace = 0;
        param[0].Constants.Num32BitValues = 4;
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 2;
        param[1].DescriptorTable.pDescriptorRanges = ranges;
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samp.MaxLOD = D3D12_FLOAT32_MAX;
        samp.ShaderRegister = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = 2;
        desc.pParameters = param;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &samp;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ID3DBlob* ser = nullptr;
        if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &ser, nullptr)))
            return false;
        const HRESULT hr = dev->CreateRootSignature(0, ser->GetBufferPointer(), ser->GetBufferSize(), IID_PPV_ARGS(outRs));
        ser->Release();
        return SUCCEEDED(hr);
    }

    static bool CreatePso(ID3D12Device* dev, ID3D12RootSignature* rs, ID3DBlob* vs, ID3DBlob* ps, ID3D12PipelineState** outPso) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = rs;
        pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.SampleMask = UINT_MAX;
        pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pso.RasterizerState.DepthClipEnable = TRUE;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.SampleDesc.Count = 1;
        pso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        return SUCCEEDED(dev->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(outPso)));
    }

    static void Transition(ID3D12GraphicsCommandList* cmd, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = res;
        b.Transition.StateBefore = before;
        b.Transition.StateAfter = after;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
    }

    static void CreateSrvRtvForTexture(ID3D12Device* dev, ID3D12Resource* tex, D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, D3D12_CPU_DESCRIPTOR_HANDLE rtvCpu, DXGI_FORMAT fmt) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Format = fmt;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels = 1;
        srv.Texture2D.MostDetailedMip = 0;
        srv.Texture2D.PlaneSlice = 0;
        srv.Texture2D.ResourceMinLODClamp = 0.0f;
        dev->CreateShaderResourceView(tex, &srv, srvCpu);

        D3D12_RENDER_TARGET_VIEW_DESC rtv{};
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv.Format = fmt;
        rtv.Texture2D.MipSlice = 0;
        rtv.Texture2D.PlaneSlice = 0;
        dev->CreateRenderTargetView(tex, &rtv, rtvCpu);
    }
    
    bool EnsureInitialized(ID3D12Device* incomingDevice) {
        if (!incomingDevice)
            return false;
        if (shadersCompiledOk)
            return true;
        micaD3dDevice = incomingDevice;

        // If a previous initialization attempt failed part-way, clear leftovers
        // before trying again so we never overwrite and leak COM pointers.
        ReleasePipelineAndSignatureObjects();

        ID3DBlob* vsBlob = nullptr;
        if (!Compile("vs_full", "vs_5_0", &vsBlob))
            return false;

        ID3DBlob *psDown = nullptr, *psBlur = nullptr, *psComp = nullptr;
        if (!Compile("ps_down", "ps_5_0", &psDown) || !Compile("ps_blur", "ps_5_0", &psBlur) || !Compile("ps_comp", "ps_5_0", &psComp)) {
            ReleaseShaderBlobs(vsBlob, psDown, psBlur, psComp);
            return false;
        }

        if (!CreateRs1Srv(incomingDevice, &rootSignatureDownsample) || !CreateRs1Srv(incomingDevice, &rootSignatureBlur) || !CreateRs2Srv(incomingDevice, &rootSignatureComposite)) {
            ReleaseShaderBlobs(vsBlob, psDown, psBlur, psComp);
            SafeRelease(rootSignatureDownsample);
            SafeRelease(rootSignatureBlur);
            SafeRelease(rootSignatureComposite);
            return false;
        }

        if (!CreatePso(incomingDevice, rootSignatureDownsample, vsBlob, psDown, &pipelineStateDownsample) || !CreatePso(incomingDevice, rootSignatureBlur, vsBlob, psBlur, &pipelineStateBlur) || !CreatePso(incomingDevice, rootSignatureComposite, vsBlob, psComp, &pipelineStateComposite)) {
            ReleaseShaderBlobs(vsBlob, psDown, psBlur, psComp);
            ReleasePipelineAndSignatureObjects();
            return false;
        }

        ReleaseShaderBlobs(vsBlob, psDown, psBlur, psComp);

        shadersCompiledOk = true;
        SPDLOG_INFO("D3D12 Mica: shaders and PSOs initialized");
        return true;
    }

    /// Scene copy + blur textures and their SRV/RTV heaps only. PSOs/root signatures stay valid across resize.
    static void ReleaseMicaTextureResources() {
        SafeRelease(sceneCopyTexture);
        SafeRelease(blurTextureA);
        SafeRelease(blurTextureB);
        SafeRelease(srvHeap);
        SafeRelease(rtvHeap);
        micaTextureSizes = {};
        cpuSrvScene = {};
        cpuSrvBlurA = {};
        cpuSrvBlurB = {};
        cpuRtvA = {};
        cpuRtvB = {};
        gpuSrvScene = {};
        gpuSrvBlurA = {};
        gpuSrvBlurB = {};
        sceneCopyResourceState = D3D12_RESOURCE_STATE_COMMON;
        blurTextureAResourceState = D3D12_RESOURCE_STATE_COMMON;
        blurTextureBResourceState = D3D12_RESOURCE_STATE_COMMON;
    }

    void ReleaseGpuResources() {
        ReleasePipelineAndSignatureObjects();
        ReleaseMicaTextureResources();
        shadersCompiledOk = false;
    }

    static bool EnsureTextures(ID3D12Device* dev, UINT widthPx, UINT heightPx) {
        if (widthPx == 0 || heightPx == 0)
            return false;
        const UINT blurWidthPx = (widthPx + 1) / 2;
        const UINT blurHeightPx = (heightPx + 1) / 2;
        if (sceneCopyTexture && micaTextureSizes.width == widthPx && micaTextureSizes.height == heightPx)
            return true;

        // Do not call ReleaseGpuResources() here: that drops PSOs/root signatures and clears shadersCompiledOk,
        // but this path only rebuilds size-dependent resources. Pipelines are independent of texture dimensions.
        ReleaseMicaTextureResources();

        D3D12_HEAP_PROPERTIES heapDefault{};
        heapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapDefault.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDefault.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDefault.CreationNodeMask = 1;
        heapDefault.VisibleNodeMask = 1;

        auto makeBlurTex = [&](UINT textureWidth, UINT textureHeight, ID3D12Resource** out) {
            D3D12_RESOURCE_DESC td{};
            td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            td.Width = textureWidth;
            td.Height = textureHeight;
            td.DepthOrArraySize = 1;
            td.MipLevels = 1;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            td.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            return SUCCEEDED(dev->CreateCommittedResource(&heapDefault, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(out)));
        };

        D3D12_RESOURCE_DESC tdScene{};
        tdScene.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tdScene.Width = widthPx;
        tdScene.Height = heightPx;
        tdScene.DepthOrArraySize = 1;
        tdScene.MipLevels = 1;
        tdScene.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tdScene.SampleDesc.Count = 1;
        tdScene.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tdScene.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (FAILED(dev->CreateCommittedResource(&heapDefault, D3D12_HEAP_FLAG_NONE, &tdScene, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&sceneCopyTexture))))
            return false;

        if (!makeBlurTex(blurWidthPx, blurHeightPx, &blurTextureA))
            return false;
        if (!makeBlurTex(blurWidthPx, blurHeightPx, &blurTextureB))
            return false;

        srvDescriptorStride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        rtvDescriptorStride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 4;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvDesc.NodeMask = 1;
        if (FAILED(dev->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&srvHeap))))
            return false;

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = 2;
        rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvDesc.NodeMask = 1;
        if (FAILED(dev->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap))))
            return false;

        cpuSrvScene = srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuSrvBlurA = cpuSrvScene;
        cpuSrvBlurA.ptr += static_cast<SIZE_T>(srvDescriptorStride);
        cpuSrvBlurB = cpuSrvBlurA;
        cpuSrvBlurB.ptr += static_cast<SIZE_T>(srvDescriptorStride);

        gpuSrvScene = srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuSrvBlurA = gpuSrvScene;
        gpuSrvBlurA.ptr += static_cast<UINT64>(srvDescriptorStride);
        gpuSrvBlurB = gpuSrvBlurA;
        gpuSrvBlurB.ptr += static_cast<UINT64>(srvDescriptorStride);

        cpuRtvA = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuRtvB = cpuRtvA;
        cpuRtvB.ptr += static_cast<SIZE_T>(rtvDescriptorStride);

        CreateSrvRtvForTexture(dev, blurTextureA, cpuSrvBlurA, cpuRtvA, DXGI_FORMAT_R8G8B8A8_UNORM);
        CreateSrvRtvForTexture(dev, blurTextureB, cpuSrvBlurB, cpuRtvB, DXGI_FORMAT_R8G8B8A8_UNORM);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvScene{};
        srvScene.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvScene.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvScene.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvScene.Texture2D.MipLevels = 1;
        dev->CreateShaderResourceView(sceneCopyTexture, &srvScene, cpuSrvScene);

        micaTextureSizes.width = widthPx;
        micaTextureSizes.height = heightPx;
        micaTextureSizes.blurWidth = blurWidthPx;
        micaTextureSizes.blurHeight = blurHeightPx;
        return true;
    }

    void RenderBackdrop(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer, const D3D12_CPU_DESCRIPTOR_HANDLE& backBufferRtv, UINT fullWidth, UINT fullHeight, float menuMinX, float menuMinY,
        float menuMaxX, float menuMaxY, float backdropStrength) {
        if (!shadersCompiledOk || !cmdList || !backBuffer || fullWidth == 0 || fullHeight == 0)
            return;
        if (backdropStrength < 0.02f)
            return;
        if (menuMaxX <= menuMinX + 2.0f || menuMaxY <= menuMinY + 2.0f)
            return;
        ID3D12Device* dev = micaD3dDevice;
        if (!dev)
            return;
        if (!EnsureTextures(dev, fullWidth, fullHeight))
            return;

        const float mx0 = std::max(0.0f, menuMinX);
        const float my0 = std::max(0.0f, menuMinY);
        const float mx1 = std::min(static_cast<float>(fullWidth), menuMaxX);
        const float my1 = std::min(static_cast<float>(fullHeight), menuMaxY);
        if (mx1 <= mx0 + 1.0f || my1 <= my0 + 1.0f)
            return;

        const LONG scL = static_cast<LONG>(mx0);
        const LONG scT = static_cast<LONG>(my0);
        const LONG scR = static_cast<LONG>(std::ceil(mx1));
        const LONG scB = static_cast<LONG>(std::ceil(my1));

        Transition(cmdList, sceneCopyTexture, sceneCopyResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
        sceneCopyResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
        Transition(cmdList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmdList->CopyResource(sceneCopyTexture, backBuffer);
        Transition(cmdList, backBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Transition(cmdList, sceneCopyTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        sceneCopyResourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        Transition(cmdList, blurTextureA, blurTextureAResourceState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        blurTextureAResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;

        D3D12_VIEWPORT vpBlur{};
        vpBlur.Width = static_cast<float>(micaTextureSizes.blurWidth);
        vpBlur.Height = static_cast<float>(micaTextureSizes.blurHeight);
        vpBlur.MinDepth = 0.0f;
        vpBlur.MaxDepth = 1.0f;
        D3D12_RECT rectBlur{ 0, 0, static_cast<LONG>(micaTextureSizes.blurWidth), static_cast<LONG>(micaTextureSizes.blurHeight) };

        const float clearC[4] = { 0, 0, 0, 0 };
        cmdList->RSSetViewports(1, &vpBlur);
        cmdList->RSSetScissorRects(1, &rectBlur);
        cmdList->OMSetRenderTargets(1, &cpuRtvA, FALSE, nullptr);
        cmdList->ClearRenderTargetView(cpuRtvA, clearC, 0, nullptr);

        ID3D12DescriptorHeap* heaps[] = { srvHeap };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootSignature(rootSignatureDownsample);
        cmdList->SetPipelineState(pipelineStateDownsample);
        float downC[4] = { static_cast<float>(fullWidth), static_cast<float>(fullHeight), 0, 0 };
        cmdList->SetGraphicsRoot32BitConstants(0, 4, downC, 0);
        cmdList->SetGraphicsRootDescriptorTable(1, gpuSrvScene);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        Transition(cmdList, blurTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        blurTextureAResourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        const float invW = 1.0f / static_cast<float>(micaTextureSizes.blurWidth);
        const float invH = 1.0f / static_cast<float>(micaTextureSizes.blurHeight);

        auto blurPass = [&](D3D12_GPU_DESCRIPTOR_HANDLE srcGpuSrv, D3D12_CPU_DESCRIPTOR_HANDLE dstRtv, ID3D12Resource* dstRes, D3D12_RESOURCE_STATES& dstState, float dirX, float dirY) {
            Transition(cmdList, dstRes, dstState, D3D12_RESOURCE_STATE_RENDER_TARGET);
            dstState = D3D12_RESOURCE_STATE_RENDER_TARGET;
            cmdList->OMSetRenderTargets(1, &dstRtv, FALSE, nullptr);
            cmdList->ClearRenderTargetView(dstRtv, clearC, 0, nullptr);
            cmdList->SetGraphicsRootSignature(rootSignatureBlur);
            cmdList->SetPipelineState(pipelineStateBlur);
            float bp[4] = { invW, invH, dirX, dirY };
            cmdList->SetGraphicsRoot32BitConstants(0, 4, bp, 0);
            cmdList->SetGraphicsRootDescriptorTable(1, srcGpuSrv);
            cmdList->DrawInstanced(3, 1, 0, 0);
            Transition(cmdList, dstRes, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            dstState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        };

        blurPass(gpuSrvBlurA, cpuRtvB, blurTextureB, blurTextureBResourceState, 1.0f, 0.0f);
        blurPass(gpuSrvBlurB, cpuRtvA, blurTextureA, blurTextureAResourceState, 0.0f, 1.0f);
        blurPass(gpuSrvBlurA, cpuRtvB, blurTextureB, blurTextureBResourceState, 1.0f, 0.0f);
        blurPass(gpuSrvBlurB, cpuRtvA, blurTextureA, blurTextureAResourceState, 0.0f, 1.0f);

        D3D12_VIEWPORT vpFull{};
        vpFull.Width = static_cast<float>(fullWidth);
        vpFull.Height = static_cast<float>(fullHeight);
        vpFull.MinDepth = 0.0f;
        vpFull.MaxDepth = 1.0f;
        D3D12_RECT scMenu{ scL, scT, scR, scB };

        cmdList->RSSetViewports(1, &vpFull);
        cmdList->RSSetScissorRects(1, &scMenu);
        cmdList->OMSetRenderTargets(1, &backBufferRtv, FALSE, nullptr);

        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootSignature(rootSignatureComposite);
        cmdList->SetPipelineState(pipelineStateComposite);
        float compC[4] = { static_cast<float>(fullWidth), static_cast<float>(fullHeight), 0.92f, backdropStrength };
        cmdList->SetGraphicsRoot32BitConstants(0, 4, compC, 0);
        cmdList->SetGraphicsRootDescriptorTable(1, gpuSrvScene);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}