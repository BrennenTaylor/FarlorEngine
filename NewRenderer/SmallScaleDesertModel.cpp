#include "DesertModels.h"
#include <d3d11.h>

#include "DebugNameUtil.h"

#include <assert.h>
#include <d3d11_1.h>
#include <dxgiformat.h>
#include <string>
#include <winerror.h>

#include "../Util/Logger.h"

#include "Renderer.h"

#include <imgui.h>

#include <array>

namespace Farlor {

SmallScaleDesertModel::SmallScaleDesertModel(
      Renderer &renderer, uint32_t gridResolution, float cellSizeMeters, std::string simulationId)
    : m_renderer(renderer)
    , m_simulationId(simulationId)
    , m_gridResolution(gridResolution)
    , m_cellSizeMeters(cellSizeMeters)
    , m_desertSimulationBlockHeight(m_cellSizeMeters / 1024.0f)
    , m_desertRandomStatesTexture(
            "SmallScale_DesertRandomStates", gridResolution, gridResolution, DXGI_FORMAT_R32_UINT)
    , m_bedrockBlocksInitial(
            "SmallScale_BedrockBlocksInitial", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_bedrockBlocksRead(
            "SmallScale_BedrockBlocksRead", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_bedrockBlocksWrite(
            "SmallScale_BedrockBlocksWrite", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_sandBlocksInitial(
            "SmallScale_SandBlocksInitial", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_sandBlocksRead(
            "SmallScale_SandBlocksRead", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_sandBlocksWrite(
            "SmallScale_SandBlocksWrite", gridResolution, gridResolution, DXGI_FORMAT_R32_SINT)
    , m_verticalBlurRadius200("SmallScale_VerticalBlurRadius200", gridResolution, gridResolution,
            DXGI_FORMAT_R32_FLOAT)
    , m_horizontalBlurRadius200("SmallScale_HorizontalBlurRadius200", gridResolution,
            gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_verticalBlurRadius50("SmallScale_VerticalBlurRadius50", gridResolution, gridResolution,
            DXGI_FORMAT_R32_FLOAT)
    , m_horizontalBlurRadius50("SmallScale_HorizontalBlurRadius50", gridResolution, gridResolution,
            DXGI_FORMAT_R32_FLOAT)
    , m_verticalBlurFinal(
            "SmallScale_VerticalBlurFinal", gridResolution, gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_horizontalBlurFinal(
            "SmallScale_HorizontalBlurFinal", gridResolution, gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_combinedHeightmap(
            "SmallScale_CombinedHeightmap", gridResolution, gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_previousCombinedHeightmap("SmallScale_PreviousCombinedHeightmap", gridResolution,
            gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_combinedHeightmapInitialValues("SmallScale_CombinedHeightmapInitialValues", gridResolution,
            gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_heightmapNormals("SmallScale_HeightmapNormals", gridResolution, gridResolution,
            DXGI_FORMAT_R32G32B32A32_FLOAT)
    , m_gradientMapRadius200("SmallScale_GradientMapRadius200", gridResolution, gridResolution,
            DXGI_FORMAT_R32G32_FLOAT)
    , m_gradientMapRadius50("SmallScale_GradientMapRadius50", gridResolution, gridResolution,
            DXGI_FORMAT_R32G32_FLOAT)
    , m_windTexture(
            "SmallScale_WindTexture", gridResolution, gridResolution, DXGI_FORMAT_R32G32_FLOAT)
    , m_windShadow("SmallScale_WindShadow", gridResolution, gridResolution, DXGI_FORMAT_R32_FLOAT)
    , m_vegitationMask(
            "SmallScale_VegitationMask", gridResolution, gridResolution, DXGI_FORMAT_R32_FLOAT)
{
}

void SmallScaleDesertModel::SetupDesertSimulation(ID3D11Device *const pDevice,
      ID3D11DeviceContext *const pDeviceContext, const std::vector<float> &initialSandHeights,
      const std::vector<float> &initialBedrockHeights, const std::vector<float> &initialVegetation)
{
    // First, we want to create the data
    std::vector<int32_t> bedrockHeights(initialBedrockHeights.size());
    for (int i = 0; i < initialBedrockHeights.size(); i++) {
        bedrockHeights[i] = initialBedrockHeights[i] / m_desertSimulationBlockHeight;
    }

    std::vector<int32_t> sandHeights(initialSandHeights.size());
    for (int i = 0; i < initialSandHeights.size(); i++) {
        sandHeights[i] = initialSandHeights[i] / m_desertSimulationBlockHeight;
    }

    // Combined Height Map
    std::vector<float> initialCombinedHeightmapValues(m_gridResolution * m_gridResolution);
    for (int i = 0; i < (m_gridResolution * m_gridResolution); i++) {
        initialCombinedHeightmapValues[i] = initialSandHeights[i] + initialBedrockHeights[i];
    }

    // Create desert sim params constant buffer
    m_desertSimParams.Initialize(pDevice);
    m_windGenerationParams.Initialize(pDevice);
    m_sandTransportParams.Initialize(pDevice);

    m_desertRandomStatesTexture.Initialize(pDevice);
    m_bedrockBlocksInitial.Initialize(pDevice, bedrockHeights.data());
    m_bedrockBlocksRead.Initialize(pDevice, bedrockHeights.data());
    m_bedrockBlocksWrite.Initialize(pDevice, bedrockHeights.data());
    m_sandBlocksInitial.Initialize(pDevice, sandHeights.data());
    m_sandBlocksRead.Initialize(pDevice, sandHeights.data());
    m_sandBlocksWrite.Initialize(pDevice, sandHeights.data());
    m_verticalBlurRadius200.Initialize(pDevice);
    m_horizontalBlurRadius200.Initialize(pDevice);
    m_verticalBlurRadius50.Initialize(pDevice);
    m_horizontalBlurRadius50.Initialize(pDevice);
    m_verticalBlurFinal.Initialize(pDevice);
    m_horizontalBlurFinal.Initialize(pDevice);

    m_combinedHeightmap.Initialize(pDevice, initialCombinedHeightmapValues.data());
    m_previousCombinedHeightmap.Initialize(pDevice, initialCombinedHeightmapValues.data());
    m_combinedHeightmapInitialValues.Initialize(pDevice, initialCombinedHeightmapValues.data());

    m_heightmapNormals.Initialize(pDevice);

    m_gradientMapRadius200.Initialize(pDevice);
    m_gradientMapRadius50.Initialize(pDevice);

    m_windTexture.Initialize(pDevice);
    m_windShadow.Initialize(pDevice);
    m_vegitationMask.Initialize(pDevice, initialVegetation.data());

    // Lets update the desert sim params before anything!
    m_desertSimParams.AccessData().m_gridResolution = m_gridResolution;
    m_desertSimParams.AccessData().m_desertCellSize = m_cellSizeMeters;
    m_desertSimParams.AccessData().m_desertBlockHeight = m_desertSimulationBlockHeight;
    m_desertSimParams.Update(pDeviceContext);

    Farlor::Vector2 windDir = Farlor::Vector2(1.0, 0.0).Normalized() * m_baseWindSpeed;
    m_windGenerationParams.AccessData().m_windX = windDir.x;
    m_windGenerationParams.AccessData().m_windZ = windDir.y;
    m_windGenerationParams.Update(pDeviceContext);

    m_sandTransportParams.AccessData().maxTransportSteps = 10;
    m_sandTransportParams.AccessData().targetBlocksToMove = 100;
    m_sandTransportParams.Update(pDeviceContext);

    Reset(pDeviceContext);
}

void SmallScaleDesertModel::FirstFrameSetup(
      ID3D11DeviceContext *const pDeviceContext, ID3DUserDefinedAnnotation *const pPerf)
{
    auto iter = m_renderer.AccessShaders().find("SmallScale/InitializeDesertRandomStates");
    assert((iter != m_renderer.AccessShaders().end())
          && "Shader Lookup Failed: SmallScale/InitializeDesertRandomStates");
    // Push State
    {
        iter->second.SetPipeline(pDeviceContext);
        std::array<ID3D11UnorderedAccessView *, 1> uavBindTable
              = { m_desertRandomStatesTexture.GetUAV() };
        pDeviceContext->CSSetUnorderedAccessViews(
              0, uavBindTable.size(), uavBindTable.data(), nullptr);
    }

    // Dispatch
    {
        const uint32_t xThreads = m_gridResolution / 32;
        const uint32_t yThreads = m_gridResolution / 32;
        const uint32_t zThreads = 1;
        pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
    }

    // Pop State
    {
        std::array<ID3D11UnorderedAccessView *, 1> uavBindTable = { nullptr };
        pDeviceContext->CSSetUnorderedAccessViews(
              0, uavBindTable.size(), uavBindTable.data(), nullptr);
        iter->second.ReleasePipeline(pDeviceContext);
    }

    // Finally, Generate the heightmap normals
    {
        pPerf->BeginEvent(L"Heightmap Normals");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/GenerateHeightmapNormals")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_heightmapNormals.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_horizontalBlurFinal.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);

            m_renderer.AccessShaders()
                  .at("SmallScale/GenerateHeightmapNormals")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }
}

bool SmallScaleDesertModel::StepDesertSimulation(
      ID3D11DeviceContext *const pDeviceContext, ID3DUserDefinedAnnotation *const pPerf)
{
    if (m_resetRequested) {
        pPerf->BeginEvent(L"Reset Event");
        Reset(pDeviceContext);
        pPerf->EndEvent();
    }

    if (!m_stepSimulation) {
        return true;
    }

    // Lets update the desert sim params before anything!
    // NOTE: This is required for rendering the heightmap
    m_desertSimParams.AccessData().m_gridResolution = m_gridResolution;
    m_desertSimParams.AccessData().m_desertCellSize = m_cellSizeMeters;
    m_desertSimParams.AccessData().m_desertBlockHeight = m_desertSimulationBlockHeight;
    m_desertSimParams.Update(pDeviceContext);

    Farlor::Vector2 windDir = Farlor::Vector2(1.0, 0.0).Normalized() * m_baseWindSpeed;
    m_windGenerationParams.AccessData().m_windX = windDir.x;
    m_windGenerationParams.AccessData().m_windZ = windDir.y;
    m_windGenerationParams.Update(pDeviceContext);

    m_sandTransportParams.Update(pDeviceContext);

    {
        pPerf->BeginEvent(L"Vertical Blur Radius 200");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalRadius200")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_verticalBlurRadius200.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1]
                  = { m_previousCombinedHeightmap.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalRadius200")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Final Blur Radius 200");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalRadius200")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1]
                  = { m_horizontalBlurRadius200.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_verticalBlurRadius200.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalRadius200")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Vertical Blur Radius 50");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalRadius50")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_verticalBlurRadius50.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1]
                  = { m_previousCombinedHeightmap.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalRadius50")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Final Blur Radius 50");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalRadius50")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_horizontalBlurRadius50.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_verticalBlurRadius50.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalRadius50")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Gradient map Radius 200 update");

        // Push State
        {
            m_renderer.AccessShaders().at("SmallScale/SandGradientGen").SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_gradientMapRadius200.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_horizontalBlurRadius200.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerState[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerState);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerState[1] = { nullptr };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerState);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandGradientGen")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Gradient map Radius 50 update");

        // Push State
        {
            m_renderer.AccessShaders().at("SmallScale/SandGradientGen").SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_gradientMapRadius50.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_horizontalBlurRadius50.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerState[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerState);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerState[1] = { nullptr };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerState);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandGradientGen")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    // Wind Texture step
    {
        pPerf->BeginEvent(L"Wind Map Update");

        // Push State
        {
            m_renderer.AccessShaders().at("LargeScale/WindTextureGen").SetPipeline(pDeviceContext);
            // Completely overwrites the wind texture, no need to clear!
            std::array<ID3D11UnorderedAccessView *, 1> uavs = { m_windTexture.GetUAV() };
            std::array<ID3D11ShaderResourceView *, 2> srvs
                  = { m_previousCombinedHeightmap.GetSRV(), m_gradientMapRadius50.GetSRV() };
            std::array<ID3D11Buffer *, 2> cbs
                  = { m_desertSimParams.GetBuffer(), m_windGenerationParams.GetBuffer() };
            pDeviceContext->CSSetShaderResources(0, srvs.size(), srvs.data());
            pDeviceContext->CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);
            pDeviceContext->CSSetConstantBuffers(0, cbs.size(), cbs.data());
        }

        // Dispatch
        {
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            std::array<ID3D11UnorderedAccessView *, 1> uavs = { nullptr };
            std::array<ID3D11ShaderResourceView *, 2> srvs = { nullptr, nullptr };
            std::array<ID3D11Buffer *, 2> cbs = { nullptr, nullptr };
            pDeviceContext->CSSetShaderResources(0, srvs.size(), srvs.data());
            pDeviceContext->CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);
            pDeviceContext->CSSetConstantBuffers(0, cbs.size(), cbs.data());
            m_renderer.AccessShaders()
                  .at("LargeScale/WindTextureGen")
                  .ReleasePipeline(pDeviceContext);
        }
        pPerf->EndEvent();
    }

    // Wind Shadow step
    {
        pPerf->BeginEvent(L"Window Shadow Map Gen");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/WindShadowTextureGen")
                  .SetPipeline(pDeviceContext);

            ID3D11ShaderResourceView *ppSRViewBindTable[2]
                  = { m_previousCombinedHeightmap.GetSRV(), m_windTexture.GetSRV() };
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_windShadow.GetUAV() };
            ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_renderer.GetWrapSamplerState() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };

            pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);
            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStateBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
        }

        // Dispatch
        {
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11ShaderResourceView *ppSRViewBindTable[2] = { nullptr, nullptr };
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };

            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStateBindTable);
            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);

            m_renderer.AccessShaders()
                  .at("SmallScale/WindShadowTextureGen")
                  .ReleasePipeline(pDeviceContext);
        }
        pPerf->EndEvent();
    }

    // Sand transport step
    {
        pPerf->BeginEvent(L"Sand Transport Step");

        // Push State
        {
            m_renderer.AccessShaders().at("SmallScale/SandTransport").SetPipeline(pDeviceContext);
            std::array<ID3D11ShaderResourceView *, 5> srvs
                  = { m_sandBlocksRead.GetSRV(), m_bedrockBlocksRead.GetSRV(),
                        m_windTexture.GetSRV(), m_windShadow.GetSRV(), m_vegitationMask.GetSRV() };
            std::array<ID3D11UnorderedAccessView *, 3> uavs
                  = { m_desertRandomStatesTexture.GetUAV(), m_sandBlocksWrite.GetUAV(),
                        m_bedrockBlocksWrite.GetUAV() };
            std::array<ID3D11SamplerState *, 1> samplers = { m_renderer.GetWrapSamplerState() };
            std::array<ID3D11Buffer *, 2> cbs
                  = { m_desertSimParams.GetBuffer(), m_sandTransportParams.GetBuffer() };
            pDeviceContext->CSSetShaderResources(0, srvs.size(), srvs.data());
            pDeviceContext->CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);
            pDeviceContext->CSSetSamplers(0, samplers.size(), samplers.data());
            pDeviceContext->CSSetConstantBuffers(0, cbs.size(), cbs.data());
        }

        // Dispatch
        {
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            std::array<ID3D11ShaderResourceView *, 5> srvs
                  = { nullptr, nullptr, nullptr, nullptr, nullptr };
            std::array<ID3D11UnorderedAccessView *, 3> uavs = { nullptr, nullptr, nullptr };
            std::array<ID3D11SamplerState *, 1> samplers = { nullptr };
            std::array<ID3D11Buffer *, 2> cbs = { nullptr, nullptr };
            pDeviceContext->CSSetShaderResources(0, srvs.size(), srvs.data());
            pDeviceContext->CSSetUnorderedAccessViews(0, uavs.size(), uavs.data(), nullptr);
            pDeviceContext->CSSetSamplers(0, samplers.size(), samplers.data());
            pDeviceContext->CSSetConstantBuffers(0, cbs.size(), cbs.data());
            m_renderer.AccessShaders()
                  .at("SmallScale/SandTransport")
                  .ReleasePipeline(pDeviceContext);
        }
        pPerf->EndEvent();
    }

    // At this point, we have that the up to date values are in sand write
    pDeviceContext->CopyResource(m_sandBlocksRead.GetTexture(), m_sandBlocksWrite.GetTexture());

    {
        pPerf->BeginEvent(L"Full Sand Cascade");
        for (uint32_t i = 0; i < m_numSandCascadePasses; ++i) {
            // Generate the combined heightmap for previous frame
            DoSandCascadePass(pDeviceContext, pPerf);
        }
        pPerf->EndEvent();
    }

    // Generate the combined heightmap
    {
        pPerf->BeginEvent(L"Generate Combined Heightmap");
        m_renderer.AccessShaders()
              .at("SmallScale/GenerateCombinedHeightmap")
              .SetPipeline(pDeviceContext);

        {
            // Bind
            {
                ID3D11ShaderResourceView *ppSRViewBindTable[2]
                      = { m_bedrockBlocksRead.GetSRV(), m_sandBlocksRead.GetSRV() };
                ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_combinedHeightmap.GetUAV() };
                ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
                pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            }

            // Dispatch
            {
                const uint32_t xThreads = m_gridResolution / 32;
                const uint32_t yThreads = m_gridResolution / 32;
                const uint32_t zThreads = 1;
                pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
            }

            // Unbind
            {
                ID3D11ShaderResourceView *ppSRViewBindTable[2] = { nullptr, nullptr };
                ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
                ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
                pDeviceContext->CSSetShader(nullptr, nullptr, 0);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);
            }
        }

