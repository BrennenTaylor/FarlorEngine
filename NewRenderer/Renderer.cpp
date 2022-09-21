#include "Renderer.h"

#include "DebugNameUtil.h"

#include <DirectXMath.h>

#include "FMath/Vector2.h"
#include "Mesh.h"
#include "Camera.h"

#include "../Core/Engine.h"

#include "../ECS/TransformComponent.h"
#include "../ECS/TransformManager.h"

#include "../Util/Logger.h"

#include "FMath/Vector3.h"
#include "FMath/Vector4.h"


#include <ImfArray.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfStringAttribute.h>

#include "agz/smaa/smaa.h"

#include "Desertscape/basics.h"
#include "Desertscape/noise.h"

#include <d3dcommon.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxgiformat.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <WICTextureLoader.h>

#include <d3dcompiler.h>

#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>
#include <chrono>
#include <corecrt_math_defines.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>

const uint32_t HeightmapGridResolution = 1024;

namespace Farlor {

extern TransformManager g_TransformManager;

std::map<std::string, RenderResourceType> Renderer::s_stringToTypeMapping
      = { { "render-target", RenderResourceType::RenderTarget } };

std::map<std::string, RenderResourceFormat> Renderer::s_stringToFormatMapping
      = { { "R8G8B8A8", RenderResourceFormat::R8G8B8A8 }, { "D24S8", RenderResourceFormat::D24S8 },
            { "R32G32B32A32F", RenderResourceFormat::R32G32B32A32F } };

std::map<RenderResourceFormat, DXGI_FORMAT> Renderer::s_formatToDXGIFormatMapping {
    { RenderResourceFormat::R8G8B8A8, DXGI_FORMAT_R8G8B8A8_UNORM },
    { RenderResourceFormat::D24S8, DXGI_FORMAT_D24_UNORM_S8_UINT },
    { RenderResourceFormat::R32G32B32A32F, DXGI_FORMAT_R32G32B32A32_FLOAT }
};

Renderer::Renderer(const Engine &engine)
    : m_engine(engine)
    , m_luminanceTexture("LuminanceTexture", 1024, 1024, DXGI_FORMAT_R32G32_FLOAT)
    , m_largeScaleDuneModel(*this, 1024, 1.0f, "LargeScaleDuneModel")
    , m_smallScaleRippleModel(*this, 1024, 8.0f / 1024.0f, "SmallScaleRippleModel")
{
}

#if defined(_DEBUG)
// Check for SDK Layer support.
inline bool SdkLayersAvailable() noexcept
{
    HRESULT hr = D3D11CreateDevice(nullptr,
          D3D_DRIVER_TYPE_NULL,  // There is no need to create a real hardware
                                 // device.
          nullptr,
          D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
          nullptr,                    // Any feature level will do.
          0, D3D11_SDK_VERSION,
          nullptr,  // No need to keep the D3D device reference.
          nullptr,  // No need to know the feature level.
          nullptr   // No need to keep the D3D device context reference.
    );

    return SUCCEEDED(hr);
}
#endif

// NOTE: Cant actually execute shaders loaded by config file atm
// TODO: Revisit this issue
void Renderer::Initialize(const GameWindow &gameWindow)
{
    HRESULT result = ERROR_SUCCESS;

    m_width = gameWindow.GetWidth();
    m_height = gameWindow.GetHeight();

    UINT deviceCreationFlags = 0;
#if defined(_DEBUG)
    if (SdkLayersAvailable()) {
        deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    } else {
        FARLOR_LOG_WARNING("D3D11 Debug Device not avalible");
    }
#endif

    std::vector<IDXGIAdapter *> adapters;
    InitializeDXGI(adapters);

    Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext = nullptr;

    // Create the swap chain
    result = D3D11CreateDevice(adapters[0], D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceCreationFlags,
          0, 0, D3D11_SDK_VERSION, device.GetAddressOf(), 0, deviceContext.GetAddressOf());
    assert(device && "Failed to create basic d3d11 device");
    assert(deviceContext && "Failed to create basic d3d11 device context");

    // We require the device 1/device context 1 interfaces
    result = device.As(&m_pDevice);
    result = deviceContext.As(&m_pDeviceContext);
    assert(m_pDevice && "Failed to get a D3D11Device1 interface");
    assert(m_pDeviceContext && "Failed to get a D3D11DeviceContext1 interface");

    // Enable the debug layer
    Microsoft::WRL::ComPtr<ID3D11Debug> m_d3d11Debug = nullptr;
    if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(m_d3d11Debug.GetAddressOf())))) {
        if (SUCCEEDED(
                  m_d3d11Debug->QueryInterface(IID_PPV_ARGS(m_pD3D11InfoQueue.GetAddressOf())))) {
            m_pD3D11InfoQueue->PushEmptyRetrievalFilter();
            m_pD3D11InfoQueue->PushEmptyStorageFilter();

#ifdef _DEBUG
            m_pD3D11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            m_pD3D11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif

            D3D11_MESSAGE_ID hide[] = { D3D11_MESSAGE_ID_DEVICE_DRAW_VERTEX_BUFFER_NOT_SET };

            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            m_pD3D11InfoQueue->AddStorageFilterEntries(&filter);
            m_pD3D11InfoQueue->Release();
        }
    }

    // Obtain the perf marker interface
    // TODO: Allow this to be toggleable
    m_pDeviceContext->QueryInterface(IID_PPV_ARGS(m_pPerf.GetAddressOf()));
    assert(m_pPerf && "Failed to query perf interface");

    // Describe swap chain
    DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
    ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
    dxgiSwapChainDesc.Width = m_width;
    dxgiSwapChainDesc.Height = m_height;
    dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.BufferCount = 2;  // Must be at least 2 for flip model of presenting
    dxgiSwapChainDesc.SampleDesc.Count = 1;
    dxgiSwapChainDesc.SampleDesc.Quality = 0;
    dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc;
    ZeroMemory(&swapChainFullscreenDesc, sizeof(swapChainFullscreenDesc));
    swapChainFullscreenDesc.Windowed = true;

    m_pDxgiFactory->CreateSwapChainForHwnd(m_pDevice.Get(), gameWindow.GetWindowHandle(),
          &dxgiSwapChainDesc, &swapChainFullscreenDesc, nullptr, m_pSwapChain.GetAddressOf());
    assert(m_pSwapChain && "Failed to create swap chain");

    // Create back buffer
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(m_pBackBuffer.GetAddressOf()));
    assert(m_pBackBuffer && "Failed to create back buffer from swap chain");
    SetDebugObjectName(m_pBackBuffer.Get(), "Back Buffer Texture");

    // We want the view to be srgb, although the texture is rgb.
    // This is to support the flip mode
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    ZeroMemory(&rtvDesc, sizeof(rtvDesc));
    // We want our back buffer rtv to have an SRGB format so gamma correction is
    // handled for us
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    result = m_pDevice->CreateRenderTargetView(
          m_pBackBuffer.Get(), &rtvDesc, m_pBackBufferRTV.GetAddressOf());
    assert(m_pBackBufferRTV && "Failed to create back buffer RTV");
    SetDebugObjectName(m_pBackBufferRTV.Get(), "Back Buffer RTV");

    // Describe depth stencil buffer
    // Dimensions need to be the size of the window
    {
        D3D11_TEXTURE2D_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
        depthStencilDesc.Width = m_width;
        depthStencilDesc.Height = m_height;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        ZeroMemory(&dsvDesc, sizeof(dsvDesc));
        dsvDesc.Flags = 0;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        D3D11_SHADER_RESOURCE_VIEW_DESC dssrvDesc;
        ZeroMemory(&dssrvDesc, sizeof(dssrvDesc));
        dssrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        dssrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        dssrvDesc.Texture2D.MipLevels = 1;

        m_pDevice->CreateTexture2D(&depthStencilDesc, NULL, &m_pWindowDSB);
        assert(m_pWindowDSB && "Failed to create window depth stencil texture");
        SetDebugObjectName(m_pWindowDSB.Get(), "Window Depth Stencil Buffer");

        result = m_pDevice->CreateDepthStencilView(m_pWindowDSB.Get(), &dsvDesc, &m_pWindowDSV);
        assert(m_pWindowDSV && "Failed to create window depth stencil view");
        SetDebugObjectName(m_pWindowDSV.Get(), "Window Depth Stencil View");
    }

    // Enable Depth stencil state creation
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        ZeroMemory(&dsDesc, sizeof(dsDesc));
        // Depth test parameters
        dsDesc.DepthEnable = true;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        // Stencil test parameters
        dsDesc.StencilEnable = false;
        dsDesc.StencilReadMask = 0xFF;
        dsDesc.StencilWriteMask = 0xFF;
        // Stencil operations if pixel is front-facing
        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        // Stencil operations if pixel is back-facing
        dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Create depth stencil state
        result = m_pDevice->CreateDepthStencilState(
              &dsDesc, m_pEnableDepthStencilState.GetAddressOf());
        assert(m_pEnableDepthStencilState && "Failed to create enabled depth stencil state");
        SetDebugObjectName(m_pEnableDepthStencilState.Get(), "Enabled Depth And Stencil State");
    }

    // Disable Depth stencil state creation
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        ZeroMemory(&dsDesc, sizeof(dsDesc));
        // Depth test parameters
        dsDesc.DepthEnable = false;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        // Stencil test parameters
        dsDesc.StencilEnable = false;
        dsDesc.StencilReadMask = 0xFF;
        dsDesc.StencilWriteMask = 0xFF;
        // Stencil operations if pixel is front-facing
        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        // Stencil operations if pixel is back-facing
        dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Create depth stencil state
        result = m_pDevice->CreateDepthStencilState(
              &dsDesc, m_pDisableDepthStencilState.GetAddressOf());
        assert(m_pDisableDepthStencilState && "Failed to create disabled depth stencil state");
        SetDebugObjectName(m_pDisableDepthStencilState.Get(), "Disabled Depth And Stencil State");
    }

    // Set raster desc
    {
        RasterizerStateDesc rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(rasterDesc));
        rasterDesc.m_rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.m_rasterDesc.FillMode = D3D11_FILL_SOLID;
        result = m_pDevice->CreateRasterizerState(
              &rasterDesc.m_rasterDesc, m_pCullRS.GetAddressOf());
        assert(m_pCullRS && "Failed to create raster state: D3D11_CULL_BACK, D3D11_FILL_SOLID");

        SetDebugObjectName(m_pCullRS.Get(), "Backface Cull and Solid Fill");
    }

    {
        RasterizerStateDesc rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(rasterDesc));
        rasterDesc.m_rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.m_rasterDesc.FillMode = D3D11_FILL_SOLID;
        result = m_pDevice->CreateRasterizerState(
              &rasterDesc.m_rasterDesc, m_pNoCullRS.GetAddressOf());
        assert(m_pNoCullRS && "No Cull and Solid Fill");
        SetDebugObjectName(m_pNoCullRS.Get(), "No Cull and Solid Fill");
    }

    {
        RasterizerStateDesc rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(rasterDesc));
        rasterDesc.m_rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.m_rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        result = m_pDevice->CreateRasterizerState(
              &rasterDesc.m_rasterDesc, m_pWireframeRS.GetAddressOf());
        assert(m_pWireframeRS
              && "Failed to create raster state: D3D11_CULL_NONE, D3D11_FILL_WIREFRAME");

        SetDebugObjectName(m_pWireframeRS.Get(), "Wireframe RS");
    }

    ZeroMemory(&m_luminanceViewport, sizeof(m_luminanceViewport));
    m_luminanceViewport.TopLeftX = 0.0f;
    m_luminanceViewport.TopLeftY = 0.0f;
    m_luminanceViewport.Width = (float)1024;
    m_luminanceViewport.Height = (float)1024;
    m_luminanceViewport.MinDepth = 0.0f;
    m_luminanceViewport.MaxDepth = 1.0f;

    // Create transform constant buffer
    m_transformParams.Initialize(m_pDevice.Get());
    // Create material properties constant buffer
    m_materialParams.Initialize(m_pDevice.Get());
    // Create light properties constant buffer
    m_lightParams.Initialize(m_pDevice.Get());
    // Create camera params constant buffer
    m_cameraParams.Initialize(m_pDevice.Get());

    // Create sampler state
    {
        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory(&samplerDesc, sizeof(samplerDesc));
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 0;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        m_pDevice->CreateSamplerState(&samplerDesc, m_pWrapSamplerState.GetAddressOf());
        assert(m_pWrapSamplerState
              && "Failed to sampler state: D3D11_FILTER_MIN_MAG_MIP_LINEAR, "
                 "D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_ALWAYS");
        SetDebugObjectName(m_pWrapSamplerState.Get(), "Wrap Texture Sampler");
    }

    // Create clamp sampler state
    {
        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory(&samplerDesc, sizeof(samplerDesc));
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 0;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        m_pDevice->CreateSamplerState(&samplerDesc, m_pClampSamplerState.GetAddressOf());
        assert(m_pClampSamplerState
              && "Failed to create sampler state: D3D11_FILTER_MIN_MAG_MIP_LINEAR, "
                 "D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_COMPARISON_ALWAYS");
        SetDebugObjectName(m_pClampSamplerState.Get(), "Clamp Texture Sampler");
    }

    // Create blend states
    {
        D3D11_BLEND_DESC additiveBlendDesc;
        ZeroMemory(&additiveBlendDesc, sizeof(additiveBlendDesc));
        additiveBlendDesc.RenderTarget[0].BlendEnable = true;
        additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        m_pDevice->CreateBlendState(&additiveBlendDesc, m_pAdditiveBlendState.GetAddressOf());
        assert(m_pAdditiveBlendState && "Failed to create additive blend state");
        SetDebugObjectName(m_pAdditiveBlendState.Get(), "Additive Blend State");
    }

    // Create blend states
    {
        D3D11_BLEND_DESC noBlendDesc;
        ZeroMemory(&noBlendDesc, sizeof(noBlendDesc));
        noBlendDesc.RenderTarget[0].BlendEnable = false;
        noBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        result = m_pDevice->CreateBlendState(&noBlendDesc, m_pNoBlendState.GetAddressOf());
        assert(m_pNoBlendState && "Failed to create no blend state");
        SetDebugObjectName(m_pNoBlendState.Get(), "Disable Blend State");
    }

    m_luminanceTexture.Initialize(m_pDevice.Get());

    // Initialize IMGUI library
    ImGui_ImplWin32_Init(gameWindow.GetWindowHandle());
    ImGui_ImplDX11_Init(m_pDevice.Get(), m_pDeviceContext.Get());

    SetupSMAAResources();

    // Load up bedrock terrain texture
    {
        std::ostringstream name;
        name << "./resources/terrain_heightmaps/";
        name << "alien_seamless_heightmap";
        name << "_" << m_largeScaleDuneModel.GetGridResolution();
        name << "_" << m_largeScaleDuneModel.GetGridResolution();
        name << ".exr";
        Imf::RgbaInputFile bedrockTerrainTextureFile(name.str().c_str());
        Imath::Box2i dw = bedrockTerrainTextureFile.dataWindow();

        Imf::Array2D<Imf::Rgba> bedrock_pixels;
        uint32_t width = dw.max.x - dw.min.x + 1;
        uint32_t height = dw.max.y - dw.min.y + 1;

        std::cout << "Width, Height: (" << width << ", " << height << ")" << std::endl;

        bedrock_pixels.resizeErase(HeightmapGridResolution, HeightmapGridResolution);

        if (bedrockTerrainTextureFile.header().findTypedAttribute<Imf::StringAttribute>(
                  "comments")) {
            std::cout << "Bedrock exr comments: "
                      << bedrockTerrainTextureFile.header()
                               .findTypedAttribute<Imf::StringAttribute>("comments")
                               ->value()
                      << std::endl;
        } else {
            std::cout << "No comments" << std::endl;
        }

        bedrockTerrainTextureFile.setFrameBuffer(
              &bedrock_pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
        bedrockTerrainTextureFile.readPixels(dw.min.y, dw.max.y);
        SetupDesertSimulation(1.0f, 1.0f, bedrock_pixels);
    }


    // Load up sand material textures
    {
        // Albedo should be srgb
        result = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/sand1-bl/sand1-albedo.png", 0, D3D11_USAGE_DEFAULT,
              D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0,
              D3D11_RESOURCE_MISC_GENERATE_MIPS, DirectX::WIC_LOADER_FORCE_SRGB,
              m_pSandAlbedoTexture.GetAddressOf(), m_pSandAlbedoTextureSRV.GetAddressOf());
        assert(m_pSandAlbedoTexture && "Failed to load sand albedo texture");
        assert(m_pSandAlbedoTextureSRV && "Failed to create sand albedo texture SRV");
    }

    {
        // Normals shouldnt have srgb
        result = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/sand1-bl/sand1-normal-ogl.png", 0, D3D11_USAGE_DEFAULT,
              D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0,
              D3D11_RESOURCE_MISC_GENERATE_MIPS, DirectX::WIC_LOADER_IGNORE_SRGB,
              m_pSandNormalTexture.GetAddressOf(), m_pSandNormalTextureSRV.GetAddressOf());
        assert(m_pSandNormalTexture && "Failed to load sand normal texture");
        assert(m_pSandNormalTextureSRV && "Failed to create sand normal texture SRV");
    }

    {
        // TODO: Change these to enforce linear color load
        result = DirectX::CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/sand1-bl/sand1-metalness.png",
              m_pSandMetalnessTexture.GetAddressOf(), m_pSandMetalnessTextureSRV.GetAddressOf(),
              1024 * 2);
        assert(m_pSandMetalnessTexture && "Failed to load sand metalness texture");
        assert(m_pSandMetalnessTextureSRV && "Failed to create sand metalness texture SRV");
    }

    {
        // TODO: Change these to enforce linear color load
        result = DirectX::CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/sand1-bl/sand1-roughness.png",
              m_pSandRoughnessTexture.GetAddressOf(), m_pSandRoughnessTextureSRV.GetAddressOf(),
              1024 * 2);
        assert(m_pSandRoughnessTexture && "Failed to load sand roughness texture");
        assert(m_pSandRoughnessTextureSRV && "Failed to create sand roughness texture SRV");
    }

    // Load up desert rock material textures
    {
        // Force SRGB for albedo load
        result = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/desert-rocks1-bl/desert-rocks1-albedo.png", 0,
              D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0,
              D3D11_RESOURCE_MISC_GENERATE_MIPS, DirectX::WIC_LOADER_FORCE_SRGB,
              m_pDesertRockAlbedoTexture.GetAddressOf(),
              m_pDesertRockAlbedoTextureSRV.GetAddressOf());
        assert(m_pSandRoughnessTexture && "Failed to load desert rock albedo texture");
        assert(m_pSandRoughnessTextureSRV && "Failed to create desert rock albedo texture SRV");
    }

    {
        // Force linear for normal map
        result = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/desert-rocks1-bl/desert-rocks1-normal-ogl.png", 0,
              D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0,
              D3D11_RESOURCE_MISC_GENERATE_MIPS, DirectX::WIC_LOADER_DEFAULT,
              m_pDesertRockNormalTexture.GetAddressOf(),
              m_pDesertRockNormalTextureSRV.GetAddressOf());
        assert(m_pDesertRockNormalTexture && "Failed to load desert rock normal texture");
        assert(m_pDesertRockNormalTextureSRV && "Failed to create desert rock normal texture SRV");
    }

    {
        // TODO: Change these to enforce linear color load
        result = DirectX::CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/desert-rocks1-bl/desert-rocks1-Metallic.png",
              m_pDesertRockMetalnessTexture.GetAddressOf(),
              m_pDesertRockMetalnessTextureSRV.GetAddressOf(), 1024 * 2);
        assert(m_pDesertRockMetalnessTexture && "Failed to load desert rock metalness texture");
        assert(m_pDesertRockMetalnessTextureSRV
              && "Failed to create desert rock metalness texture SRV");
    }

    {
        // TODO: Change these to enforce linear color load
        result = DirectX::CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(),
              L"./resources/materials/desert-rocks1-bl/desert-rocks1-Roughness.png",
              m_pDesertRockRoughnessTexture.GetAddressOf(),
              m_pDesertRockRoughnessTextureSRV.GetAddressOf(), 1024 * 2);
        assert(m_pDesertRockRoughnessTexture && "Failed to load desert rock roughness texture");
        assert(m_pDesertRockRoughnessTextureSRV
              && "Failed to create desert rock roughness texture SRV");
    }


    // Sand Height Field stuff
    m_cbSandHeightfield.Initialize(m_pDevice.Get());
    m_cbHullTerrainLOD.Initialize(m_pDevice.Get());
    m_cbDomainTerrainSettings.Initialize(m_pDevice.Get());

    m_cbHullTerrainLOD.AccessData().m_minMaxLOD.x = 1.0f;
    m_cbHullTerrainLOD.AccessData().m_minMaxLOD.y = 63.0f;
    m_cbHullTerrainLOD.AccessData().m_minMaxDistance.x = 0.0f;
    m_cbHullTerrainLOD.AccessData().m_minMaxDistance.y = 100.0f;

    m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.x = 1.0f;
    m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.y = 1.0f;
    m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.z = 128.8;

    // For the standard, default strip based approach for terrain
    {
        // Number of grid cells within mesh?
        int32_t xResolution = HeightmapGridResolution;
        int32_t zResolution = HeightmapGridResolution;


        // Terrain width
        const float terrainWidth = 1.0f;
        const Farlor::Vector3 bottomLeftTerrainPoint
              = Farlor::Vector3(-terrainWidth / 2.0f, 0.0f, -terrainWidth / 2.0f);

        std::vector<VertexPositionUV> gridVertices;
        std::vector<unsigned int> gridIndices;
        for (int32_t xIdx = 0; xIdx < xResolution; xIdx++) {
            for (int32_t zIdx = 0; zIdx < zResolution; zIdx++) {
                VertexPositionUV vertex;

                float xPos = bottomLeftTerrainPoint.x + (1.0f / xResolution) * xIdx;
                float zPos = bottomLeftTerrainPoint.z + (1.0f / zResolution) * zIdx;

                vertex.m_position = Vector3(xPos, 0.0, zPos);
                vertex.m_uv = Vector2(
                      xIdx * (1.0f / (xResolution - 1)), zIdx * (1.0f / (zResolution - 1)));

                gridVertices.push_back(vertex);
            }
        }

        for (unsigned int zIdx = 0; zIdx < zResolution - 1;
              zIdx++)  // for each row a.k.a. each strip
        {
            for (unsigned int xIdx = 0; xIdx < xResolution; xIdx++)  // for each column
            {
                for (unsigned int k = 0; k < 2; k++)  // for each side of the strip
                {
                    gridIndices.push_back(xIdx + xResolution * (zIdx + k));
                }
            }
        }

        m_sandHeightfieldVertexCount = gridVertices.size();
        m_sandHeightfieldIndexCount = gridIndices.size();


        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(VertexPositionUV) * m_sandHeightfieldVertexCount;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vertexData;
        ZeroMemory(&vertexData, sizeof(vertexData));
        vertexData.pSysMem = gridVertices.data();
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(
              &vertexBufferDesc, &vertexData, m_pSandHeightfieldVertexBuffer.GetAddressOf());
        assert(m_pSandHeightfieldVertexBuffer && "Failed to create ScalarField2D vertex buffer");
        SetDebugObjectName(m_pSandHeightfieldVertexBuffer.Get(), "Sand Heightfield Vertex Buffer");

        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_sandHeightfieldIndexCount;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA indexData;
        ZeroMemory(&indexData, sizeof(indexData));
        indexData.pSysMem = gridIndices.data();
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(
              &indexBufferDesc, &indexData, m_pSandHeightfieldIndexBuffer.GetAddressOf());
        assert(m_pSandHeightfieldIndexBuffer && "Failed to create ScalarField2D index buffer");
        SetDebugObjectName(m_pSandHeightfieldIndexBuffer.Get(), "Sand Heightfield Index Buffer");
    }

    // Setup Resources for the Dynamically Tessalated Terrain
    {
        // Number of grid cells within mesh?
        int32_t xResolution = HeightmapGridResolution;
        int32_t zResolution = HeightmapGridResolution;

        // Terrain width
        int32_t numVertices = (xResolution + 1) * (zResolution + 1);

        float width = static_cast<float>(xResolution);
        float height = static_cast<float>(zResolution);

        std::vector<VertexPositionUV> gridVertices(numVertices);
        for (int x = 0; x < (xResolution + 1); ++x) {
            for (int z = 0; z < (zResolution + 1); ++z) {
                VertexPositionUV vertex;
                vertex.m_position = Farlor::Vector3(static_cast<float>(x) / width - 0.5f, 0.0f,
                      static_cast<float>(z) / height - 0.5f);
                vertex.m_uv = Farlor::Vector2(
                      static_cast<float>(x) / width, 1.0f - static_cast<float>(z) / height);
                gridVertices[x + z * (xResolution + 1)] = vertex;
            }
        }


        std::vector<unsigned int> gridIndices;
#define clamp(value, minimum, maximum) (std::max(std::min((value), (maximum)), (minimum)))
        for (int x = 0; x < xResolution; ++x) {
            for (int z = 0; z < zResolution; ++z) {
                // Define 12 control points per terrain quad

                // 0-3 are the actual quad vertices
                gridIndices.push_back((z + 0) + (x + 0) * (xResolution + 1));
                gridIndices.push_back((z + 1) + (x + 0) * (xResolution + 1));
                gridIndices.push_back((z + 0) + (x + 1) * (xResolution + 1));
                gridIndices.push_back((z + 1) + (x + 1) * (xResolution + 1));

                // 4-5 are +x
                gridIndices.push_back(clamp(z + 0, 0, zResolution)
                      + clamp(x + 2, 0, xResolution) * (xResolution + 1));
                gridIndices.push_back(clamp(z + 1, 0, zResolution)
                      + clamp(x + 2, 0, xResolution) * (xResolution + 1));

                // 6-7 are +z
                gridIndices.push_back(clamp(z + 2, 0, zResolution)
                      + clamp(x + 0, 0, xResolution) * (xResolution + 1));
                gridIndices.push_back(clamp(z + 2, 0, zResolution)
                      + clamp(x + 1, 0, xResolution) * (xResolution + 1));

                // 8-9 are -x
                gridIndices.push_back(clamp(z + 0, 0, zResolution)
                      + clamp(x - 1, 0, xResolution) * (xResolution + 1));
                gridIndices.push_back(clamp(z + 1, 0, zResolution)
                      + clamp(x - 1, 0, xResolution) * (xResolution + 1));

                // 10-11 are -z
                gridIndices.push_back(clamp(z - 1, 0, zResolution)
                      + clamp(x + 0, 0, xResolution) * (xResolution + 1));
                gridIndices.push_back(clamp(z - 1, 0, zResolution)
                      + clamp(x + 1, 0, xResolution) * (xResolution + 1));
            }
        }

        m_tesselatedTerrainVertexCount = gridVertices.size();
        m_tesselatedTerrainIndexCount = gridIndices.size();

        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(VertexPositionUV) * m_tesselatedTerrainVertexCount;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vertexData;
        ZeroMemory(&vertexData, sizeof(vertexData));
        vertexData.pSysMem = gridVertices.data();
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(
              &vertexBufferDesc, &vertexData, m_pTesselatedTerrainVertexBuffer.GetAddressOf());
        assert(m_pTesselatedTerrainVertexBuffer
              && "Failed to create Tesselated Terrain vertex buffer");
        SetDebugObjectName(
              m_pTesselatedTerrainVertexBuffer.Get(), "Tesselated Terrain Vertex Buffer");

        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_tesselatedTerrainIndexCount;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA indexData;
        ZeroMemory(&indexData, sizeof(indexData));
        indexData.pSysMem = gridIndices.data();
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(
              &indexBufferDesc, &indexData, m_pTesselatedTerrainIndexBuffer.GetAddressOf());
        assert(m_pTesselatedTerrainIndexBuffer
              && "Failed to create Tesselated Terrain index buffer");
        SetDebugObjectName(
              m_pTesselatedTerrainIndexBuffer.Get(), "Tesselated Terrain Index Buffer");
    }

    m_upSMAA = std::make_unique<agz::smaa::SMAA>(m_pDevice.Get(), m_pDeviceContext.Get(), m_width,
          m_height, agz::smaa::EdgeDetectionMode::Lum);
}

