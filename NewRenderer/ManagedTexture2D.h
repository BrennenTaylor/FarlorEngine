#pragma once

#include <d3dcommon.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <d3d11.h>

#include "DebugNameUtil.h"

#include "../Util/Logger.h"

namespace Farlor {

template<typename T>
class ManagedTexture2D {
   public:
    ManagedTexture2D<T>(const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(typeid(T).name())
    {
    }

    ManagedTexture2D<T>(const std::string &name, const uint32_t width, const uint32_t height,
          const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(name)
    {
    }

    ManagedTexture2D<T>(
          const char *pName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(pName)
    {
    }

    ID3D11Texture2D *GetTexture() { return m_texture.Get(); }
    ID3D11ShaderResourceView *GetSRV() { return m_srv.Get(); }
    ID3D11UnorderedAccessView *GetUAV() { return m_uav.Get(); }

    void Initialize(ID3D11Device *pDevice)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
            textureUAVDesc.Format = m_format;
            textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            textureUAVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateUnorderedAccessView(
                  m_texture.Get(), &textureUAVDesc, m_uav.GetAddressOf());
            assert(m_uav && (m_name + " uav").c_str());
            SetDebugObjectName(m_uav.Get(), m_name + " UAV");
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " SRV");
        }
    }

    void Initialize(ID3D11Device *pDevice, const T *const pData)
    {
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA initData = { 0 };
            initData.pSysMem = (const void *)pData;
            initData.SysMemPitch = m_width * sizeof(T);
            initData.SysMemSlicePitch = 0;

            pDevice->CreateTexture2D(&textureDesc, &initData, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
            textureUAVDesc.Format = m_format;
            textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            textureUAVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateUnorderedAccessView(
                  m_texture.Get(), &textureUAVDesc, m_uav.GetAddressOf());
            assert(m_uav && (m_name + " uav").c_str());
            SetDebugObjectName(m_uav.Get(), m_name + " UAV");
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " SRV");
        }
    }

   private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv = nullptr;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uav = nullptr;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::string m_name = "";
};

template<typename T>
class ManagedRenderTargetTexture2D {
   public:
    ManagedRenderTargetTexture2D<T>(
          const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(typeid(T).name())
    {
    }

    ManagedRenderTargetTexture2D<T>(const std::string &name, const uint32_t width,
          const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(name)
    {
    }

    ManagedRenderTargetTexture2D<T>(
          const char *pName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(pName)
    {
    }

    ID3D11Texture2D *GetTexture() { return m_texture.Get(); }
    ID3D11ShaderResourceView *GetSRV() { return m_srv.Get(); }
    ID3D11UnorderedAccessView *GetUAV() { return m_uav.Get(); }
    ID3D11RenderTargetView *GetRTV() { return m_rtv.Get(); }

    void Initialize(ID3D11Device *pDevice)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE
                  | D3D11_BIND_RENDER_TARGET;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
            textureUAVDesc.Format = m_format;
            textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            textureUAVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateUnorderedAccessView(
                  m_texture.Get(), &textureUAVDesc, m_uav.GetAddressOf());
            assert(m_uav && (m_name + " uav").c_str());
            SetDebugObjectName(m_uav.Get(), m_name + " UAV");
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " SRV");
        }

        {
            D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
            textureRTVDesc.Format = m_format;
            textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            textureRTVDesc.Texture2D.MipSlice = 0;

            pDevice->CreateRenderTargetView(m_texture.Get(), &textureRTVDesc, m_rtv.GetAddressOf());
            assert(m_rtv && (m_name + " rtv").c_str());
            SetDebugObjectName(m_rtv.Get(), m_name + " RTV");
        }
    }

   private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv = nullptr;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uav = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv = nullptr;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::string m_name = "";
};


