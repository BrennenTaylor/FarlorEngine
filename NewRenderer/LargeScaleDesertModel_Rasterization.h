#pragma once

#include "ManagedConstantBuffer.h"
#include "ManagedTexture2D.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl/client.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <FMath/FMath.h>

#include <memory>
#include <vector>

namespace Farlor {

class Renderer;

class LargeScaleDesertModel_Rasterization {
   public:
    struct SandTransportPrimitiveVertex {
        Farlor::Vector4 data_0;       // v0,.v1
        Farlor::Vector4 data_1;       // v2, v3
        Farlor::Vector4 data_2;       // v4, v5
        Farlor::Vector4 data_3;       // v6, v5
        Farlor::Vector4 extraParams;  // m0, m1, m2, m3
    };

    struct cbDesertSimParams {
        uint32_t m_gridResolution;
        float m_desertCellSize;
        float m_desertBlockHeight;
        float _pad1;
    };

    struct cbWindGenerationParams {
        float m_windX;
        float m_windZ;
        float _pad0;
        float _pad1;
    };

    struct cbSandTransportParams {
        uint32_t maxTransportSteps;
        uint32_t targetBlocksToMove;
        float _pad0;
        float _pad1;
    };

    struct cbPerObject {
        DirectX::XMMATRIX WorldMatrix;
        DirectX::XMMATRIX InvTransposeWorldMatrix;
        DirectX::XMMATRIX ViewProjection;
    };

   public:
    LargeScaleDesertModel_Rasterization(Renderer &renderer, uint32_t gridResolution,
          float cellSizeMeters, std::string simulationId);
    void SetupDesertSimulation(ID3D11Device *const pDevice,
          ID3D11DeviceContext *const pDeviceContext, const std::vector<float> &initialSandHeights,
          const std::vector<float> &initialBedrockHeights,
          const std::vector<float> &initialVegitation);
    bool StepDesertSimulation(
          ID3D11DeviceContext *const pDeviceContext, ID3DUserDefinedAnnotation *const pPerf);

    void FirstFrameSetup(
          ID3D11DeviceContext *const pDeviceContext, ID3DUserDefinedAnnotation *const pPerf);

    void Gui();

    uint32_t GetGridResolution() const { return m_gridResolution; }
    float GetCellSizeMeters() const { return m_cellSizeMeters; }

    ID3D11ShaderResourceView *GetWindTexture() { return m_windTexture.GetSRV(); }
    ID3D11ShaderResourceView *GetWindShadow() { return m_windShadow.GetSRV(); }
    ID3D11ShaderResourceView *GetSandHeightmap() { return m_horizontalBlurFinal.GetSRV(); }
    ID3D11ShaderResourceView *GetBedrockHeightmap() { return m_bedrockHeightmap.GetSRV(); }
    ID3D11ShaderResourceView *GetHeightmapNormals() { return m_heightmapNormals.GetSRV(); }
    ID3D11ShaderResourceView *GetObstacleMask() { return m_obstacleMask.GetSRV(); }

    void Reset(ID3D11DeviceContext *const pDeviceContext)
    {
        pDeviceContext->CopyResource(
              m_bedrockBlocksRead.GetTexture(), m_bedrockBlocksInitial.GetTexture());
        pDeviceContext->CopyResource(
              m_bedrockBlocksWrite.GetTexture(), m_bedrockBlocksInitial.GetTexture());

        pDeviceContext->CopyResource(
              m_sandBlocksRead.GetTexture(), m_sandBlocksInitial.GetTexture());
        pDeviceContext->CopyResource(
              m_sandBlocksWrite.GetTexture(), m_sandBlocksInitial.GetTexture());

        pDeviceContext->CopyResource(
              m_bedrockHeightmap.GetTexture(), m_bedrockBlocksInitial.GetTexture());

        pDeviceContext->CopyResource(
              m_combinedHeightmap.GetTexture(), m_combinedHeightmapInitialValues.GetTexture());
        pDeviceContext->CopyResource(m_previousCombinedHeightmap.GetTexture(),
              m_combinedHeightmapInitialValues.GetTexture());
        pDeviceContext->CopyResource(
              m_horizontalBlurFinal.GetTexture(), m_combinedHeightmapInitialValues.GetTexture());

        m_resetRequested = false;
    }