bool Renderer::StepDesertSimulation()
{
    m_simulationStepCount++;

    m_largeScaleDuneModel.StepDesertSimulation(m_pDeviceContext.Get(), m_pPerf.Get());
    m_smallScaleRippleModel.StepDesertSimulation(m_pDeviceContext.Get(), m_pPerf.Get());

    return true;
}

void Renderer::ExportFrame()
{
    // const int FrameCountToExport = 10;
    // assert(m_exportFrameSequenceActive && "Attempting to export a frame when not active");
    // assert((m_exportFrameSequenceIdx < FrameCountToExport) && "Exporting frame idx out of range");

    // std::cout << "\tExporting frame: " << m_exportFrameSequenceIdx << std::endl;
    // m_pDeviceContext->CopyResource(
    //       m_pFinalHeightmapReadbackStagingTexture.Get(), m_pCombinedHeightTexture.Get());
    // D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    // HRESULT result = m_pDeviceContext->Map(
    //       m_pFinalHeightmapReadbackStagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource);
    // assert(SUCCEEDED(result) && "Failed to map read back texture");
    // std::vector<float> readData(DesertSimulationGridSize * DesertSimulationGridSize);
    // memcpy(readData.data(), mappedSubresource.pData,
    //       sizeof(float) * DesertSimulationGridSize * DesertSimulationGridSize);
    // m_pDeviceContext->Unmap(m_pFinalHeightmapReadbackStagingTexture.Get(), 0);

    // Imf::Array2D<Imf::Rgba> exrData;
    // exrData.resizeErase(DesertSimulationGridSize, DesertSimulationGridSize);
    // for (uint32_t newMipX = 0; newMipX < DesertSimulationGridSize; newMipX++) {
    //     for (uint32_t newMipY = 0; newMipY < DesertSimulationGridSize; newMipY++) {
    //         exrData[newMipY][newMipX].r = readData[newMipX * DesertSimulationGridSize + newMipY];
    //         exrData[newMipY][newMipX].g = readData[newMipX * DesertSimulationGridSize + newMipY];
    //         exrData[newMipY][newMipX].b = readData[newMipX * DesertSimulationGridSize + newMipY];
    //         exrData[newMipY][newMipX].a = 1.0f;
    //     }
    // }

    // std::ostringstream exrExportFilename;
    // exrExportFilename << m_exportFolder << '/' << "frame_image_" << m_exportFrameSequenceIdx
    //                   << ".exr";

    // Imf::RgbaOutputFile file(exrExportFilename.str().c_str(), DesertSimulationGridSize,
    //       DesertSimulationGridSize, Imf::WRITE_RGBA);  // 1

    // file.setFrameBuffer(&exrData[0][0], 1, DesertSimulationGridSize);  // 2
    // file.writePixels(DesertSimulationGridSize);                        // 3

    // m_exportFrameSequenceIdx++;
    // if (m_exportFrameSequenceIdx >= FrameCountToExport) {
    //     m_exportFrameSequenceActive = false;
    // }
}
void Renderer::InitializeDXGI(std::vector<IDXGIAdapter *> &adapters)
{
    HRESULT result = S_OK;
    bool debugDXGI = false;

    // TODO: Make sure this is only for development mode
    result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_dxgiDebug1.GetAddressOf()));
    if (SUCCEEDED(result)) {
        debugDXGI = true;

        result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_dxgiInfoQueue.GetAddressOf()));
        if (FAILED(result)) {
            FARLOR_LOG_ERROR("Failed to create debug dxgi info queue");
        }

        m_dxgiInfoQueue->SetBreakOnSeverity(
              DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        m_dxgiInfoQueue->SetBreakOnSeverity(
              DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }

    result = CreateDXGIFactory2(
          debugDXGI ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(m_pDxgiFactory.GetAddressOf()));
    assert(m_pDxgiFactory && "Failed to create dxgi factory 2");

    IDXGIAdapter *pAdapter;
    for (UINT i = 0; m_pDxgiFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC desc = { 0 };
        pAdapter->GetDesc(&desc);
        // We've got an nvidia card
        if (desc.VendorId == 4318) {
            adapters.push_back(pAdapter);
        }
    }

    for (auto &val : adapters) {
        DXGI_ADAPTER_DESC desc = { 0 };
        val->GetDesc(&desc);
        std::wcout << L"Adapter Vendor ID: " << desc.VendorId << std::endl;
        std::wcout << L"Adapter Description: " << desc.Description << std::endl;
    }
}