template<typename T>
class ManagedRenderTargetTexture2DMS {
   public:
    ManagedRenderTargetTexture2DMS<T>(
          const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_rtv(nullptr)
        , m_resolvedTexture(nullptr)
        , m_resolvedSRV(nullptr)
        , m_resolvedRTV(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(typeid(T).name())
    {
    }

    ManagedRenderTargetTexture2DMS<T>(const std::string &name, const uint32_t width,
          const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_rtv(nullptr)
        , m_resolvedTexture(nullptr)
        , m_resolvedSRV(nullptr)
        , m_resolvedRTV(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(name)
    {
    }

    ManagedRenderTargetTexture2DMS<T>(
          const char *pName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_rtv(nullptr)
        , m_resolvedTexture(nullptr)
        , m_resolvedSRV(nullptr)
        , m_resolvedRTV(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(pName)
    {
    }

    ID3D11Texture2D *GetTexture() { return m_texture.Get(); }
    ID3D11ShaderResourceView *GetSRV() { return m_srv.Get(); }
    ID3D11RenderTargetView *GetRTV() { return m_rtv.Get(); }

    ID3D11Texture2D *GetResolvedTexture() { return m_resolvedTexture.Get(); }
    ID3D11ShaderResourceView *GetResolvedSRV() { return m_resolvedSRV.Get(); }
    ID3D11RenderTargetView *GetResolvedRTV() { return m_resolvedRTV.Get(); }

    void Initialize(ID3D11Device *const pDevice)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 4;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " MS texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " ms srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " ms SRV");
        }

        {
            D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
            textureRTVDesc.Format = m_format;
            textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            textureRTVDesc.Texture2D.MipSlice = 0;

            pDevice->CreateRenderTargetView(m_texture.Get(), &textureRTVDesc, m_rtv.GetAddressOf());
            assert(m_rtv && (m_name + " ms rtv").c_str());
            SetDebugObjectName(m_rtv.Get(), m_name + " ms RTV");
        }

        // Resolved  Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_resolvedTexture.GetAddressOf());
            assert(m_resolvedTexture && (m_name + " MS resolved texture").c_str());
            SetDebugObjectName(m_resolvedTexture.Get(), m_name);
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_resolvedTexture.Get(), &textureSRVDesc, m_resolvedSRV.GetAddressOf());
            assert(m_resolvedSRV && (m_name + " ms resolved srv").c_str());
            SetDebugObjectName(m_resolvedSRV.Get(), m_name + " ms resolved SRV");
        }

        {
            D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
            textureRTVDesc.Format = m_format;
            textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            textureRTVDesc.Texture2D.MipSlice = 0;

            pDevice->CreateRenderTargetView(
                  m_resolvedTexture.Get(), &textureRTVDesc, m_resolvedRTV.GetAddressOf());
            assert(m_resolvedRTV && (m_name + " ms resolved rtv").c_str());
            SetDebugObjectName(m_resolvedRTV.Get(), m_name + " ms resolved RTV");
        }
    }

    void Resolve(ID3D11DeviceContext *const pDeviceContext)
    {
        pDeviceContext->ResolveSubresource(
              m_resolvedTexture.Get(), 0, m_texture.Get(), 0, m_format);
    }

   private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_resolvedTexture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_resolvedSRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_resolvedRTV = nullptr;

    uint32_t m_width = 1;
    uint32_t m_height = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::string m_name = "";
};

template<typename T>
class ManagedTexture2DMipEnabled {
   public:
    ManagedTexture2DMipEnabled<T>(
          const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(typeid(T).name())
    {
    }

    ManagedTexture2DMipEnabled<T>(const std::string &name, const uint32_t width,
          const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(name)
    {
    }

    ManagedTexture2DMipEnabled<T>(
          const char *pName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_srv(nullptr)
        , m_uav(nullptr)
        , m_rtv(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(pName)
    {
    }

    ID3D11Texture2D *GetTexture() { return m_texture.Get(); }
    ID3D11ShaderResourceView *GetSRV() { return m_srv.Get(); }
    ID3D11UnorderedAccessView *GetUAV() { return m_uav.Get(); }
    ID3D11RenderTargetView *GetRTV() { return m_rtv.Get(); }


    void Initialize(ID3D11Device *pDevice)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 0;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE
                  | D3D11_BIND_RENDER_TARGET;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }


        {
            D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
            textureRTVDesc.Format = m_format;
            textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            textureRTVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateRenderTargetView(m_texture.Get(), &textureRTVDesc, m_rtv.GetAddressOf());
            assert(m_rtv && (m_name + " rtv").c_str());
            SetDebugObjectName(m_rtv.Get(), m_name + " RTV");
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
            textureUAVDesc.Format = m_format;
            textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            textureUAVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateUnorderedAccessView(
                  m_texture.Get(), &textureUAVDesc, m_uav.GetAddressOf());
            assert(m_uav && (m_name + " uav").c_str());
            SetDebugObjectName(m_uav.Get(), m_name + " UAV");
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            textureSRVDesc.Texture2D.MipLevels = 1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " SRV");
        }
    }

    void Initialize(ID3D11Device *pDevice, const T *const pData)
    {
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

            D3D11_SUBRESOURCE_DATA initData = { 0 };
            initData.pSysMem = (const void *)pData;
            initData.SysMemPitch = m_width * sizeof(T);
            initData.SysMemSlicePitch = 0;

            pDevice->CreateTexture2D(&textureDesc, &initData, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " texture").c_str());
            SetDebugObjectName(m_texture.Get(), m_name);
        }