        m_renderer.AccessShaders()
              .at("SmallScale/GenerateCombinedHeightmap")
              .ReleasePipeline(pDeviceContext);
        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Vertical Blur Display");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalDisplay")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_verticalBlurFinal.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_combinedHeightmap.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurVerticalDisplay")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    {
        pPerf->BeginEvent(L"Final Blur Display");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalDisplay")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_horizontalBlurFinal.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_verticalBlurFinal.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            ID3D11SamplerState *ppSamplerStates[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);

            m_renderer.AccessShaders()
                  .at("SmallScale/SandHeightBlurHorizontalDisplay")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }


    for (int i = 1; i < m_numGaussianHeightmapBlurPasses; i++) {
        {
            pPerf->BeginEvent(L"Vertical Blur Display");
            // Push State
            {
                m_renderer.AccessShaders()
                      .at("LargeScale/SandHeightBlurVerticalDisplay")
                      .SetPipeline(pDeviceContext);
                ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_verticalBlurFinal.GetUAV() };
                ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_horizontalBlurFinal.GetSRV() };
                ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
                ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
                pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
            }
            // Dispatch
            {
                assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
                const uint32_t xThreads = m_gridResolution / 32;
                const uint32_t yThreads = m_gridResolution / 32;
                const uint32_t zThreads = 1;
                pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
            }
            // Pop State
            {
                ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
                ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
                ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
                ID3D11SamplerState *ppSamplerStates[1] = { nullptr };
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
                pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
                m_renderer.AccessShaders()
                      .at("LargeScale/SandHeightBlurVerticalDisplay")
                      .ReleasePipeline(pDeviceContext);
            }
            pPerf->EndEvent();
        }
        {
            pPerf->BeginEvent(L"Final Blur Display");
            // Push State
            {
                m_renderer.AccessShaders()
                      .at("LargeScale/SandHeightBlurHorizontalDisplay")
                      .SetPipeline(pDeviceContext);
                ID3D11UnorderedAccessView *ppUAViewBindTable[1]
                      = { m_horizontalBlurFinal.GetUAV() };
                ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_verticalBlurFinal.GetSRV() };
                ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
                ID3D11SamplerState *ppSamplerStates[1] = { m_renderer.GetWrapSamplerState() };
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
                pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
            }
            // Dispatch
            {
                assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
                const uint32_t xThreads = m_gridResolution / 32;
                const uint32_t yThreads = m_gridResolution / 32;
                const uint32_t zThreads = 1;
                pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
            }
            // Pop State
            {
                ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
                ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
                ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
                ID3D11SamplerState *ppSamplerStates[1] = { nullptr };
                pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
                pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
                pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
                pDeviceContext->CSSetSamplers(0, 1, ppSamplerStates);
                m_renderer.AccessShaders()
                      .at("LargeScale/SandHeightBlurHorizontalDisplay")
                      .ReleasePipeline(pDeviceContext);
            }
            pPerf->EndEvent();
        }
    }

    // Finally, Generate the heightmap normals
    {
        pPerf->BeginEvent(L"Heightmap Normals");

        // Push State
        {
            m_renderer.AccessShaders()
                  .at("SmallScale/GenerateHeightmapNormals")
                  .SetPipeline(pDeviceContext);

            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { m_heightmapNormals.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { m_horizontalBlurFinal.GetSRV() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };

            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
        }

        // Dispatch
        {
            assert((m_gridResolution % 32) == 0 && "Exact grid only supported atm");
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Pop State
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[1] = { nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };


            pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 1, ppSRViewBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);

            m_renderer.AccessShaders()
                  .at("SmallScale/GenerateHeightmapNormals")
                  .ReleasePipeline(pDeviceContext);
        }

        pPerf->EndEvent();
    }

    // Store current values into previous frame values
    {
        pDeviceContext->CopyResource(
              m_bedrockBlocksRead.GetTexture(), m_bedrockBlocksWrite.GetTexture());
        pDeviceContext->CopyResource(m_sandBlocksRead.GetTexture(), m_sandBlocksWrite.GetTexture());
        pDeviceContext->CopyResource(
              m_previousCombinedHeightmap.GetTexture(), m_combinedHeightmap.GetTexture());
    }

    return true;
}

