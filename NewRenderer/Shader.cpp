#include "Shader.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include "../Util/StringUtil.h"
#include "../Util/Logger.h"

#include "DebugNameUtil.h"

#include <filesystem>
#include <iostream>
#include <memory>

namespace Farlor {

void PrintShaderErrors(const Microsoft::WRL::ComPtr<ID3DBlob> &errors)
{
    FARLOR_LOG_ERROR("Shader Compile Errors")
    std::string errorString((char *)errors->GetBufferPointer());

    size_t start = 0;
    size_t end = errorString.find("\n");
    while (end != -1) {
        FARLOR_LOG_ERROR("{}", errorString.substr(start, end - start).c_str());
        start = end + 1;
        end = errorString.find("\n", start);
    }
    FARLOR_LOG_ERROR("{}", errorString.substr(start, end - start).c_str());
}
void PrintShaderWarnings(const Microsoft::WRL::ComPtr<ID3DBlob> &warnings)
{
    FARLOR_LOG_WARNING("Shader Compile Warnings: ");

    std::string errorString((char *)warnings->GetBufferPointer());

    size_t start = 0;
    size_t end = errorString.find("\n");
    while (end != -1) {
        FARLOR_LOG_WARNING("{}", errorString.substr(start, end - start).c_str());
        start = end + 1;
        end = errorString.find("\n", start);
    }
    FARLOR_LOG_WARNING("{}", errorString.substr(start, end - start).c_str());
}

const std::wstring ShaderResourceDirectoryWStr = L"resources/shaders/";


Shader::ShaderIncludeHandler::ShaderIncludeHandler()
    : m_shaderIncludeDirectories()
{
}

void Shader::ShaderIncludeHandler::RegisterIncludeDirectoryRelative(
      const std::string &includeDirectory)
{
    m_shaderIncludeDirectories.push_back(includeDirectory);
}

HRESULT Shader::ShaderIncludeHandler::Open(D3D_INCLUDE_TYPE includeType, LPCSTR pFilename,
      LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) noexcept
{
    for (const auto &includeDirectory : m_shaderIncludeDirectories) {
        std::filesystem::path attemptedFilename = includeDirectory / std::string(pFilename);
        if (std::filesystem::exists(attemptedFilename)) {
            std::ifstream includeFile(attemptedFilename.c_str(), std::ios::binary);
            if (!includeFile.is_open()) {
                std::cout << "Failed to include: " << pFilename << std::endl;
                *ppData = nullptr;
                *pBytes = 0;
                return S_OK;
            }

            includeFile.seekg(0, std::ios::end);
            UINT size = static_cast<UINT>(includeFile.tellg());
            includeFile.seekg(0, std::ios::beg);

            std::unique_ptr<char[]> readData(new char[size]);
            includeFile.read(readData.get(), size);
            *ppData = readData.get();
            *pBytes = size;
            m_data.push_back(std::move(readData));
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT Shader::ShaderIncludeHandler::Close(LPCVOID pData) noexcept
{
    std::vector<std::unique_ptr<char[]>>::iterator findResult = std::find_if(
          m_data.begin(), m_data.end(), [&](std::unique_ptr<char[]> const &upStoredValue) {
              return upStoredValue.get() == pData;
          });
    if (findResult != m_data.end()) {
        m_data.erase(findResult);
    }
    return S_OK;
}

Shader::Shader(const std::string &filename, const IncludedShaders includedShaders,
      const std::vector<std::string> &defines)
    : m_defines { defines }
    , m_filename { filename }
    , m_includedShaderFlags(includedShaders)
{
}

void Shader::Initialize(ID3D11Device *const pDevice, ID3D11DeviceContext *const pDeviceContext)
{
    FARLOR_LOG_INFO("Loading shader: {}", m_filename)

    // Create shader defines
    std::vector<D3D_SHADER_MACRO> macros;
    for (const auto &define : m_defines) {
        macros.push_back({ define.c_str(), "1" });
    }

    // NOTE: Must cap off with null defines!!!!
    macros.push_back({ nullptr, nullptr });

    ShaderIncludeHandler includeHandler;
    includeHandler.RegisterIncludeDirectoryRelative(
          WideStringToString(ShaderResourceDirectoryWStr));

    if (m_includedShaderFlags & IncludedShaders::Flags_Compute) {
        InitializeCompute(pDevice, pDeviceContext, includeHandler, macros);
        return;  // We can return as we should never enable compute and graphics pipeline
    }

    if (m_includedShaderFlags & IncludedShaders::Flags_Vertex) {
        InitializeVertex(pDevice, pDeviceContext, includeHandler, macros);
    }

    if (m_includedShaderFlags & IncludedShaders::Flags_Pixel) {
        InitializePixel(pDevice, pDeviceContext, includeHandler, macros);
    }

    if (m_includedShaderFlags & IncludedShaders::Flags_EnableTess) {
        InitializeTesselation(pDevice, pDeviceContext, includeHandler, macros);
    }
}

void Shader::InitializeCompute(ID3D11Device *const pDevice,
      ID3D11DeviceContext *const pDeviceContext, ShaderIncludeHandler &includeHandler,
      std::vector<D3D_SHADER_MACRO> macros)
{
    macros.insert(macros.begin(), { "COMPUTE_SHADER", "1" });
    Microsoft::WRL::ComPtr<ID3DBlob> errorMessage = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> computeShaderBuffer = nullptr;
    std::wstring computeShaderName = ShaderResourceDirectoryWStr + StringToWideString(m_filename);

    HRESULT result = D3DCompileFromFile(computeShaderName.c_str(), macros.data(), &includeHandler,
          "CSMain", "cs_5_0", 0, 0, &computeShaderBuffer, &errorMessage);
    if (errorMessage) {
        if (FAILED(result)) {
            PrintShaderErrors(errorMessage);
        } else {
            PrintShaderWarnings(errorMessage);
        }
    }

    pDevice->CreateComputeShader(computeShaderBuffer->GetBufferPointer(),
          computeShaderBuffer->GetBufferSize(), 0, &m_pComputeShader);
    assert(m_pComputeShader && "Failed to create compute shader");

    const std::string shaderName = m_filename + std::string(": CS");
    SetDebugObjectName(m_pComputeShader.Get(), shaderName);
}

void Shader::InitializeVertex(ID3D11Device *const pDevice,
      ID3D11DeviceContext *const pDeviceContext, ShaderIncludeHandler &includeHandler,
      std::vector<D3D_SHADER_MACRO> macros)
{
    macros.insert(macros.begin(), { "VERTEX_SHADER", "1" });
    Microsoft::WRL::ComPtr<ID3DBlob> errorMessage = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;

    // TODO: Remove hardcoding
    std::wstring vertexShaderName = ShaderResourceDirectoryWStr + StringToWideString(m_filename);
    HRESULT result = D3DCompileFromFile(vertexShaderName.c_str(), macros.data(), &includeHandler,
          "VSMain", "vs_5_0", 0, 0, &vertexShaderBuffer, &errorMessage);
    if (errorMessage) {
        if (FAILED(result)) {
            PrintShaderErrors(errorMessage);
        } else {
            PrintShaderWarnings(errorMessage);
        }
    }

    pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
          vertexShaderBuffer->GetBufferSize(), 0, &m_pVertexShader);
    assert(m_pVertexShader && "Failed to create vertex shader");
    // Shader name
    const std::string shaderName = m_filename + std::string(": VS");
    SetDebugObjectName(m_pVertexShader.Get(), shaderName);

    // Reflection shader info
    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> pVertexShaderReflection = nullptr;
    result = D3DReflect(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
          IID_ID3D11ShaderReflection, (void **)&pVertexShaderReflection);
    assert(pVertexShaderReflection && "Failed to get shader reflection");

    // TODO: Leads to duplicate input layouts
    D3D11_SHADER_DESC shaderDesc;
    pVertexShaderReflection->GetDesc(&shaderDesc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
    for (uint32_t i = 0; i < shaderDesc.InputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC elementDesc;
        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = 0;
        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        // determine DXGI format
        if (paramDesc.Mask == 1) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        } else if (paramDesc.Mask <= 3) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (paramDesc.Mask <= 7) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        } else if (paramDesc.Mask <= 15) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        //save element desc
        inputLayoutDesc.push_back(elementDesc);
    }

    result = pDevice->CreateInputLayout(inputLayoutDesc.data(),
          (unsigned int)inputLayoutDesc.size(), vertexShaderBuffer->GetBufferPointer(),
          vertexShaderBuffer->GetBufferSize(), &m_pInputLayout);
    assert(m_pInputLayout && "Failed to create input layout from shader");
    SetDebugObjectName(m_pInputLayout.Get(), shaderName + " input layout");
}

void Shader::InitializePixel(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
      ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros)
{
    macros.insert(macros.begin(), { "PIXEL_SHADER", "1" });
    Microsoft::WRL::ComPtr<ID3DBlob> errorMessage = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
    std::wstring pixelShaderName = ShaderResourceDirectoryWStr + StringToWideString(m_filename);
    HRESULT result = D3DCompileFromFile(pixelShaderName.c_str(), macros.data(), &includeHandler,
          "PSMain", "ps_5_0", 0, 0, &pixelShaderBuffer, &errorMessage);
    if (errorMessage) {
        if (FAILED(result)) {
            PrintShaderErrors(errorMessage);
        } else {
            PrintShaderWarnings(errorMessage);
        }
    }

    result = pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
          pixelShaderBuffer->GetBufferSize(), 0, &m_pPixelShader);
    assert(m_pPixelShader && "Failed to create pixel shader");

    const std::string shaderName = m_filename + std::string(": PS");
    SetDebugObjectName(m_pPixelShader.Get(), shaderName);
}
void Shader::InitializeTesselation(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext,
      ShaderIncludeHandler &includeHandler, std::vector<D3D_SHADER_MACRO> macros)
{
    {
        macros.insert(macros.begin(), { "HULL_SHADER", "1" });
        Microsoft::WRL::ComPtr<ID3DBlob> errorMessage = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> hullShaderBuffer = nullptr;
        std::wstring hullShaderName = ShaderResourceDirectoryWStr + StringToWideString(m_filename);
        HRESULT result = D3DCompileFromFile(hullShaderName.c_str(), macros.data(), &includeHandler,
              "HSMain", "hs_5_0", 0, 0, &hullShaderBuffer, &errorMessage);
        if (errorMessage) {
            if (FAILED(result)) {
                PrintShaderErrors(errorMessage);
            } else {
                PrintShaderWarnings(errorMessage);
            }
        }

        result = pDevice->CreateHullShader(hullShaderBuffer->GetBufferPointer(),
              hullShaderBuffer->GetBufferSize(), 0, &m_pHullShader);
        assert(m_pHullShader && "Failed to create hull shader");

        const std::string shaderName = m_filename + std::string(": HS");
        SetDebugObjectName(m_pHullShader.Get(), shaderName);
    }

    macros.erase(macros.begin());
    {
        macros.insert(macros.begin(), { "DOMAIN_SHADER", "1" });
        Microsoft::WRL::ComPtr<ID3DBlob> errorMessage = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> domainShaderBuffer = nullptr;
        std::wstring domainShaderName
              = ShaderResourceDirectoryWStr + StringToWideString(m_filename);
        HRESULT result = D3DCompileFromFile(domainShaderName.c_str(), macros.data(),
              &includeHandler, "DSMain", "ds_5_0", 0, 0, &domainShaderBuffer, &errorMessage);
        if (errorMessage) {
            if (FAILED(result)) {
                PrintShaderErrors(errorMessage);
            } else {
                PrintShaderWarnings(errorMessage);
            }
        }

        result = pDevice->CreateDomainShader(domainShaderBuffer->GetBufferPointer(),
              domainShaderBuffer->GetBufferSize(), 0, &m_pDomainShader);
        assert(m_pDomainShader && "Failed to create domain shader");

        const std::string shaderName = m_filename + std::string(": DS");
        SetDebugObjectName(m_pDomainShader.Get(), shaderName);
    }
}

void Shader::SetPipeline(ID3D11DeviceContext *pDeviceContext) const
{
    if (m_includedShaderFlags & IncludedShaders::Flags_Vertex) {
        pDeviceContext->VSSetShader(m_pVertexShader.Get(), 0, 0);
        pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
    }
    // Should be if tesselate?
    if (m_includedShaderFlags & IncludedShaders::Flags_EnableTess) {
        pDeviceContext->HSSetShader(m_pHullShader.Get(), nullptr, 0);
        pDeviceContext->DSSetShader(m_pDomainShader.Get(), nullptr, 0);
    }
    if (m_includedShaderFlags & IncludedShaders::Flags_Pixel) {
        pDeviceContext->PSSetShader(m_pPixelShader.Get(), 0, 0);
    }
    if (m_includedShaderFlags & IncludedShaders::Flags_Compute) {
        pDeviceContext->CSSetShader(m_pComputeShader.Get(), 0, 0);
    }
}

void Shader::ReleasePipeline(ID3D11DeviceContext *pDeviceContext) const
{
    if (m_includedShaderFlags & IncludedShaders::Flags_Vertex) {
        pDeviceContext->VSSetShader(nullptr, 0, 0);
        pDeviceContext->IASetInputLayout(nullptr);
    }
    // Should be if tesselate?
    if (m_includedShaderFlags & IncludedShaders::Flags_EnableTess) {
        pDeviceContext->HSSetShader(nullptr, nullptr, 0);
        pDeviceContext->DSSetShader(nullptr, nullptr, 0);
    }
    if (m_includedShaderFlags & IncludedShaders::Flags_Pixel) {
        pDeviceContext->PSSetShader(nullptr, 0, 0);
    }
    if (m_includedShaderFlags & IncludedShaders::Flags_Compute) {
        pDeviceContext->CSSetShader(nullptr, 0, 0);
    }
}

ShaderBlobManager::ShaderHandle ShaderBlobManager::NullShader = 0;

ShaderBlobManager::ShaderHandle ShaderBlobManager::CompileShader(
      const ShaderBlobManager::ShaderType shaderType, const std::string &relativeFilename,
      const std::string &entryFunctionName)
{
    return ShaderBlobManager::NullShader;
}

ID3D11ComputeShader *ShaderBlobManager::GetComputeShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredComputeShaders.find(handle);
    if (foundEntry != m_registeredComputeShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}
ID3D11VertexShader *ShaderBlobManager::GetVertexShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredVertexShaders.find(handle);
    if (foundEntry != m_registeredVertexShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}
ID3D11PixelShader *ShaderBlobManager::GetPixelShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredPixelShaders.find(handle);
    if (foundEntry != m_registeredPixelShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}
ID3D11DomainShader *ShaderBlobManager::GetDomainShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredDomainShaders.find(handle);
    if (foundEntry != m_registeredDomainShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}
ID3D11HullShader *ShaderBlobManager::GetHullShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredHullShaders.find(handle);
    if (foundEntry != m_registeredHullShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}
ID3D11GeometryShader *ShaderBlobManager::GetGeometryShader(ShaderHandle handle)
{
    auto foundEntry = m_registeredGeometryShaders.find(handle);
    if (foundEntry != m_registeredGeometryShaders.end()) {
        return foundEntry->second.Get();
    }
    return nullptr;
}

PipelineState::PipelineState(ShaderBlobManager &shaderBlobManager)
    : m_shaderBlobManager(shaderBlobManager)
{
}

void PipelineState::BindPipeline(ID3D11DeviceContext *pDeviceContext)
{
    if (m_computeShader != ShaderBlobManager::NullShader) {
        ID3D11ComputeShader *pComputeShader = m_shaderBlobManager.GetComputeShader(m_computeShader);
        pDeviceContext->CSSetShader(pComputeShader, 0, 0);
        return;
    }

    // Graphics Pipeline Version
    // if () {
    //     pDeviceContext->VSSetShader(m_pVertexShader.Get(), 0, 0);
    //     pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
    // }
    // if (m_hasPS) {
    //     pDeviceContext->PSSetShader(m_pPixelShader.Get(), 0, 0);
    // }
}

void PipelineState::UnbindPipeline(ID3D11DeviceContext *pDeviceContext)
{
    // Compute Path
    if (m_computeShader != ShaderBlobManager::NullShader) {
        pDeviceContext->CSSetShader(nullptr, 0, 0);
        return;
    }

    // Graphics Pipeline path

    if (m_vertexShader != ShaderBlobManager::NullShader) {
        pDeviceContext->VSSetShader(nullptr, 0, 0);
        pDeviceContext->IASetInputLayout(nullptr);
    }
    if (m_pixelShader != ShaderBlobManager::NullShader) {
        pDeviceContext->PSSetShader(nullptr, 0, 0);
    }
    if (m_domainShader != ShaderBlobManager::NullShader) {
        pDeviceContext->DSSetShader(nullptr, 0, 0);
    }
    if (m_hullShader != ShaderBlobManager::NullShader) {
        pDeviceContext->HSSetShader(nullptr, 0, 0);
    }
    if (m_geometryShader != ShaderBlobManager::NullShader) {
        pDeviceContext->GSSetShader(nullptr, 0, 0);
    }
}

}
