#pragma once

#include "FMath/Vector4.h"
#include <d3d11.h>
#include <unordered_map>
#include <wrl/client.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "RenderingComponent.h"
#include "LightComponents.h"

#include "RenderResourceDesc.h"
#include "RenderTarget.h"

#include "DesertModels.h"

#include "D3D11/D3D11_Utils.h"

#include "../Core/Window.h"
#include "../ECS/Entity.h"
#include "Mesh.h"
#include "CBs.h"
#include "Shader.h"

#include "agz/smaa/smaa.h"

#undef min
#undef max
#include <ImfArray.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfStringAttribute.h>

// Desert Stuff
#include "ScalarField2D.h"

#include "Camera.h"

#include <DirectXMath.h>

#include <map>
#include <memory>
#include <string>
#include <deque>

namespace Farlor {

class Engine;

class Renderer {
    struct cbPerObject {
        DirectX::XMMATRIX WorldMatrix;
        DirectX::XMMATRIX InvTransposeWorld;
        DirectX::XMMATRIX VP;
    };

    struct cbHullTerrainLOD {
        DirectX::XMFLOAT4 m_cameraPos;
        Farlor::Vector2 m_minMaxDistance;
        Farlor::Vector2 m_minMaxLOD;
    };

    struct cbDomainTerrainSettings {
        Farlor::Vector4 m_heightmapScaleFactors;
    };

   public:
    Renderer(const Engine &engine);

    void Initialize(const GameWindow &gameWindow);
    void Shutdown();

    void Render(const Camera &currentCamera, const float dt);

    void RenderSand(const DirectX::XMMATRIX &world, const DirectX::XMMATRIX &wvp,
          const DirectX::XMMATRIX &invTransposeWorld);

    void RenderSandTess(const DirectX::XMMATRIX &world, const DirectX::XMMATRIX &wvp,
          const DirectX::XMMATRIX &invTransposeWorld, const DirectX::XMFLOAT4 &cameraPos);

    void Render(const RenderingComponent &renderingComponent, const Camera &currentCamera);
    void RenderLight(const LightComponent &lightComponent, const Camera &currentCamera);

    void RenderFrameTimeGraph();

    RenderingComponent *RegisterEntityAsRenderable(const Entity entity);
    RenderingComponent *RegisterEntityAsRenderable(
          const Entity entity, RenderingComponent renderingComponent);

    LightComponent *RegisterEntityAsLight(const Entity entity);
    LightComponent *RegisterEntityAsLight(const Entity entity, LightComponent lightComponent);

    RenderingComponent *GetRenderingComponent(const Entity entity);
    LightComponent *GetLightComponent(const Entity entity);

    void RegisterGlobalResource(std::string name, std::string resourceType, int widthScale,
          int heightScale, std::string format);
    void CreateGlobalResources();
    void RegisterShader(const std::string &filename, const std::string &name,
          const IncludedShaders includedShaders, const std::vector<std::string> &defines);
    void CreateShaders();

    ID3D11SamplerState *GetWrapSamplerState() { return m_pWrapSamplerState.Get(); }
    ID3D11SamplerState *GetClampSamplerState() { return m_pClampSamplerState.Get(); }

    const std::map<std::string, Shader> &AccessShaders() const { return m_shaders; }

    ShaderBlobManager &GetShaderBlobManager() { return m_shaderBlobManager; }

    static RenderResourceType TypeFromString(std::string strType);
    static RenderResourceFormat FormatFromString(std::string strFormat);

    DXGI_FORMAT FormatToDXGIFormat(RenderResourceFormat format);

    void TriggerSequenceExport();

    void ResetSimulation();

    //     LargeScaleDesertModel &LargeDesertSimulation() { return m_largeScaleDuneModel; }
    LargeScaleDesertModel_Rasterization &LargeDesertSimulation() { return m_largeScaleDuneModel; }

   private:
    const Engine &m_engine;

    void InitializeDXGI(std::vector<IDXGIAdapter *> &adapters);

    void LoadMesh(HashedString meshName);

    void SetupSMAAResources();
    void SetupDesertSimulation(
          const float rMin, const float rMax, const Imf::Array2D<Imf::Rgba> &defaultValues);
    bool StepDesertSimulation();

    void ExportFrame();

    void CreateVegetationResources();

    void FirstFrameSetupWithShaders();

    void DoBedrockCascadePass();
    void DoSandCascadePass();

   public:
    std::unordered_map<Entity, RenderingComponent> m_renderingComponents;
    std::unordered_map<Entity, LightComponent> m_lightComponents;

    std::vector<RenderResourceDesc> m_globalResourcesDescs;