void Renderer::SetupSMAAResources()
{
    // Edge Target Texture
    {
        D3D11_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = m_width;
        textureDesc.Height = m_height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        m_pDevice->CreateTexture2D(&textureDesc, nullptr, m_pEdgeTargetSMAA.GetAddressOf());
        assert(m_pEdgeTargetSMAA && "Failed to create SMAA weight target");
        SetDebugObjectName(m_pEdgeTargetSMAA.Get(), "Edge Target SMAA");
    }

    {
        D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
        textureRTVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        textureRTVDesc.Texture2D.MipSlice = 0;
        m_pDevice->CreateRenderTargetView(
              m_pEdgeTargetSMAA.Get(), &textureRTVDesc, m_pEdgeTargetRTV.GetAddressOf());
        assert(m_pEdgeTargetRTV && "Failed to create SMAA edge RTV");
        SetDebugObjectName(m_pEdgeTargetRTV.Get(), "Edge Target SMAA RTV");
    }

    {
        D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
        textureSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        textureSRVDesc.Texture2D.MipLevels = 1;
        textureSRVDesc.Texture2D.MostDetailedMip = 0;
        m_pDevice->CreateShaderResourceView(
              m_pEdgeTargetSMAA.Get(), &textureSRVDesc, m_pEdgeTargetSRV.GetAddressOf());
        assert(m_pEdgeTargetSRV && "Failed to create SMAA edge SRV");
        SetDebugObjectName(m_pEdgeTargetSRV.Get(), "Edge Target SMAA SRV");
    }


    // Weight Target
    {
        D3D11_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = m_width;
        textureDesc.Height = m_height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        m_pDevice->CreateTexture2D(&textureDesc, nullptr, m_pWeightTargetSMAA.GetAddressOf());
        assert(m_pWeightTargetSMAA && "Failed to create SMAA weight target");
        SetDebugObjectName(m_pWeightTargetSMAA.Get(), "Weight Target SMAA");
    }

    {
        D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
        textureRTVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        textureRTVDesc.Texture2D.MipSlice = 0;
        m_pDevice->CreateRenderTargetView(
              m_pWeightTargetSMAA.Get(), &textureRTVDesc, m_pWeightTargetRTV.GetAddressOf());
        assert(m_pWeightTargetRTV && "Failed to create SMAA edge RTV");
        SetDebugObjectName(m_pWeightTargetRTV.Get(), "Weight Target SMAA RTV");
    }

    {
        D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
        textureSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        textureSRVDesc.Texture2D.MipLevels = 1;
        textureSRVDesc.Texture2D.MostDetailedMip = 0;
        m_pDevice->CreateShaderResourceView(
              m_pWeightTargetSMAA.Get(), &textureSRVDesc, m_pWeightTargetSRV.GetAddressOf());
        assert(m_pWeightTargetSRV && "Failed to create SMAA edge SRV");
        SetDebugObjectName(m_pWeightTargetSRV.Get(), "Weight Target SMAA SRV");
    }


    // AA Target
    {
        D3D11_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = m_width;
        textureDesc.Height = m_height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        m_pDevice->CreateTexture2D(&textureDesc, nullptr, m_pAATargetSMAA.GetAddressOf());
        assert(m_pWeightTargetSMAA && "Failed to create SMAA weight target");
        SetDebugObjectName(m_pAATargetSMAA.Get(), "AA Target SMAA");
    }

    {
        D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
        textureRTVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        textureRTVDesc.Texture2D.MipSlice = 0;
        m_pDevice->CreateRenderTargetView(
              m_pAATargetSMAA.Get(), &textureRTVDesc, m_pAATargetRTV.GetAddressOf());
        assert(m_pWeightTargetRTV && "Failed to create SMAA edge RTV");
        SetDebugObjectName(m_pAATargetRTV.Get(), "AA Target SMAA RTV");
    }

    {
        D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
        textureSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        textureSRVDesc.Texture2D.MipLevels = 1;
        textureSRVDesc.Texture2D.MostDetailedMip = 0;
        m_pDevice->CreateShaderResourceView(
              m_pAATargetSMAA.Get(), &textureSRVDesc, m_pAATargetSRV.GetAddressOf());
        assert(m_pWeightTargetSRV && "Failed to create SMAA edge SRV");
        SetDebugObjectName(m_pAATargetSRV.Get(), "AA Target SMAA SRV");
    }
}