        {
            D3D11_RENDER_TARGET_VIEW_DESC textureRTVDesc;
            textureRTVDesc.Format = m_format;
            textureRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            textureRTVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateRenderTargetView(m_texture.Get(), &textureRTVDesc, m_rtv.GetAddressOf());
            assert(m_rtv && (m_name + " rtv").c_str());
            SetDebugObjectName(m_rtv.Get(), m_name + " RTV");
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
            textureUAVDesc.Format = m_format;
            textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            textureUAVDesc.Texture2D.MipSlice = 0;
            pDevice->CreateUnorderedAccessView(
                  m_texture.Get(), &textureUAVDesc, m_uav.GetAddressOf());
            assert(m_uav && (m_name + " uav").c_str());
            SetDebugObjectName(m_uav.Get(), m_name + " UAV");
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
            textureSRVDesc.Format = m_format;
            textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            textureSRVDesc.Texture2D.MipLevels = -1;
            textureSRVDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(
                  m_texture.Get(), &textureSRVDesc, m_srv.GetAddressOf());
            assert(m_srv && (m_name + " srv").c_str());
            SetDebugObjectName(m_srv.Get(), m_name + " SRV");
        }
    }

    void GenerateMips(ID3D11DeviceContext *const pDeviceContext)
    {
        assert(m_srv.Get());
        pDeviceContext->GenerateMips(m_srv.Get());
    }

   private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv = nullptr;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uav = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv = nullptr;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::string m_name = "";
};

template<typename T>
class ManagedTexture2DStaging {
   public:
    ManagedTexture2DStaging<T>(
          const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(typeid(T).name())
    {
    }

    ManagedTexture2DStaging<T>(const std::string &name, const uint32_t width, const uint32_t height,
          const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(name)
    {
    }

    ManagedTexture2DStaging<T>(
          const char *pName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format)
        : m_texture(nullptr)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_name(pName)
    {
    }

    ID3D11Texture2D *GetTexture() { return m_texture.Get(); }

    void Initialize(ID3D11Device *pDevice)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.BindFlags = 0;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            textureDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " staging").c_str());
            SetDebugObjectName(m_texture.Get(), m_name + "_STAGING");
        }
        m_cachedValues.resize(m_width * m_height, 0);
    }

    void Initialize(ID3D11Device *pDevice, const T *const pData)
    {
        // Random State Texture
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            textureDesc.Width = m_width;
            textureDesc.Height = m_height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = m_format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.BindFlags = 0;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            textureDesc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA initData = { 0 };
            initData.pSysMem = (const void *)pData;
            initData.SysMemPitch = m_width * sizeof(T);
            initData.SysMemSlicePitch = 0;

            pDevice->CreateTexture2D(&textureDesc, &initData, m_texture.GetAddressOf());
            assert(m_texture && (m_name + " staging").c_str());
            SetDebugObjectName(m_texture.Get(), m_name + "_STAGING");
        }

        m_cachedValues.resize(m_width * m_height);
        for (int i = 0; i < m_cachedValues.size(); i++) {
            m_cachedValues[i] = pData[i];
        }
    }
    void UpdateCachedValues(ID3D11DeviceContext *const pDeviceContext)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        pDeviceContext->Map(m_texture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
        const T *const mappedDataPtr = (T *)mappedResource.pData;
        for (int i = 0; i < m_cachedValues.size(); i++) {
            m_cachedValues[i] = mappedDataPtr[i];
        }
        pDeviceContext->Unmap(m_texture.Get(), 0);
    }

    const std::vector<T> &CachedValues() const { return m_cachedValues; }

   private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture = nullptr;
    std::vector<T> m_cachedValues;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    std::string m_name = "";
};
}