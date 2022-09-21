#pragma once

#include <d3d11.h>
#include <d3d11_1.h>

#include <wrl.h>

#include <vector>
#include <string>

namespace Farlor {
class GraphicsPipelineState {
   public:
    struct ShaderBytecode {
        void *pData = nullptr;
        size_t pNumBytes = 0;
    };

   public:
    GraphicsPipelineState();
    GraphicsPipelineState(const std::string &filename, bool hasVS, bool hasPS, bool hasDS,
          bool hasHS, bool hasGS, bool hasCS, const std::vector<std::string> &defines);
    void Initialize(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);

    void SetPipeline(ID3D11DeviceContext *pDeviceContext);
    void ReleasePipeline(ID3D11DeviceContext *pDeviceContext);

   private:
    void PrintShaderErrors(const Microsoft::WRL::ComPtr<ID3DBlob> &errors);
    void PrintShaderWarnings(const Microsoft::WRL::ComPtr<ID3DBlob> &warnings);

   private:
    std::string m_filename = "";

    ID3D11InputLayout *m_pInputLayout = nullptr;

    ShaderBytecode m_vertexShader;
    ShaderBytecode m_pixelShader;

    ShaderBytecode m_domainShaderBytecode;
    ShaderBytecode m_hullShaderBytecode;
    ShaderBytecode m_geometryShaderBytecode;
};
}
