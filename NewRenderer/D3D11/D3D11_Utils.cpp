#include "D3D11_Utils.h"

namespace Farlor
{
    DXGIModeDesc::DXGIModeDesc(int width, int height)
    {
        ZeroMemory(&m_dxgiModeDesc, sizeof(m_dxgiModeDesc));

        Default(width, height);
    }

    void DXGIModeDesc::Default(int width, int height)
    {
        m_dxgiModeDesc.Width = width;
        m_dxgiModeDesc.Height = height;
        m_dxgiModeDesc.RefreshRate.Numerator = 60;
        m_dxgiModeDesc.RefreshRate.Denominator = 1;
        m_dxgiModeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_dxgiModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        m_dxgiModeDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    }

    DXGISwapChainDesc::DXGISwapChainDesc(DXGIModeDesc dxgiModeDesc, GameWindow &window)
    {
        ZeroMemory(&m_dxgiSwapChainDesc, sizeof(m_dxgiSwapChainDesc));

        Default(dxgiModeDesc, window);
    }

    void DXGISwapChainDesc::Default(DXGIModeDesc dxgiModeDesc, GameWindow &window)
    {
        m_dxgiSwapChainDesc.BufferDesc = dxgiModeDesc.m_dxgiModeDesc;
        m_dxgiSwapChainDesc.SampleDesc.Count = 1;
        m_dxgiSwapChainDesc.SampleDesc.Quality = 0;
        m_dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        m_dxgiSwapChainDesc.BufferCount = 1;
        m_dxgiSwapChainDesc.OutputWindow = window.GetWindowHandle();
        m_dxgiSwapChainDesc.Windowed = true;
        m_dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    RasterizerStateDesc::RasterizerStateDesc()
    {
        ZeroMemory(&m_rasterDesc, sizeof(m_rasterDesc));

        Default();
    }

    void RasterizerStateDesc::Default()
    {
        m_rasterDesc.AntialiasedLineEnable = false;
        m_rasterDesc.CullMode = D3D11_CULL_BACK;
        m_rasterDesc.DepthBias = 0;
        m_rasterDesc.DepthBiasClamp = 0.0f;
        m_rasterDesc.DepthClipEnable = true;
        m_rasterDesc.FillMode = D3D11_FILL_SOLID;
        m_rasterDesc.FrontCounterClockwise = false;
        m_rasterDesc.MultisampleEnable = false;
        m_rasterDesc.ScissorEnable = false;
        m_rasterDesc.SlopeScaledDepthBias = false;
    }
}