void SmallScaleDesertModel::Gui()
{
    ImGui::PushID(m_simulationId.c_str());

    ImGui::Checkbox("Step Small Scale Sim", &m_stepSimulation);

    if (ImGui::Button("Reset Simulation") && (!m_resetRequested)) {
        m_resetRequested = true;
    }
    {
        uint32_t min = 0;
        uint32_t max = 20;
        ImGui::SliderScalar("Max Transport Steps", ImGuiDataType_U32,
              &m_sandTransportParams.AccessData().maxTransportSteps, &min, &max, "%u");
    }
    {
        uint32_t min = 0;
        uint32_t max = 200;
        ImGui::SliderScalar("Target Blocks to Transport", ImGuiDataType_U32,
              &m_sandTransportParams.AccessData().targetBlocksToMove, &min, &max, "%u");
    }
    {
        uint32_t min = 1;
        uint32_t max = 10;
        ImGui::SliderScalar("Num Gaussian Steps", ImGuiDataType_U32,
              &m_numGaussianHeightmapBlurPasses, &min, &max);
    }
    {
        const uint32_t minVal = 0;
        const uint32_t maxVal = 200;
        ImGui::SliderScalar("Number of sand cascade passes", ImGuiDataType_U32,
              &m_numSandCascadePasses, &minVal, &maxVal);
    }
    ImGui::InputFloat("Wind Base Speed", &m_baseWindSpeed, 0.1f, 1.0f, "%.6f");
    ImGui::PopID();
}