void Renderer::SetupDesertSimulation(
      const float rMin, const float rMax, const Imf::Array2D<Imf::Rgba> &defaultValues)
{
    // Default to no vegetation
    std::vector<float> smallRippleVegetation(
          m_smallScaleRippleModel.GetGridResolution() * m_smallScaleRippleModel.GetGridResolution(),
          0.0f);

    std::vector<float> smallRippleBedrock(
          m_smallScaleRippleModel.GetGridResolution() * m_smallScaleRippleModel.GetGridResolution(),
          0.0f);

    float defaultValue = m_smallScaleRippleModel.GetCellSizeMeters();
    std::vector<float> smallRippleSand(
          m_smallScaleRippleModel.GetGridResolution() * m_smallScaleRippleModel.GetGridResolution(),
          defaultValue);

    m_smallScaleRippleModel.SetupDesertSimulation(m_pDevice.Get(), m_pDeviceContext.Get(),
          smallRippleSand, smallRippleBedrock, smallRippleVegetation);

    // Default to no vegetation
    std::vector<float> largeScaleVegetation(
          m_largeScaleDuneModel.GetGridResolution() * m_largeScaleDuneModel.GetGridResolution(),
          0.0f);
    for (size_t i = 0; i < m_largeScaleDuneModel.GetGridResolution(); i++) {
        for (size_t j = 0; j < m_largeScaleDuneModel.GetGridResolution(); j++) {
            if ((abs(m_largeScaleDuneModel.GetGridResolution() / 2.0f - i) < 50.0f)
                  && (abs(m_largeScaleDuneModel.GetGridResolution() / 2.0f - i) < 50.0f)) {
                largeScaleVegetation[i * m_largeScaleDuneModel.GetGridResolution() + j] = 1.0f;
            }
        }
    }

    std::vector<float> largeScaleBedrock(
          m_largeScaleDuneModel.GetGridResolution() * m_largeScaleDuneModel.GetGridResolution(),
          0.0f);

    for (size_t i = 0; i < m_largeScaleDuneModel.GetGridResolution(); i++) {
        for (size_t j = 0; j < m_largeScaleDuneModel.GetGridResolution(); j++) {
            largeScaleBedrock[i * m_largeScaleDuneModel.GetGridResolution() + j]
                  = defaultValues[i][j].r * 100;
        }
    }

    std::vector<float> largeScaleSand(
          m_largeScaleDuneModel.GetGridResolution() * m_largeScaleDuneModel.GetGridResolution(),
          m_largeScaleDuneModel.GetCellSizeMeters() * 20);

    m_largeScaleDuneModel.SetupDesertSimulation(m_pDevice.Get(), m_pDeviceContext.Get(),
          largeScaleSand, largeScaleBedrock, largeScaleVegetation);
}

void Renderer::FirstFrameSetupWithShaders()
{
    m_largeScaleDuneModel.FirstFrameSetup(m_pDeviceContext.Get(), m_pPerf.Get());
    m_smallScaleRippleModel.FirstFrameSetup(m_pDeviceContext.Get(), m_pPerf.Get());
}

void Renderer::Shutdown()
{
    if (m_d3d11Debug) {
        HRESULT result = m_d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        if (FAILED(result)) {
            std::cout << "Failed to d3d11 ReportLiveDeviceObjects" << std::endl;
        }
    }

    if (m_dxgiDebug1) {
        HRESULT result = m_dxgiDebug1->ReportLiveObjects(DXGI_DEBUG_DX, DXGI_DEBUG_RLO_SUMMARY);
        if (FAILED(result)) {
            std::cout << "Failed to d3d11 ReportLiveDeviceObjects" << std::endl;
        }
    }
}