    bool ResetRequested() const { return m_resetRequested; }
    void UpdateCachedTerrainValues(ID3D11DeviceContext *const pDeviceContext);
    const std::vector<float> &GetCachedTerrainValues() const
    {
        return m_latestCachedHeightmapValues;
    }

    bool WasTerrainUpdated() { return m_cachedTerrainUpdated; }
    void HandleTerrainUpdated() { m_cachedTerrainUpdated = false; }

   private:
    void DoBedrockCascadePass(
          ID3D11DeviceContext *pDeviceContext, ID3DUserDefinedAnnotation *pPerf);
    void DoSandCascadePass(ID3D11DeviceContext *pDeviceContext, ID3DUserDefinedAnnotation *pPerf);

   private:
    Renderer &m_renderer;
    std::string m_simulationId = "";

    uint32_t m_gridResolution = 1;
    float m_cellSizeMeters = 1.0f;
    float m_desertSimulationBlockHeight = 1.0f / m_cellSizeMeters;  // In meters


    uint32_t m_numSandCascadePasses = 50;
    uint32_t m_numGaussianHeightmapBlurPasses = 1;

    Farlor::Vector2 m_baseWindDirection = Farlor::Vector2(1.0f, 0.0f);
    float m_baseWindSpeed = 1.0f;

    bool m_stepSimulation = false;

    bool m_resetRequested = false;
    bool m_updateCachedTerrainRequested = false;
    bool m_cachedTerrainUpdated = false;

    std::vector<float> m_latestCachedHeightmapValues;

    // Resources for Sand Sim
    ManagedTexture2D<uint32_t> m_desertRandomStatesTexture;

    ManagedTexture2DStaging<int32_t> m_bedrockBlocksInitial;
    ManagedTexture2D<int32_t> m_bedrockBlocksRead;
    ManagedTexture2D<int32_t> m_bedrockBlocksWrite;

    ManagedTexture2D<float> m_bedrockHeightmapInitial;
    ManagedTexture2D<float> m_bedrockHeightmap;

    ManagedTexture2DStaging<int32_t> m_sandBlocksInitial;
    ManagedTexture2D<int32_t> m_sandBlocksRead;
    ManagedTexture2D<int32_t> m_sandBlocksWrite;

    ManagedTexture2D<float> m_verticalBlurRadius200;
    ManagedTexture2D<float> m_horizontalBlurRadius200;
    ManagedTexture2D<float> m_verticalBlurRadius50;
    ManagedTexture2D<float> m_horizontalBlurRadius50;

    ManagedTexture2D<float> m_verticalBlurFinal;
    ManagedTexture2D<float> m_horizontalBlurFinal;

    ManagedTexture2D<float> m_combinedHeightmap;
    ManagedTexture2D<float> m_previousCombinedHeightmap;
    ManagedTexture2D<float> m_combinedHeightmapInitialValues;

    ManagedTexture2D<Farlor::Vector3> m_heightmapNormals;

    ManagedTexture2D<Farlor::Vector2> m_gradientMapRadius200;
    ManagedTexture2D<Farlor::Vector2> m_gradientMapRadius50;

    ManagedTexture2D<uint32_t> m_obstacleMask;

    ManagedTexture2D<Farlor::Vector2> m_windTexture;
    ManagedTexture2D<float> m_windShadow;
    ManagedTexture2D<float> m_vegitationMask;
    ManagedTexture2DStaging<float> m_heightmapReadBack;

    ManagedConstantBuffer<cbDesertSimParams> m_desertSimParams;
    ManagedConstantBuffer<cbWindGenerationParams> m_windGenerationParams;
    ManagedConstantBuffer<cbSandTransportParams> m_sandTransportParams;

    uint32_t m_sandTransportPrimitivesVertexCount = 0;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pSandTransportPrimitivesVertices = nullptr;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pSandTransportPrimitivesUAV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSandTransportPrimitivesSRV = nullptr;
    //     ManagedRenderTargetTexture2D<float> m_transportedSandTexture;
    ManagedRenderTargetTexture2DMS<float> m_transportedSandTexture;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pSandTransportRS = nullptr;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_pSandTransportBS = nullptr;


    ManagedConstantBuffer<cbPerObject> m_transportedSandObjectParams;
};

}