void SmallScaleDesertModel::DoBedrockCascadePass(
      ID3D11DeviceContext *pDeviceContext, ID3DUserDefinedAnnotation *pPerf)
{
    {
        pPerf->BeginEvent(L"Bedrock Cascade");
        m_renderer.AccessShaders().at("SmallScale/BedrockCascade").SetPipeline(pDeviceContext);

        // Bind
        std::array UAViewBindTable
              = { m_bedrockBlocksWrite.GetUAV(), m_desertRandomStatesTexture.GetUAV() };
        std::array SRViewBindTable = { m_bedrockBlocksRead.GetSRV() };
        std::array SamplerStateBindTable = { m_renderer.GetWrapSamplerState() };
        std::array ConstantBuffers = { m_desertSimParams.GetBuffer() };

        pDeviceContext->CSSetShaderResources(0, SRViewBindTable.size(), SRViewBindTable.data());
        pDeviceContext->CSSetUnorderedAccessViews(
              0, UAViewBindTable.size(), UAViewBindTable.data(), nullptr);
        pDeviceContext->CSSetSamplers(
              0, SamplerStateBindTable.size(), SamplerStateBindTable.data());
        pDeviceContext->CSSetConstantBuffers(0, ConstantBuffers.size(), ConstantBuffers.data());


        // Dispatch
        {
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
        }

        // Unbind
        for (auto &entry : UAViewBindTable) {
            entry = nullptr;
        }
        for (auto &entry : SRViewBindTable) {
            entry = nullptr;
        }
        for (auto &entry : SamplerStateBindTable) {
            entry = nullptr;
        }
        for (auto &entry : ConstantBuffers) {
            entry = nullptr;
        }
        pDeviceContext->CSSetShader(nullptr, nullptr, 0);
        pDeviceContext->CSSetShaderResources(0, SRViewBindTable.size(), SRViewBindTable.data());
        pDeviceContext->CSSetUnorderedAccessViews(
              0, UAViewBindTable.size(), UAViewBindTable.data(), nullptr);
        pDeviceContext->CSSetSamplers(
              0, SamplerStateBindTable.size(), SamplerStateBindTable.data());
        pDeviceContext->CSSetConstantBuffers(0, ConstantBuffers.size(), ConstantBuffers.data());

        m_renderer.AccessShaders().at("SmallScale/BedrockCascade").ReleasePipeline(pDeviceContext);
        pPerf->EndEvent();
    }

    pDeviceContext->CopyResource(
          m_bedrockBlocksRead.GetTexture(), m_bedrockBlocksWrite.GetTexture());
}


