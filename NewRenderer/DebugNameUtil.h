#pragma once

#include <d3d11.h>
#include <string>

#include "../Util/Logger.h"

template<UINT TNameLength>
inline void SetDebugObjectName(ID3D11DeviceChild *resource, const char (&name)[TNameLength])
{
    HRESULT result = resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
    if (FAILED(result)) {
        std::string error("Failed to name: ");
        error += std::string(name);
        FARLOR_LOG_WARNING(error.c_str());
    }
}

inline void SetDebugObjectName(ID3D11DeviceChild *resource, const std::string debugName)
{
    HRESULT result = resource->SetPrivateData(
          WKPDID_D3DDebugObjectName, (UINT)debugName.size(), debugName.c_str());
    if (FAILED(result)) {
        std::string error("Failed to name: ");
        error += debugName;
        FARLOR_LOG_WARNING(error.c_str());
    }
}