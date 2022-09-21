#pragma once

#include <d3d11_1.h>

#include "CBs.h"

#include <FMath/FMath.h>

#include <vector>

namespace Farlor
{
    template <typename T>
    class ScalarField2D
    {
    public:
        struct cbPerObject
        {
            DirectX::XMMATRIX WorldMatrix;
            DirectX::XMMATRIX InvTransposeWorld;
            DirectX::XMMATRIX WVP;
        };

    public:

        // A simple grid of data
        ScalarField2D(const uint32_t numX, const uint32_t numZ, const T& defaultValue)
            : m_numPointsX(numX)
            , m_numPointsZ(numZ)
        {
            m_scalarValues.resize(numX * numZ);
            for (auto& value : m_scalarValues)
            {
                value = defaultValue;
            }
        }

        uint32_t GetResolutionX() const {
            return m_numPointsX;
        }

        uint32_t GetResolutionZ() const {
            return m_numPointsZ;
        }

        const std::vector<T>& GetScalarValues() const {
            return m_scalarValues;
        }

        std::vector<T>& AccessScalarValues() {
            return m_scalarValues;
        }

    private:
        uint32_t m_numPointsX = 0;
        uint32_t m_numPointsZ = 0;

        std::vector<T> m_scalarValues;
    };
}