   private:
    ShaderBlobManager m_shaderBlobManager;
    std::map<std::string, Shader> m_shaders;
    std::map<HashedString, Mesh> m_meshes;

    DirectX::XMFLOAT4X4 m_camView;
    DirectX::XMFLOAT4X4 m_camProjection;
    uint32_t m_width = 800;
    uint32_t m_height = 600;

    float m_minTessLOD = 1.0f;
    float m_maxTessLOD = 1.0f;

    D3D11_VIEWPORT m_windowClientViewport;
    D3D11_VIEWPORT m_luminanceViewport;

    std::map<std::string, RenderTarget> m_renderTargets;

    // Debug only
    Microsoft::WRL::ComPtr<IDXGIDebug1> m_dxgiDebug1 = nullptr;
    Microsoft::WRL::ComPtr<IDXGIInfoQueue> m_dxgiInfoQueue = nullptr;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_pDxgiFactory = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Debug> m_d3d11Debug = nullptr;
    Microsoft::WRL::ComPtr<ID3D11InfoQueue> m_pD3D11InfoQueue = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Device1> m_pDevice = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_pDeviceContext = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
    Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_pPerf = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pBackBuffer = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV = nullptr;

    // Represents a depth stencil buffer the size of the window
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pWindowDSB = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pWindowDSV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pCullRS = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pNoCullRS = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pWireframeRS = nullptr;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pWrapSamplerState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pClampSamplerState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pEnableDepthStencilState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDisableDepthStencilState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_pAdditiveBlendState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_pNoBlendState = nullptr;

    ManagedTexture2DMipEnabled<float> m_luminanceTexture;

    ManagedConstantBuffer<cbTransforms> m_transformParams;
    ManagedConstantBuffer<cbMatProperties> m_materialParams;
    ManagedConstantBuffer<cbLightProperties> m_lightParams;
    ManagedConstantBuffer<cbCameraParams> m_cameraParams;

    static std::map<std::string, RenderResourceType> s_stringToTypeMapping;
    static std::map<std::string, RenderResourceFormat> s_stringToFormatMapping;

    static std::map<RenderResourceFormat, DXGI_FORMAT> s_formatToDXGIFormatMapping;

    // SMAA
    std::unique_ptr<agz::smaa::SMAA> m_upSMAA = nullptr;

    //     LargeScaleDesertModel m_largeScaleDuneModel;
    LargeScaleDesertModel_Rasterization m_largeScaleDuneModel;
    SmallScaleDesertModel m_smallScaleRippleModel;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pEdgeTargetSMAA = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pEdgeTargetSRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pEdgeTargetRTV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pWeightTargetSMAA = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pWeightTargetSRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pWeightTargetRTV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pAATargetSMAA = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pAATargetSRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pAATargetRTV = nullptr;

    // Resources for Sand Sim
    Microsoft::WRL::ComPtr<ID3D11Resource> m_pSandAlbedoTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSandAlbedoTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pSandMetalnessTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSandMetalnessTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pSandNormalTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSandNormalTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pSandRoughnessTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSandRoughnessTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pDesertRockAlbedoTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDesertRockAlbedoTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pDesertRockMetalnessTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDesertRockMetalnessTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pDesertRockNormalTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDesertRockNormalTextureSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Resource> m_pDesertRockRoughnessTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDesertRockRoughnessTextureSRV = nullptr;

    // Modifiable simulation related parameters
   private:
    // Desert Field
    Farlor::Vector3 m_desertTranslation = Farlor::Vector3(0.0f, 0.0f, 0.0f);
    Farlor::Vector3 m_desertRotation = Farlor::Vector3(0.0f, 0.0f, 0.0f);
    Farlor::Vector3 m_desertScale = Farlor::Vector3(1024.0f, 1.0f, 1024.0f);

   private:
    uint32_t m_simulationStepCount = 0;

    bool m_enableSMAA = true;

    // Default Sand Heightfield
    uint32_t m_sandHeightfieldVertexCount = 0;
    uint32_t m_sandHeightfieldIndexCount = 0;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pSandHeightfieldVertexBuffer = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pSandHeightfieldIndexBuffer = nullptr;

    // Tesselated Terrain Heightfield
    uint32_t m_tesselatedTerrainVertexCount = 0;
    uint32_t m_tesselatedTerrainIndexCount = 0;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pTesselatedTerrainVertexBuffer = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pTesselatedTerrainIndexBuffer = nullptr;


    ManagedConstantBuffer<Renderer::cbPerObject> m_cbSandHeightfield;
    ManagedConstantBuffer<Renderer::cbHullTerrainLOD> m_cbHullTerrainLOD;
    ManagedConstantBuffer<Renderer::cbDomainTerrainSettings> m_cbDomainTerrainSettings;
};
}