void SmallScaleDesertModel::DoSandCascadePass(
      ID3D11DeviceContext *pDeviceContext, ID3DUserDefinedAnnotation *pPerf)
{
    {
        pPerf->BeginEvent(L"Sand Cascade");
        m_renderer.AccessShaders().at("SmallScale/SandCascade").SetPipeline(pDeviceContext);

        // Bind
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[2]
                  = { m_desertRandomStatesTexture.GetUAV(), m_sandBlocksWrite.GetUAV() };
            ID3D11ShaderResourceView *ppSRViewBindTable[2]
                  = { m_sandBlocksRead.GetSRV(), m_bedrockBlocksRead.GetSRV() };
            ID3D11SamplerState *ppSamplerStateBindTable[1] = { m_renderer.GetWrapSamplerState() };
            ID3D11Buffer *ppConstantBuffers[1] = { m_desertSimParams.GetBuffer() };
            pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);
            pDeviceContext->CSSetUnorderedAccessViews(0, 2, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStateBindTable);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
        }

        // Dispatch
        {
            const uint32_t xThreads = m_gridResolution / 32;
            const uint32_t yThreads = m_gridResolution / 32;
            const uint32_t zThreads = 1;
            pDeviceContext->Flush();
            pDeviceContext->Dispatch(xThreads, yThreads, zThreads);
            pDeviceContext->Flush();
        }

        // Unbind
        {
            ID3D11UnorderedAccessView *ppUAViewBindTable[2] = { nullptr, nullptr };
            ID3D11ShaderResourceView *ppSRViewBindTable[2] = { nullptr, nullptr };
            ID3D11SamplerState *ppSamplerStateBindTable[1] = { nullptr };
            ID3D11Buffer *ppConstantBuffers[1] = { nullptr };
            pDeviceContext->CSSetShader(nullptr, nullptr, 0);
            pDeviceContext->CSSetConstantBuffers(0, 1, ppConstantBuffers);
            pDeviceContext->CSSetSamplers(0, 1, ppSamplerStateBindTable);
            pDeviceContext->CSSetUnorderedAccessViews(0, 2, ppUAViewBindTable, nullptr);
            pDeviceContext->CSSetShaderResources(0, 2, ppSRViewBindTable);
        }
        m_renderer.AccessShaders().at("SmallScale/SandCascade").ReleasePipeline(pDeviceContext);
        pPerf->EndEvent();
    }

    pDeviceContext->CopyResource(m_sandBlocksRead.GetTexture(), m_sandBlocksWrite.GetTexture());
}
}