void Renderer::Render(const Camera &currentCamera, const float dt)
{
    m_pPerf->BeginEvent(L"Full Frame Render");

    static bool firstFrame = true;
    static uint32_t frameCount = 0;

    // Initialize the imgui frame and create a default window for controls
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Main Window", 0,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);
    }


    if (firstFrame) {
        FirstFrameSetupWithShaders();
        firstFrame = false;
    }

    m_pPerf->BeginEvent(L"Desert Simulation Step");
    StepDesertSimulation();
    m_pPerf->EndEvent();
    // Position clear color
    {
        m_pPerf->BeginEvent(L"Initial buffer clears and frame setup");

        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), clearColor);
        m_pDeviceContext->ClearDepthStencilView(
              m_pWindowDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            union FloatInt {
                float m_floatVal;
                int32_t m_intVal;
            };
            FloatInt myVal;
            myVal.m_intVal = 0;
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, myVal.m_floatVal };
            m_pDeviceContext->ClearRenderTargetView(
                  m_renderTargets["G-Position"].m_pRTView, clearColor);
        }
        m_pDeviceContext->ClearRenderTargetView(m_renderTargets["G-Normals"].m_pRTView, clearColor);
        m_pDeviceContext->ClearRenderTargetView(m_renderTargets["G-Diffuse"].m_pRTView, clearColor);
        m_pDeviceContext->ClearDepthStencilView(
              m_pWindowDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        m_pDeviceContext->OMSetDepthStencilState(m_pEnableDepthStencilState.Get(), 0);

        m_windowClientViewport = *currentCamera.GetViewport().Get11();

        // Set to main viewport
        m_pDeviceContext->RSSetViewports(1, &m_windowClientViewport);

        // Perspective
        m_camProjection = currentCamera.GetCamProjection();
        m_camView = currentCamera.GetCamView();

        m_pPerf->EndEvent();
    }

    // Render the height field
    // This is part of the gbuffer pass
    {
        m_pPerf->BeginEvent(L"Render Heightfield");

        // Go ahead and render out as a gbuffer object
        std::array renderTargetViews = { m_renderTargets["G-Position"].m_pRTView,
            m_renderTargets["G-Normals"].m_pRTView, m_renderTargets["G-Diffuse"].m_pRTView };
        m_pDeviceContext->OMSetRenderTargets(
              renderTargetViews.size(), renderTargetViews.data(), m_pWindowDSV.Get());

        // Note: Render lights handled this, so we probably dont need this, but we
        // will do it again to be sure
        DirectX::XMFLOAT4 pos = currentCamera.GetCamPos();
        m_cameraParams.AccessData().m_eyePosition = Vector3(pos.x, pos.y, pos.z);
        m_cameraParams.Update(m_pDeviceContext.Get());

        m_pDeviceContext->RSSetState(m_pNoCullRS.Get());
        // m_pDeviceContext->RSSetState(m_pWireframeRS.Get());

        DirectX::XMMATRIX world
              = DirectX::XMMatrixScaling(m_desertScale.x, m_desertScale.y, m_desertScale.z);
        world *= DirectX::XMMatrixRotationRollPitchYaw(
              m_desertRotation.x * static_cast<float>(M_PI) / 180.0f,
              m_desertRotation.y * static_cast<float>(M_PI) / 180.0f,
              m_desertRotation.z * static_cast<float>(M_PI) / 180.0f);
        world *= DirectX::XMMatrixTranslation(
              m_desertTranslation.x, m_desertTranslation.y, m_desertTranslation.z);

        // Update resources
        DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_camProjection);
        DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_camView);

        // Dont move grid at all for now
        DirectX::XMMATRIX vp = view;
        vp = DirectX::XMMatrixMultiply(vp, proj);

        DirectX::XMVECTOR det;
        DirectX::XMMATRIX invTransposeWorld
              = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, world));

        //RenderSand(world, vp, invTransposeWorld);
        RenderSandTess(world, vp, invTransposeWorld, currentCamera.GetCamPos());

        m_pDeviceContext->RSSetState(nullptr);

        std::fill(renderTargetViews.begin(), renderTargetViews.end(), nullptr);
        m_pDeviceContext->OMSetRenderTargets(
              renderTargetViews.size(), renderTargetViews.data(), nullptr);

        m_pPerf->EndEvent();
    }

    // Render all other objects
    if (m_renderingComponents.size() > 0) {
        m_pPerf->BeginEvent(L"Deferred - Render PBR Objects");

        // Push State
        {
            m_shaders.at("GBufferBasePass").SetPipeline(m_pDeviceContext.Get());

            const uint32_t numRenderTargetViews = 3;
            ID3D11RenderTargetView *renderTargetViews[numRenderTargetViews];
            renderTargetViews[0] = m_renderTargets["G-Position"].m_pRTView;
            renderTargetViews[1] = m_renderTargets["G-Normals"].m_pRTView;
            renderTargetViews[2] = m_renderTargets["G-Diffuse"].m_pRTView;
            m_pDeviceContext->OMSetRenderTargets(
                  numRenderTargetViews, renderTargetViews, m_pWindowDSV.Get());

            const uint32_t numSamplers = 1;
            ID3D11SamplerState *pSamplers[numSamplers] = { m_pWrapSamplerState.Get() };
            m_pDeviceContext->PSSetSamplers(0, numSamplers, pSamplers);
        }

        // Draw
        {
            // Render all objects
            for (auto &renderComponent : m_renderingComponents) {
                Render(renderComponent.second, currentCamera);
            }
        }

        // Pop State
        {
            // Render of render component dirties constant buffer state
            ID3D11Buffer *pConstantBuffers[2] = { nullptr, nullptr };
            m_pDeviceContext->VSSetConstantBuffers(0, 2, pConstantBuffers);
            m_pDeviceContext->PSSetConstantBuffers(0, 2, pConstantBuffers);

            const uint32_t numSamplers = 1;
            ID3D11SamplerState *pSamplers[numSamplers] = { nullptr };
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            const uint32_t numRenderTargetViews = 3;
            ID3D11RenderTargetView *renderTargetViews[numRenderTargetViews]
                  = { nullptr, nullptr, nullptr };
            m_pDeviceContext->OMSetRenderTargets(3, renderTargetViews, nullptr);

            m_shaders.at("GBufferBasePass").ReleasePipeline(m_pDeviceContext.Get());
        }
        m_pPerf->EndEvent();
    }

    // Lighting pass
    {
        m_pPerf->BeginEvent(L"Lighting");
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_pDeviceContext->ClearRenderTargetView(
              m_renderTargets["Light-Accumulation"].m_pRTView, clearColor);

        m_pDeviceContext->IASetInputLayout(nullptr);
        m_pDeviceContext->OMSetDepthStencilState(m_pDisableDepthStencilState.Get(), 0);

        ID3D11RenderTargetView *pRenderTargets[1]
              = { m_renderTargets["Light-Accumulation"].m_pRTView };
        m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, m_pWindowDSV.Get());

        // Enable additive blending for the lighting pass
        // Each pass of the lighting additivly contributes more light to the final
        // accumulated result
        m_pDeviceContext->OMSetBlendState(m_pAdditiveBlendState.Get(), 0, 0xffffffff);

        // Render ambient light
        {
            m_pPerf->BeginEvent(L"PBR Ambient Light Pass");
            m_shaders.at("GBufferLightAmbient").SetPipeline(m_pDeviceContext.Get());

            ID3D11ShaderResourceView *resourceViews[2];
            resourceViews[0] = m_renderTargets["G-Position"].m_pRTShaderResourceView;
            resourceViews[1] = m_renderTargets["G-Diffuse"].m_pRTShaderResourceView;
            m_pDeviceContext->PSSetShaderResources(0, 2, resourceViews);

            ID3D11SamplerState *pSamplers[1] = { m_pClampSamplerState.Get() };
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            m_pDeviceContext->Draw(3, 0);

            pSamplers[0] = nullptr;
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            resourceViews[0] = nullptr;
            resourceViews[1] = nullptr;
            m_pDeviceContext->PSSetShaderResources(0, 2, resourceViews);
            m_shaders.at("GBufferLightAmbient").ReleasePipeline(m_pDeviceContext.Get());
            m_pPerf->EndEvent();
        }

        {
            m_pPerf->BeginEvent(L"PBR Lights");
            m_shaders.at("GBufferLightPassDirectional").SetPipeline(m_pDeviceContext.Get());

            ID3D11ShaderResourceView *resourceViews[3];
            resourceViews[0] = m_renderTargets["G-Position"].m_pRTShaderResourceView;
            resourceViews[1] = m_renderTargets["G-Normals"].m_pRTShaderResourceView;
            resourceViews[2] = m_renderTargets["G-Diffuse"].m_pRTShaderResourceView;
            m_pDeviceContext->PSSetShaderResources(0, 3, resourceViews);

            ID3D11SamplerState *pSamplers[1] = { m_pClampSamplerState.Get() };
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            // Render Lights
            for (auto &lightComponent : m_lightComponents) {
                RenderLight(lightComponent.second, currentCamera);
            }

            // Render light sets some state, lets change that back here
            ID3D11Buffer *pConstantBuffers[2];
            pConstantBuffers[0] = nullptr;
            pConstantBuffers[1] = nullptr;
            m_pDeviceContext->PSSetConstantBuffers(0, 2, pConstantBuffers);

            pSamplers[0] = nullptr;
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            resourceViews[0] = nullptr;
            resourceViews[1] = nullptr;
            resourceViews[2] = nullptr;
            m_pDeviceContext->PSSetShaderResources(0, 3, resourceViews);

            m_shaders.at("GBufferLightPassDirectional").ReleasePipeline(m_pDeviceContext.Get());
            m_pPerf->EndEvent();
        }

        // We disable blending again
        m_pDeviceContext->OMSetBlendState(m_pNoBlendState.Get(), 0, 0xffffffff);

        pRenderTargets[0] = nullptr;
        m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

        m_pPerf->EndEvent();
    }

    // Now we render out the back buffer
    {
        m_pPerf->BeginEvent(L"Tonemapping");

        // Disable the depth stencil test
        m_pDeviceContext->OMSetDepthStencilState(m_pDisableDepthStencilState.Get(), 0);

        // We want to now calculate the average luminance
        // This is done by rendering the HDR luminance out to a 1024x1024 texture
        // then generating the mipmaps of this texture.
        // The lowest level, 1x1 mipmap is the average luminance
        {
            m_pPerf->BeginEvent(L"Average Luminance");
            // Grab the current viewport to reset at the end of the draw
            D3D11_VIEWPORT currentViewport;
            uint32_t numViewports = 1;
            m_pDeviceContext->RSGetViewports(&numViewports, &currentViewport);

            m_pDeviceContext->RSSetViewports(1, &m_luminanceViewport);
            // First, lets do the texture render
            {
                ID3D11RenderTargetView *pRenderTargets[1] = { m_luminanceTexture.GetRTV() };
                m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

                m_shaders.at("Luminance").SetPipeline(m_pDeviceContext.Get());

                ID3D11ShaderResourceView *resourceViews[1];
                resourceViews[0] = m_renderTargets["Light-Accumulation"].m_pRTShaderResourceView;
                m_pDeviceContext->PSSetShaderResources(0, 1, resourceViews);

                ID3D11SamplerState *pSamplers[1] = { m_pClampSamplerState.Get() };
                m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

                // Turn on alpha blending
                m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                m_pDeviceContext->Draw(3, 0);

                pSamplers[0] = nullptr;
                m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

                resourceViews[0] = nullptr;
                m_pDeviceContext->PSSetShaderResources(0, 1, resourceViews);

                m_shaders.at("Luminance").ReleasePipeline(m_pDeviceContext.Get());

                pRenderTargets[0] = nullptr;
                m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);
            }

            m_luminanceTexture.GenerateMips(m_pDeviceContext.Get());

            // Set the viewport back to what it was
            m_pDeviceContext->RSSetViewports(numViewports, &currentViewport);
            m_pPerf->EndEvent();
        }

        // Push State
        {
            m_shaders.at("Tonemapping").SetPipeline(m_pDeviceContext.Get());

            ID3D11RenderTargetView *pRenderTargets[1]
                  = { m_enableSMAA ? m_pAATargetRTV.Get() : m_pBackBufferRTV.Get() };
            m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

            const uint32_t numPixelShaderResources = 2;
            ID3D11ShaderResourceView *resourceViews[numPixelShaderResources] = { nullptr, nullptr };
            resourceViews[0] = m_renderTargets["Light-Accumulation"].m_pRTShaderResourceView;
            resourceViews[1] = m_luminanceTexture.GetSRV();
            m_pDeviceContext->PSSetShaderResources(0, numPixelShaderResources, resourceViews);

            const uint32_t numPixelShaderSamplers = 1;
            ID3D11SamplerState *pSamplers[numPixelShaderSamplers] = { m_pClampSamplerState.Get() };
            m_pDeviceContext->PSSetSamplers(0, numPixelShaderSamplers, pSamplers);

            m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        }

        // Draw
        {
            m_pDeviceContext->Draw(3, 0);
        }

        // Pop state
        {
            const uint32_t numPixelShaderSamplers = 1;
            ID3D11SamplerState *pSamplers[numPixelShaderSamplers] = { nullptr };
            m_pDeviceContext->PSSetSamplers(0, 1, pSamplers);

            const uint32_t numPixelShaderResources = 2;
            ID3D11ShaderResourceView *resourceViews[numPixelShaderResources] = { nullptr, nullptr };
            m_pDeviceContext->PSSetShaderResources(0, numPixelShaderResources, resourceViews);

            ID3D11RenderTargetView *pRenderTargets[1] = { nullptr };
            m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

            m_shaders.at("Tonemapping").ReleasePipeline(m_pDeviceContext.Get());
        }

        m_pPerf->EndEvent();
    }

    // Lets do the SMAA
    if (m_enableSMAA) {
        {
            m_pPerf->BeginEvent(L"SMAA Edge Detection");
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            m_pDeviceContext->ClearRenderTargetView(m_pEdgeTargetRTV.Get(), clearColor);
            ID3D11RenderTargetView *pRenderTargets[1] = { m_pEdgeTargetRTV.Get() };
            m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);
            m_upSMAA->detectEdge(m_pAATargetSRV.Get());
            m_pPerf->EndEvent();
        }

        {
            m_pPerf->BeginEvent(L"SMAA Blending Weight Computation");
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            m_pDeviceContext->ClearRenderTargetView(m_pWeightTargetRTV.Get(), clearColor);
            ID3D11RenderTargetView *pRenderTargets[1] = { m_pWeightTargetRTV.Get() };
            m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);
            m_upSMAA->computeBlendingWeight(m_pEdgeTargetSRV.Get());
            m_pPerf->EndEvent();
        }

        {
            m_pPerf->BeginEvent(L"SMAA Blend");
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), clearColor);
            ID3D11RenderTargetView *pRenderTargets[1] = { m_pBackBufferRTV.Get() };
            m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);
            m_upSMAA->blend(m_pWeightTargetSRV.Get(), m_pAATargetSRV.Get());
            m_pPerf->EndEvent();
        }
    }

    // Initialize imgui frame drawing resources
    {
        // Print simulation step count
        {
            std::string value = "Simulation Step: ";
            value += std::to_string(m_simulationStepCount);
            ImGui::Text("%s", value.c_str());
        }

        if (ImGui::CollapsingHeader("Render Params", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Min Tess LOD", &m_cbHullTerrainLOD.AccessData().m_minMaxLOD.x, 1.0f,
                  5.0f, "%.6f");
            ImGui::SliderFloat("Max Tess LOD", &m_cbHullTerrainLOD.AccessData().m_minMaxLOD.y, 1.0f,
                  63.0f, "%.6f");

            ImGui::SliderFloat("Min Distance", &m_cbHullTerrainLOD.AccessData().m_minMaxDistance.x,
                  0.0f, 5.0f, "%.6f");
            ImGui::SliderFloat("Max Distance", &m_cbHullTerrainLOD.AccessData().m_minMaxDistance.y,
                  10.0f, 500.0f, "%.6f");

            ImGui::SliderFloat("Large Scale Heightmap SF",
                  &m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.x, 0.0f, 5.0f,
                  "%.6f");

            ImGui::SliderFloat("Small Scale Heightmap SF",
                  &m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.y, 0.0f, 2000.0f,
                  "%.6f");
            ImGui::SliderFloat("Small Scale UV SF",
                  &m_cbDomainTerrainSettings.AccessData().m_heightmapScaleFactors.z, 1.0f, 1024.0f,
                  "%.6f");

            ImGui::Checkbox("Enable SMAA", &m_enableSMAA);
        }

        if (ImGui::CollapsingHeader("Desert Transform")) {
            ImGui::SliderFloat3("Desert Grid Translation", &m_desertTranslation.m_data[0], -100.0f,
                  100.0f, "%.3f");
            ImGui::SliderFloat3(
                  "Desert Grid Rotation", &m_desertRotation.m_data[0], 0.0f, 360.0f, "%.3f");
            ImGui::SliderFloat3(
                  "Desert Grid Scale", &m_desertScale.m_data[0], 0.0f, 1024.0f, "%.3f");
        }

        if (ImGui::CollapsingHeader("Large Scale Desert Params", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_largeScaleDuneModel.Gui();
        }

        if (ImGui::CollapsingHeader(
                  "Small Scale Ripple Desert Params", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_smallScaleRippleModel.Gui();
        }

        RenderFrameTimeGraph();

        ImGui::End();

        m_pPerf->BeginEvent(L"Imgui Render");

        // Set the render target
        ID3D11RenderTargetView *pRenderTargets[1] = { m_pBackBufferRTV.Get() };
        m_pDeviceContext->OMSetDepthStencilState(m_pDisableDepthStencilState.Get(), 0);
        m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        pRenderTargets[0] = nullptr;
        m_pDeviceContext->OMSetRenderTargets(1, pRenderTargets, nullptr);

        m_pPerf->EndEvent();
    }

    m_pPerf->EndEvent();

    frameCount++;

    m_pSwapChain->Present(0, 0);

    if (m_pD3D11InfoQueue) {
        UINT64 message_count = m_pD3D11InfoQueue->GetNumStoredMessages();

        for (UINT64 i = 0; i < message_count; i++) {
            SIZE_T message_size = 0;
            m_pD3D11InfoQueue->GetMessage(i, nullptr,
                  &message_size);  // get the size of the message

            D3D11_MESSAGE *message
                  = (D3D11_MESSAGE *)malloc(message_size);  // allocate enough space
            HRESULT result = m_pD3D11InfoQueue->GetMessage(
                  i, message, &message_size);  // get the actual message
            if (FAILED(result)) {
                std::cout << "Failed to retrieve message from infoqueue" << std::endl;
                continue;
            }

            // do whatever you want to do with it
            if (message) {
                std::cout << "Directx11: " << message->ID << ", " << message->pDescription
                          << std::endl;
            }

            free(message);
        }

        m_pD3D11InfoQueue->ClearStoredMessages();
    }
}

void Renderer::RenderSand(const DirectX::XMMATRIX &world, const DirectX::XMMATRIX &vp,
      const DirectX::XMMATRIX &invTransposeWorld)
{  // Sand Heightfield
    {
        m_pPerf->BeginEvent(L"Render Sand Heightfield");

        m_cbSandHeightfield.AccessData().WorldMatrix = DirectX::XMMatrixTranspose(world);
        m_cbSandHeightfield.AccessData().InvTransposeWorld = invTransposeWorld;
        m_cbSandHeightfield.AccessData().VP = DirectX::XMMatrixTranspose(vp);
        m_cbSandHeightfield.Update(m_pDeviceContext.Get());

        unsigned int stride = sizeof(VertexPositionUV);
        unsigned int offset = 0;
        // Push State
        {  // IA
            {
                m_shaders["RenderSand"].SetPipeline(m_pDeviceContext.Get());

                ID3D11Buffer *vertexBuffers[1] = { m_pSandHeightfieldVertexBuffer.Get() };
                m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
                m_pDeviceContext->IASetIndexBuffer(
                      m_pSandHeightfieldIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            }

            // VS
            {
                ID3D11Buffer *pConstantBuffers[1] = { m_cbSandHeightfield.GetBuffer() };
                m_pDeviceContext->VSSetConstantBuffers(0, 1, pConstantBuffers);
                ID3D11ShaderResourceView *pSRVs[2] = { m_largeScaleDuneModel.GetSandHeightmap(),
                    m_smallScaleRippleModel.GetSandHeightmap() };
                m_pDeviceContext->VSSetShaderResources(0, 2, pSRVs);
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_pWrapSamplerState.Get() };
                m_pDeviceContext->VSSetSamplers(0, 1, ppSamplerStateBindTable);
            }

            // PS
            {
                ID3D11Buffer *pConstantBuffers[1] = { m_cbSandHeightfield.GetBuffer() };
                m_pDeviceContext->PSSetConstantBuffers(0, 1, pConstantBuffers);

                ID3D11ShaderResourceView *pSRVs[6] = { m_pSandAlbedoTextureSRV.Get(),
                    m_pSandNormalTextureSRV.Get(), m_pSandMetalnessTextureSRV.Get(),
                    m_pSandRoughnessTextureSRV.Get(), m_largeScaleDuneModel.GetWindShadow(),
                    m_largeScaleDuneModel.GetWindTexture() };
                m_pDeviceContext->PSSetShaderResources(0, 6, pSRVs);

                ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_pWrapSamplerState.Get() };
                m_pDeviceContext->PSSetSamplers(0, 1, ppSamplerStateBindTable);
            }
        }

        // Draw call
        for (unsigned int strip = 0; strip < HeightmapGridResolution - 1; ++strip) {
            m_pDeviceContext->DrawIndexed(
                  HeightmapGridResolution * 2, HeightmapGridResolution * 2 * strip, 0);
        }

        // Pop State
        {
            // PS
            {
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
                m_pDeviceContext->PSSetSamplers(0, 1, ppSamplerStateBindTable);

                std::array<ID3D11ShaderResourceView *, 6> pSRVs
                      = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                m_pDeviceContext->PSSetShaderResources(0, pSRVs.size(), pSRVs.data());

                ID3D11Buffer *pConstantBuffers[1] = { nullptr };
                m_pDeviceContext->PSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // VS
            {
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
                m_pDeviceContext->VSSetSamplers(0, 1, ppSamplerStateBindTable);
                ID3D11ShaderResourceView *pSRVs[2] = { nullptr, nullptr };
                m_pDeviceContext->VSSetShaderResources(0, 1, pSRVs);
                ID3D11Buffer *pConstantBuffers[1] = { nullptr };
                m_pDeviceContext->VSSetConstantBuffers(0, 1, pConstantBuffers);
            }
            // IA
            {
                ID3D11Buffer *vertexBuffers[1] = { nullptr };
                m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
                m_pDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
            }

            m_shaders["RenderSand"].ReleasePipeline(m_pDeviceContext.Get());
        }

        m_pPerf->EndEvent();
    }
}

// Uses a different algorithm that dynamically tesselates terrain
void Renderer::RenderSandTess(const DirectX::XMMATRIX &world, const DirectX::XMMATRIX &vp,
      const DirectX::XMMATRIX &invTransposeWorld, const DirectX::XMFLOAT4 &cameraPos)
{  // Sand Heightfield
    {
        m_pPerf->BeginEvent(L"Render Sand Heightfield Tesselated");

        m_cbSandHeightfield.AccessData().WorldMatrix = DirectX::XMMatrixTranspose(world);
        m_cbSandHeightfield.AccessData().InvTransposeWorld = invTransposeWorld;
        m_cbSandHeightfield.AccessData().VP = DirectX::XMMatrixTranspose(vp);
        m_cbSandHeightfield.Update(m_pDeviceContext.Get());

        m_cbHullTerrainLOD.AccessData().m_cameraPos = cameraPos;
        m_cbHullTerrainLOD.Update(m_pDeviceContext.Get());
        m_cbDomainTerrainSettings.Update(m_pDeviceContext.Get());

        unsigned int stride = sizeof(VertexPositionUV);
        unsigned int offset = 0;
        // Push State
        {
            // IA
            {
                m_shaders["RenderSandTess"].SetPipeline(m_pDeviceContext.Get());

                std::array<ID3D11Buffer *, 1> vertexBuffers
                      = { m_pTesselatedTerrainVertexBuffer.Get() };
                m_pDeviceContext->IASetVertexBuffers(
                      0, vertexBuffers.size(), vertexBuffers.data(), &stride, &offset);
                m_pDeviceContext->IASetIndexBuffer(
                      m_pTesselatedTerrainIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                m_pDeviceContext->IASetPrimitiveTopology(
                      D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
            }

            // VS
            {
                ID3D11Buffer *pConstantBuffers[1] = { m_cbSandHeightfield.GetBuffer() };
                m_pDeviceContext->VSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // HS
            {
                ID3D11Buffer *pConstantBuffers[1] = { m_cbHullTerrainLOD.GetBuffer() };
                m_pDeviceContext->HSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // DS
            {
                std::array<ID3D11Buffer *, 2> pConstantBuffers
                      = { m_cbSandHeightfield.GetBuffer(), m_cbDomainTerrainSettings.GetBuffer() };
                m_pDeviceContext->DSSetConstantBuffers(
                      0, pConstantBuffers.size(), pConstantBuffers.data());
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_pWrapSamplerState.Get() };
                m_pDeviceContext->DSSetSamplers(0, 1, ppSamplerStateBindTable);
                std::array<ID3D11ShaderResourceView *, 5> srvs
                      = { m_largeScaleDuneModel.GetBedrockHeightmap(),
                            m_largeScaleDuneModel.GetSandHeightmap(),
                            m_smallScaleRippleModel.GetSandHeightmap(),
                            m_largeScaleDuneModel.GetHeightmapNormals(),
                            m_smallScaleRippleModel.GetHeightmapNormals() };
                m_pDeviceContext->DSSetShaderResources(0, srvs.size(), srvs.data());
            }

            // PS
            {
                ID3D11Buffer *pConstantBuffers[1] = { m_cbSandHeightfield.GetBuffer() };
                m_pDeviceContext->PSSetConstantBuffers(0, 1, pConstantBuffers);

                std::array<ID3D11ShaderResourceView *, 9> srvs = { m_pSandAlbedoTextureSRV.Get(),
                    m_pSandNormalTextureSRV.Get(), m_pSandMetalnessTextureSRV.Get(),
                    m_pSandRoughnessTextureSRV.Get(), m_pDesertRockAlbedoTextureSRV.Get(),
                    m_pDesertRockNormalTextureSRV.Get(), m_pDesertRockMetalnessTextureSRV.Get(),
                    m_pDesertRockRoughnessTextureSRV.Get(),
                    m_largeScaleDuneModel.GetObstacleMask() };
                m_pDeviceContext->PSSetShaderResources(0, srvs.size(), srvs.data());

                ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_pWrapSamplerState.Get() };
                m_pDeviceContext->PSSetSamplers(0, 1, ppSamplerStateBindTable);
            }
        }

        // Draw call
        m_pDeviceContext->DrawIndexed(m_tesselatedTerrainIndexCount, 0, 0);

        // Pop State
        {
            // PS
            {
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
                m_pDeviceContext->PSSetSamplers(0, 1, ppSamplerStateBindTable);

                std::array<ID3D11ShaderResourceView *, 9> pSRVs = { nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                m_pDeviceContext->PSSetShaderResources(0, pSRVs.size(), pSRVs.data());

                ID3D11Buffer *pConstantBuffers[1] = { nullptr };
                m_pDeviceContext->PSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // DS
            {
                std::array<ID3D11Buffer *, 2> pConstantBuffers = { nullptr, nullptr };
                m_pDeviceContext->DSSetConstantBuffers(
                      0, pConstantBuffers.size(), pConstantBuffers.data());
                ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
                m_pDeviceContext->DSSetSamplers(0, 1, ppSamplerStateBindTable);
                std::array<ID3D11ShaderResourceView *, 5> srvs
                      = { nullptr, nullptr, nullptr, nullptr, nullptr };
                m_pDeviceContext->DSSetShaderResources(0, srvs.size(), srvs.data());
            }

            // HS
            {
                ID3D11Buffer *pConstantBuffers[1] = { nullptr };
                m_pDeviceContext->HSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // VS
            {
                ID3D11Buffer *pConstantBuffers[1] = { nullptr };
                m_pDeviceContext->DSSetConstantBuffers(0, 1, pConstantBuffers);
            }

            // IA
            {
                ID3D11Buffer *vertexBuffers[1] = { nullptr };
                m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
                m_pDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
            }

            m_shaders["RenderSandTess"].ReleasePipeline(m_pDeviceContext.Get());
        }

        m_pPerf->EndEvent();
    }
}

void Renderer::Render(const RenderingComponent &renderingComponent, const Camera &currentCamera)
{
    const TransformComponent *pTransformComponent
          = g_TransformManager.GetComponent(renderingComponent.m_entity);
    if (!pTransformComponent) {
        FARLOR_LOG_ERROR("Missing transform component");
    }

    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(pTransformComponent->m_scale.x,
          pTransformComponent->m_scale.y, pTransformComponent->m_scale.z);
    DirectX::XMMATRIX rotate
          = DirectX::XMMatrixRotationRollPitchYaw(pTransformComponent->m_rotation.x,
                pTransformComponent->m_rotation.y, pTransformComponent->m_rotation.z);
    DirectX::XMMATRIX translate = DirectX::XMMatrixTranslation(pTransformComponent->m_position.x,
          pTransformComponent->m_position.y, pTransformComponent->m_position.z);

    DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_camProjection);
    DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_camView);

    DirectX::XMMATRIX world = scale * rotate * translate;
    DirectX::XMMATRIX wv = world * view;
    DirectX::XMMATRIX wvp = world * view * proj;

    // Update parameters
    {
        m_transformParams.AccessData().World = DirectX::XMMatrixTranspose(world);
        m_transformParams.AccessData().WorldView = DirectX::XMMatrixTranspose(wv);
        m_transformParams.AccessData().WorldViewProjection = DirectX::XMMatrixTranspose(wvp);
        m_transformParams.Update(m_pDeviceContext.Get());
    }

    // Push State
    {
        const uint32_t numConstBuffers = 2;
        ID3D11Buffer *pConstantBuffers[numConstBuffers]
              = { m_transformParams.GetBuffer(), m_materialParams.GetBuffer() };

        m_pDeviceContext->VSSetConstantBuffers(0, numConstBuffers, pConstantBuffers);
        m_pDeviceContext->PSSetConstantBuffers(0, numConstBuffers, pConstantBuffers);
    }

    // Draw
    {
        m_meshes[renderingComponent.m_mesh].Render(m_pDevice.Get(), m_pDeviceContext.Get());
    }

    // Pop State - Slow?
    {
        const uint32_t numConstBuffers = 2;
        ID3D11Buffer *pConstantBuffers[numConstBuffers] = { nullptr, nullptr };

        m_pDeviceContext->VSSetConstantBuffers(0, numConstBuffers, pConstantBuffers);
        m_pDeviceContext->PSSetConstantBuffers(0, numConstBuffers, pConstantBuffers);
    }
}

void Renderer::RenderLight(const LightComponent &lightComponent, const Camera &currentCamera)
{
    const TransformComponent *pTransformComponent
          = g_TransformManager.GetComponent(lightComponent.m_entity);
    if (!pTransformComponent) {
        FARLOR_LOG_ERROR("Missing transform component");
    }

    m_lightParams.AccessData().m_position = pTransformComponent->m_position;
    m_lightParams.AccessData().m_color = lightComponent.m_color;
    m_lightParams.AccessData().m_direction = lightComponent.m_direction;
    m_lightParams.AccessData().m_lightRange = lightComponent.m_lightRange;
    m_lightParams.Update(m_pDeviceContext.Get());

    DirectX::XMFLOAT4 pos = currentCamera.GetCamPos();
    m_cameraParams.AccessData().m_eyePosition = Vector3(pos.x, pos.y, pos.z);
    m_cameraParams.Update(m_pDeviceContext.Get());

    ID3D11Buffer *pConstantBuffers[2];
    pConstantBuffers[0] = m_lightParams.GetBuffer();
    pConstantBuffers[1] = m_cameraParams.GetBuffer();

    m_pDeviceContext->PSSetConstantBuffers(0, 2, pConstantBuffers);

    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pDeviceContext->Draw(3, 0);
}

void Renderer::TriggerSequenceExport()
{
    // if (m_exportFrameSequenceActive) {
    //     std::cout << "Error: Frame sequence export already active. Ignoring trigger event"
    //               << std::endl;
    //     return;
    // }

    // std::time_t currentTime
    //       = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // char str[26];
    // ctime_s(str, sizeof(str), &currentTime);
    // std::string currentTimeString(str);
    // std::replace(currentTimeString.begin(), currentTimeString.end(), ' ', '_');
    // std::replace(currentTimeString.begin(), currentTimeString.end(), ':', '_');
    // currentTimeString.erase(std::remove(currentTimeString.begin(), currentTimeString.end(), '\n'),
    //       currentTimeString.end());
    // std::cout << "Creating folder called: " << currentTimeString << std::endl;
    // auto frameExportFolder = std::filesystem::current_path() / std::string("frame_exports");
    // if (!std::filesystem::exists(frameExportFolder)) {
    //     std::filesystem::create_directory(frameExportFolder);
    // }

    // // Current frame directory
    // auto currentFrameExportFolder = frameExportFolder / currentTimeString;
    // if (!std::filesystem::exists(currentFrameExportFolder)) {
    //     std::filesystem::create_directory(currentFrameExportFolder);
    // }

    // // Start the export
    // m_exportFrameSequenceActive = true;
    // m_exportFrameSequenceIdx = 0;
    // m_exportFolder = currentFrameExportFolder.string();
}

void Renderer::ResetSimulation()
{
    m_largeScaleDuneModel.Reset(m_pDeviceContext.Get());
    m_smallScaleRippleModel.Reset(m_pDeviceContext.Get());

    FirstFrameSetupWithShaders();

    m_simulationStepCount = 0;
}

static Farlor::Vector4 DeltaTimeToColor(float dt)
{
    static Farlor::Vector3 colors[4] = {
        Farlor::Vector3(0.f, 0.f, 1.f),  // blue
        Farlor::Vector3(0.f, 1.f, 0.f),  // green
        Farlor::Vector3(1.f, 1.f, 0.f),  // yellow
        Farlor::Vector3(1.f, 0.f, 0.f),  // red
    };
    constexpr float dts[] = {
        1.f / 120.f,
        1.f / 60.f,
        1.f / 30.f,
        1.f / 15.f,
    };
    if (dt < dts[0])
        return Farlor::Vector4(colors[0].x, colors[0].y, colors[0].z, 1.f);
    for (size_t i = 1; i < _countof(dts); ++i) {
        if (dt < dts[i]) {
            const float t = (dt - dts[i - 1]) / (dts[i] - dts[i - 1]);
            const Farlor::Vector3 mixedColor = colors[i - 1] * (1.0f - t) + colors[i] * t;
            return Farlor::Vector4(mixedColor.x, mixedColor.y, mixedColor.z, 1.f);
        }
    }
    return Farlor::Vector4(colors[_countof(dts) - 1].x, colors[_countof(dts) - 1].y,
          colors[_countof(dts) - 1].z, 1.f);
}

void Renderer::RenderFrameTimeGraph()
{
    const std::deque<float> &frameTimes = m_engine.GetFrameTimes();
    const float width = ImGui::GetWindowWidth();
    const size_t frameCount = frameTimes.size();
    if (width > 0.f && frameCount > 0) {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 basePos = ImGui::GetCursorScreenPos();
        constexpr float minHeight = 2.f;
        constexpr float maxHeight = 64.f;
        float endX = width;
        constexpr float dtMin = 1.f / 60.f;
        constexpr float dtMax = 1.f / 15.f;
        const float dtMin_Log2 = log2(dtMin);
        const float dtMax_Log2 = log2(dtMax);
        drawList->AddRectFilled(
              basePos, ImVec2(basePos.x + width, basePos.y + maxHeight), 0xFF404040);
        for (size_t frameIndex = 0; frameIndex < frameCount && endX > 0.f; ++frameIndex) {
            const float dt = frameTimes[frameIndex];
            const float dt_log2 = std::log2(dt);
            const float frameWidth = (dt / dtMin) * (width / 100);
            const float frameHeightFactor = (dt_log2 - dtMin_Log2) / (dtMax_Log2 - dtMin_Log2);
            const float frameHeightFactor_Nrm = std::min(std::max(0.f, frameHeightFactor), 1.f);
            const float frameHeight
                  = minHeight * (1.0f - frameHeightFactor_Nrm) + maxHeight * frameHeightFactor_Nrm;
            const float begX = endX - frameWidth;
            const Farlor::Vector4 color = DeltaTimeToColor(dt);
            drawList->AddRectFilled(ImVec2(basePos.x + std::max(0.f, floor(begX)),
                                          basePos.y + maxHeight - frameHeight),
                  ImVec2(basePos.x + ceil(endX), basePos.y + maxHeight),
                  IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255));
            endX = begX;
        }
        ImGui::Dummy(ImVec2(width, maxHeight));
    }
}

void Renderer::LoadMesh(HashedString meshName)
{
    Mesh newMesh;
    std::string modelName = "resources/meshes/";
    modelName += meshName.m_hashedString;
    FARLOR_LOG_INFO("Loading model: {}", modelName)
    Mesh::LoadObjTinyObj(modelName, newMesh);
    newMesh.InitializeBuffers(m_pDevice.Get(), m_pDeviceContext.Get());
    m_meshes[meshName] = newMesh;
}

void Renderer::RegisterGlobalResource(std::string name, std::string resourceType, int widthScale,
      int heightScale, std::string format)
{
    RenderResourceDesc resourceDesc;
    resourceDesc.m_name = name;
    resourceDesc.m_type = Renderer::TypeFromString(resourceType);
    resourceDesc.m_widthScale = widthScale;
    resourceDesc.m_heightScale = heightScale;
    resourceDesc.m_format = Renderer::FormatFromString(format);

    m_globalResourcesDescs.push_back(resourceDesc);
}

void Renderer::CreateGlobalResources()
{
    for (auto &resourceDesc : m_globalResourcesDescs) {
        switch (resourceDesc.m_type) {
            case RenderResourceType::RenderTarget: {
                FARLOR_LOG_INFO("Creating render target")

                D3D11_TEXTURE2D_DESC rtDesc;
                ZeroMemory(&rtDesc, sizeof(rtDesc));

                RenderTarget newRT;

                rtDesc.Width = m_width * resourceDesc.m_widthScale;
                rtDesc.Height = m_height * resourceDesc.m_heightScale;
                rtDesc.MipLevels = 1;
                rtDesc.ArraySize = 1;
                rtDesc.Format = FormatToDXGIFormat(resourceDesc.m_format);
                rtDesc.SampleDesc.Count = 1;
                rtDesc.Usage = D3D11_USAGE_DEFAULT;
                rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                rtDesc.CPUAccessFlags = 0;
                rtDesc.MiscFlags = 0;

                HRESULT result = m_pDevice->CreateTexture2D(&rtDesc, 0, &newRT.m_pRTTexture);
                if (FAILED(result)) {
                    FARLOR_LOG_ERROR(
                          "Failed to create global render target texture: {}", resourceDesc.m_name)
                }

                D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
                ZeroMemory(&rtViewDesc, sizeof(rtViewDesc));
                rtViewDesc.Format = rtDesc.Format;
                rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtViewDesc.Texture2D.MipSlice = 0;
                result = m_pDevice->CreateRenderTargetView(
                      newRT.m_pRTTexture, &rtViewDesc, &newRT.m_pRTView);
                if (FAILED(result)) {
                    FARLOR_LOG_ERROR(
                          "Failed to create global render target view: {}", resourceDesc.m_name)
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC rtSRViewDesc;
                ZeroMemory(&rtSRViewDesc, sizeof(rtSRViewDesc));
                rtSRViewDesc.Format = rtDesc.Format;
                rtSRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                rtSRViewDesc.Texture2D.MostDetailedMip = 0;
                rtSRViewDesc.Texture2D.MipLevels = 1;
                result = m_pDevice->CreateShaderResourceView(
                      newRT.m_pRTTexture, &rtSRViewDesc, &newRT.m_pRTShaderResourceView);
                if (FAILED(result)) {
                    FARLOR_LOG_ERROR(
                          "Failed to create global render target shader resource view: {}",
                          resourceDesc.m_name)
                }

                SetDebugObjectName(newRT.m_pRTTexture, resourceDesc.m_name);
                SetDebugObjectName(newRT.m_pRTView, (resourceDesc.m_name + std::string("RTV")));
                SetDebugObjectName(
                      newRT.m_pRTShaderResourceView, (resourceDesc.m_name + std::string("SRV")));

                FARLOR_LOG_INFO(
                      "Successfuly created global render target: {}", resourceDesc.m_name);

                m_renderTargets.insert(
                      std::pair<std::string, RenderTarget>(resourceDesc.m_name, newRT));

            } break;

            default: {
                FARLOR_LOG_WARNING("Incorrect resource type, cannot create")
            } break;
        }
    }
}

void Renderer::RegisterShader(const std::string &filename, const std::string &name,
      const IncludedShaders includedShaders, const std::vector<std::string> &defines)
{
    Shader newShader(filename, includedShaders, defines);
    m_shaders[name] = newShader;
}

void Renderer::CreateShaders()
{
    for (auto &shaderPair : m_shaders) {
        shaderPair.second.Initialize(m_pDevice.Get(), m_pDeviceContext.Get());
    }
}


RenderingComponent *Renderer::RegisterEntityAsRenderable(const Entity entity)
{
    RenderingComponent newComponent;
    newComponent.m_entity = entity;
    m_renderingComponents.emplace(entity, newComponent);
    return &m_renderingComponents[entity];
}
RenderingComponent *Renderer::RegisterEntityAsRenderable(
      const Entity entity, RenderingComponent renderingComponent)
{
    renderingComponent.m_entity = entity;
    LoadMesh(renderingComponent.m_mesh);
    m_renderingComponents.emplace(entity, renderingComponent);
    return &m_renderingComponents[entity];
}

LightComponent *Renderer::RegisterEntityAsLight(const Entity entity)
{
    LightComponent newComponent;
    newComponent.m_entity = entity;
    m_lightComponents.emplace(entity, newComponent);
    return &m_lightComponents[entity];
}
LightComponent *Renderer::RegisterEntityAsLight(const Entity entity, LightComponent lightComponent)
{
    lightComponent.m_entity = entity;
    m_lightComponents.emplace(entity, lightComponent);
    return &m_lightComponents[entity];
}

RenderingComponent *Renderer::GetRenderingComponent(const Entity entity)
{
    auto entry = m_renderingComponents.find(entity);
    if (entry == m_renderingComponents.end()) {
        return nullptr;
    }
    return &entry->second;
}

LightComponent *Renderer::GetLightComponent(const Entity entity)
{
    auto entry = m_lightComponents.find(entity);
    if (entry == m_lightComponents.end()) {
        return nullptr;
    }
    return &entry->second;
}

RenderResourceType Renderer::TypeFromString(std::string strType)
{
    return s_stringToTypeMapping[strType];
}

RenderResourceFormat Renderer::FormatFromString(std::string strFormat)
{
    return s_stringToFormatMapping[strFormat];
}

DXGI_FORMAT Renderer::FormatToDXGIFormat(RenderResourceFormat format)
{
    return s_formatToDXGIFormatMapping[format];
}
}
