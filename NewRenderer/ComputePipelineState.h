#pragma once

#include <d3d11.h>
#include <d3d11_1.h>

#include <vector>
#include <string>

namespace Farlor {
class ComputePipelineState {
   public:
    struct ShaderBytecode {
        void *pData = nullptr;
        size_t pNumBytes = 0;
    };

   public:
    ComputePipelineState();
    ComputePipelineState(const std::string &filename, bool hasVS, bool hasPS, bool hasDS,
          bool hasHS, bool hasGS, bool hasCS, const std::vector<std::string> &defines);
    void Initialize(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext);

    void SetPipeline(ID3D11DeviceContext *pDeviceContext);
    void ReleasePipeline(ID3D11DeviceContext *pDeviceContext);

    uint32_t Hash()
    {
        if (!m_hashCached) {
            ComputeHash();
        }
        return m_hash;
    }

   private:
    void ComputeHash();

   private:
    ShaderBytecode m_ComputeShader;

    bool m_hashCached = false;
    uint32_t m_hash = 0;
};
}
