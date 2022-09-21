#include "Mesh.h"
#include "FMath/Vector3.h"

#include <FMath/FMath.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>
#include <unordered_map>

namespace Farlor {
Mesh::Mesh()
    : m_vertices {}
    , m_indices {}
{
}

void Mesh::InitializeBuffers(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext)
{
    HRESULT hr;
    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(VertexPositionUVNormal) * (int)m_vertices.size();
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = m_vertices.data();
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    // Create the vertex buffer.
    hr = pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer);
    if (FAILED(hr)) {
        std::cout << "Failed to create vertex buffer: " << std::endl;
    }

    ZeroMemory(&bufferDesc, sizeof(bufferDesc));
    ZeroMemory(&InitData, sizeof(InitData));
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(unsigned int) * (int)m_indices.size();
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    // Fill in the subresource data.
    InitData.pSysMem = m_indices.data();
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    // Create the index buffer.
    hr = pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer);
    if (FAILED(hr)) {
        std::cout << "Failed to create index buffer: " << std::endl;
    }
}

void Mesh::Render(ID3D11Device *pDevice, ID3D11DeviceContext *pDeviceContext)
{
    unsigned int stride = 0;
    unsigned int offset = 0;
    stride = sizeof(VertexPositionUVNormal);
    offset = 0;

    pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pDeviceContext->DrawIndexed((unsigned int)m_indices.size(), 0, 0);
}

bool Mesh::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
      uint32_t stackCount, Mesh &mesh)
{
    mesh.m_vertices.clear();
    mesh.m_indices.clear();

    float stackHeight = height / stackCount;
    float radiusStep = (topRadius - bottomRadius) / stackCount;

    uint32_t ringCount = stackCount + 1;

    // Compute vertices
    for (uint32_t i = 0; i < ringCount; ++i) {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;

        // Vertics of ring
        float dTheta = 2.0f * static_cast<float>(M_PI) / sliceCount;
        for (uint32_t j = 0; j < sliceCount; ++j) {
            VertexPositionUVNormal vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.m_position = Vector3(r * c, y, r * s);

            vertex.m_uv.x = (float)j / sliceCount;
            vertex.m_uv.y = 1.0f - (float)i / stackCount;

            Vector3 tangent(-1.0f * s, 0.0f, c);
            float dr = bottomRadius - topRadius;
            Vector3 bitangent(dr * c, -1.0f * height, dr * s);

            vertex.m_normal = tangent.Cross(bitangent).Normalized();
        }
    }

    uint32_t ringVertexCount = sliceCount + 1;

    for (uint32_t i = 0; i < stackCount; ++i) {
        for (uint32_t j = 0; j < stackCount; ++j) {
            mesh.m_indices.push_back(i * ringVertexCount + j);
            mesh.m_indices.push_back((i + 1) * ringVertexCount + j);
            mesh.m_indices.push_back((i + 1) * ringVertexCount + j + 1);
            mesh.m_indices.push_back(i * ringVertexCount + j);
            mesh.m_indices.push_back((i + 1) * ringVertexCount + j + 1);
            mesh.m_indices.push_back(i * ringVertexCount + j + 1);
        }
    }

    //  No end cap geometry yet, need to do!!

    return true;
}

bool Mesh::LoadObjTinyObj(const std::string &filename, Mesh &mesh)
{
    // tinyobj::attrib_t attrib;
    // std::vector<tinyobj::shape_t> shapes;
    // std::vector<tinyobj::material_t> materials;
    // std::string warnings;
    // std::string errors;

    // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, filename.c_str())) {
    //     std::cout << "Failed to load model:" << filename << std::endl;
    //     std::cout << "Warnings: " << warnings << std::endl;
    //     std::cout << "Errors: " << errors << std::endl;
    // }

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "resources/meshes/";

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();

    uint32_t vtxCount = 0;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                VertexPositionUVNormal vertex;

                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                vertex.m_position = Farlor::Vector3(vx, vy, vz);

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    vertex.m_normal = Farlor::Vector3(nx, ny, nz);
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    vertex.m_uv = Farlor::Vector2(tx, ty);
                }
                mesh.m_vertices.push_back(vertex);
                mesh.m_indices.push_back(vtxCount);
                vtxCount++;
            }
            index_offset += fv;
        }
    }
    return true;
}
}
