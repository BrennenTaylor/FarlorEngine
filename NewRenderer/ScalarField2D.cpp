//// NOTE: For some fucking reason this must be included before any d3d11 stuff gets included. Why you ask? No clue.
//#include <d3dcompiler.h>
//#include "ScalarField2D.h"
//
//#include "Renderer.h"
//#include "Vertex.h"
//
//#include <DirectXMath.h>
//
//#include "../Util/Logger.h"
//
//namespace Farlor
//{
//    extern Renderer g_RenderingSystem;
//
//    // TODO: Work on better interface for creation, why seperate worldHeightValue
//    // NOTE: ScalarField2D is in world space, y axis is up
//    // Bottom left corner is at (corner.x, corner.y, corner.z)
//    // Top right is (corner.x + m_width, corner.y, corner.z + m_height)
//    // For mapping purposes, bottom left uv is (0, 0)
//    // Top right is (1, 1)
//
//    template <typename T>
//    ScalarField2D::ScalarField2D(float width, float height, const Farlor::Vector3 bottomLeftCorner, float localHeight, uint32_t dx, uint32_t dz)
//        : m_width{ width }
//        , m_height{ height }
//        , m_localBottomLeftCorner{ bottomLeftCorner.x, localHeight, bottomLeftCorner.z }
//        , m_localTopRightCorner{ bottomLeftCorner.x + width, localHeight, bottomLeftCorner.z + height }
//        , m_numPointsX{ dx }
//        , m_numPointsZ{ dz }
//        , m_scalarValues(dx * dz)
//    {
//    }
//
//    ScalarField2D::ScalarField2D(const Farlor::Vector3 bottomLeftCorner, const Farlor::Vector3 topRightCorner, float localHeight, uint32_t dx, uint32_t dz)
//        : m_width{ (topRightCorner.x - bottomLeftCorner.x) }
//        , m_height{ (topRightCorner.z - bottomLeftCorner.z) }
//        , m_localBottomLeftCorner{ bottomLeftCorner.x, localHeight, bottomLeftCorner.z }
//        , m_localTopRightCorner{ topRightCorner.x, localHeight, topRightCorner.z }
//        , m_numPointsX{ dx }
//        , m_numPointsZ{ dz }
//        , m_scalarValues(dx* dz)
//    {
//    }
//}
