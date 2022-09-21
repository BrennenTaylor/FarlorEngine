#pragma once

#include <d3d11.h>
#include <d3d11_1.h>

#include <d3dcommon.h>
#include <dxgicommon.h>
#include <dxgiformat.h>
#include <unordered_map>
#include <wrl.h>

#include <array>
#include <memory>
#include <filesystem>
#include <vector>
#include <string>
#include <wrl/client.h>
#include <winnt.h>

namespace Farlor {

enum IncludedShaders {
    Flags_None = 0,
    Flags_Compute = 1 << 0,
    Flags_Vertex = 1 << 1,
    Flags_Pixel = 1 << 2,
    Flags_EnableTess = 1 << 3,
    Flags_Geometry = 1 << 4
};

DEFINE_ENUM_FLAG_OPERATORS(IncludedShaders)

class Shader {
   public:
    class ShaderIncludeHandler : public ID3DInclude {
       public:
        ShaderIncludeHandler();
        virtual ~ShaderIncludeHandler() = default;

        // Directories are added in search order
        void RegisterIncludeDirectoryRelative(const std::string &includeDirectory);

        HRESULT Open(D3D_INCLUDE_TYPE includeType, LPCSTR pFilename, LPCVOID pParentData,
              LPCVOID *ppData, UINT *pBytes) noexcept final;

        HRESULT Close(LPCVOID pData) noexcept final;

       private:
        std::vector<std::filesystem::path> m_shaderIncludeDirectories;
        std::vector<std::unique_ptr<char[]>> m_data;
    };

   public:
    Shader() = default;
    Shader(const std::string &filename, IncludedShaders includedShaders,
          const std::vector<std::string> &defines);
    void Initialize(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);


    void SetPipeline(ID3D11DeviceContext *pDeviceContext) const;
    void ReleasePipeline(ID3D11DeviceContext *pDeviceContext) const;

   private:
    void InitializeCompute(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
          ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros);
    void InitializeVertex(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
          ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros);
    void InitializePixel(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
          ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros);
    void InitializeTesselation(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
          ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros);

   private:
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_pComputeShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayout = nullptr;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11DomainShader> m_pDomainShader = nullptr;
    Microsoft::WRL::ComPtr<ID3D11HullShader> m_pHullShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_pGeometryShader = nullptr;

    // TODO: Do we need to store this crap?
    std::vector<std::string> m_defines;
    std::string m_filename = "";
    IncludedShaders m_includedShaderFlags = IncludedShaders::Flags_None;
};

class ShaderBlobManager {
   public:
    enum class ShaderType { Compute, Vertex, Pixel, Hull, Domain, Geometry };

    using ShaderHandle = uint32_t;
    static ShaderHandle NullShader;

   public:
    ShaderBlobManager() = default;

    void RegisterIncludeDirectoryRelative(const std::string &includeDirectory)
    {
        m_includeHandler.RegisterIncludeDirectoryRelative(includeDirectory);
    }


    ShaderBlobManager::ShaderHandle CompileShader(const ShaderBlobManager::ShaderType shaderType,
          const std::string &relativeFilename, const std::string &entryFunctionName);

    ID3D11ComputeShader *GetComputeShader(ShaderHandle handle);
    ID3D11VertexShader *GetVertexShader(ShaderHandle handle);
    ID3D11PixelShader *GetPixelShader(ShaderHandle handle);
    ID3D11DomainShader *GetDomainShader(ShaderHandle handle);
    ID3D11HullShader *GetHullShader(ShaderHandle handle);
    ID3D11GeometryShader *GetGeometryShader(ShaderHandle handle);

   private:
    Shader::ShaderIncludeHandler m_includeHandler;

    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11ComputeShader>>
          m_registeredComputeShaders;
    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11VertexShader>>
          m_registeredVertexShaders;
    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11PixelShader>>
          m_registeredPixelShaders;
    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11DomainShader>>
          m_registeredDomainShaders;
    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11HullShader>>
          m_registeredHullShaders;
    std::unordered_map<ShaderHandle, Microsoft::WRL::ComPtr<ID3D11GeometryShader>>
          m_registeredGeometryShaders;
};


class PipelineState {
   public:
    PipelineState(ShaderBlobManager &shaderBlobManager);
    void BindPipeline(ID3D11DeviceContext *pDeviceContext);
    void UnbindPipeline(ID3D11DeviceContext *pDeviceContext);

   private:
    ShaderBlobManager &m_shaderBlobManager;

    // Compute Pipeline Path
    ShaderBlobManager::ShaderHandle m_computeShader = ShaderBlobManager::NullShader;

    // Graphics Pipeline Path
    ShaderBlobManager::ShaderHandle m_vertexShader = ShaderBlobManager::NullShader;
    ShaderBlobManager::ShaderHandle m_pixelShader = ShaderBlobManager::NullShader;
    ShaderBlobManager::ShaderHandle m_domainShader = ShaderBlobManager::NullShader;
    ShaderBlobManager::ShaderHandle m_hullShader = ShaderBlobManager::NullShader;
    ShaderBlobManager::ShaderHandle m_geometryShader = ShaderBlobManager::NullShader;

    D3D11_PRIMITIVE_TOPOLOGY m_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    D3D11_BLEND_DESC m_blendDesc = { 0 };
    D3D11_RASTERIZER_DESC m_rasterizerDesc;  // Needs to be zeroed
    D3D11_DEPTH_STENCIL_DESC m_dsDesc = { 0 };

    std::array<DXGI_FORMAT, 8> m_renderTargetFormats = { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };
    DXGI_FORMAT m_dsvFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_SAMPLE_DESC m_sampleDesc = { 1, 0 };  // For now, only handle single sampled rendering
};

class PipelineStateManager {
   public:
   private:
};
}
