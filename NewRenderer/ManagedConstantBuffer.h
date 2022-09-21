#pragma once

#include <wrl/client.h>
#include <d3d11.h>

#include "DebugNameUtil.h"

#include "../Util/Logger.h"

namespace Farlor {

template<typename T>
class ManagedConstantBuffer {
   public:
    ManagedConstantBuffer<T>()
        : m_buffer(nullptr)
        , m_data()
        , m_name(typeid(T).name())
    {
    }

    ManagedConstantBuffer<T>(const std::string &name)
        : m_buffer(nullptr)
        , m_data()
        , m_name(name)
    {
    }

    ManagedConstantBuffer<T>(const char *pName)
        : m_buffer(nullptr)
        , m_data()
        , m_name(pName)
    {
    }

    ID3D11Buffer *GetBuffer() { return m_buffer.Get(); }
    T &AccessData() { return m_data; }

    void Initialize(ID3D11Device *pDevice)
    {
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(T);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = 0;
        cbd.MiscFlags = 0;

        pDevice->CreateBuffer(&cbd, 0, m_buffer.GetAddressOf());

        assert(m_buffer && ("Failed to create" + m_name + " buffer").c_str());
        SetDebugObjectName(m_buffer.Get(), m_name + " Constant Buffer");
    }

    void Update(ID3D11DeviceContext *pDeviceContext)
    {
        if (!m_buffer) {
            FARLOR_LOG_ERROR("Failed to update constant buffer");
        }
        pDeviceContext->UpdateSubresource(m_buffer.Get(), 0, 0, &m_data, 0, 0);
    }

   private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer = nullptr;
    T m_data;
    std::string m_name = "";
};
}