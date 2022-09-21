#pragma once

#include <dxgi1_6.h>
#include <d3d11_1.h>

#include "../../Core/Window.h"

namespace Farlor
{
    struct DXGIModeDesc
    {
    public:
        DXGIModeDesc(int width, int height);

        void Default(int width, int height);

    public:
        DXGI_MODE_DESC m_dxgiModeDesc;
    };

    struct DXGISwapChainDesc
    {
    public:
        DXGISwapChainDesc(DXGIModeDesc dxgiModeDesc, GameWindow &gameWindow);

        void Default(DXGIModeDesc dxgiModeDesc, GameWindow &gameWindow);

    public:
        DXGI_SWAP_CHAIN_DESC m_dxgiSwapChainDesc;
    };

    class RasterizerStateDesc
    {
    public:
        RasterizerStateDesc();
        void Default();

    public:
        D3D11_RASTERIZER_DESC m_rasterDesc;
    };
}
