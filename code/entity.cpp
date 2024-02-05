#include <algorithm>
#include "map.h"

//------------------------------------------------------------------------------
/**
*/
void
GenerateIndices(Primitive* prim, size_t faceNumVertices, uint32_t indexOffset)
{
    uint32_t i0 = indexOffset;
    uint32_t i1 = indexOffset + 1;
    uint32_t i2 = indexOffset + 2;
    prim->indexBuffer.push_back(i0);
    prim->indexBuffer.push_back(i1);
    prim->indexBuffer.push_back(i2);

    uint32_t vert = static_cast<uint32_t>(faceNumVertices);
    while (vert > 3)
    {
        uint32_t i3 = indexOffset + 3 + static_cast<uint32_t>(faceNumVertices - vert);
        prim->indexBuffer.push_back(i0);
        prim->indexBuffer.push_back(i2);
        prim->indexBuffer.push_back(i3);
        vert--;
        i2 = i3;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AddPrimitive(std::vector<Primitive>& primitives, const Poly& poly, Vector3 origin, std::unordered_map<uint32_t, uint32_t>& map)
{
    Primitive* prim;
    auto it = map.find(poly.textureId);
    if (it == map.end())
    {
        map[poly.textureId] = static_cast<uint32_t>(primitives.size());
        Primitive primitive;
        primitive.min = poly.min;
        primitive.max = poly.max;
        primitive.textureId = poly.textureId;
        primitives.push_back(primitive);
        prim = &primitives.back();
    }
    else
    {
        prim = &primitives[it->second];
    }

    uint32_t indexOffset = static_cast<uint32_t>(prim->positionBuffer.size()) / 3;
    for (const auto& vert : poly.verts)
    {
        prim->positionBuffer.push_back(static_cast<float>(vert.p.x) - static_cast<float>(origin.x));
        prim->positionBuffer.push_back(static_cast<float>(vert.p.y) - static_cast<float>(origin.y));
        prim->positionBuffer.push_back(static_cast<float>(vert.p.z) - static_cast<float>(origin.z));
        prim->normalBuffer.push_back(static_cast<float>(poly.plane.n.x));
        prim->normalBuffer.push_back(static_cast<float>(poly.plane.n.y));
        prim->normalBuffer.push_back(static_cast<float>(poly.plane.n.z));
        prim->texcoordBuffer.push_back(static_cast<float>(vert.tex[0]));
        prim->texcoordBuffer.push_back(static_cast<float>(vert.tex[1]));
    }

    GenerateIndices(prim, poly.verts.size(), indexOffset);
    prim->min.Minimize(poly.min);
    prim->max.Maximize(poly.max);
}

//------------------------------------------------------------------------------
/**
*/
std::vector<Primitive>
GeneratePrimitives(const std::vector<Poly>& polygons, Vector3 origin)
{
    std::vector<Primitive> primitives;
    std::unordered_map<uint32_t, uint32_t> map;

    for (const auto& poly : polygons)
    {
        AddPrimitive(primitives, poly, origin, map);
    }
    return primitives;
}

//------------------------------------------------------------------------------
/**
*/
void
ScalePrimitives(std::vector<Primitive>& primitives, float meshScale)
{
    for (auto& primitive : primitives)
    {
        for (auto& position : primitive.positionBuffer)
        {
            position *= meshScale;
        }

        primitive.max = primitive.max * meshScale;
        primitive.min = primitive.min * meshScale;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FlipXAxis(Primitive& primitive)
{
    for (size_t i = 0; i < primitive.positionBuffer.size(); i += 3)
    {
        primitive.positionBuffer[i] *= -1;
        primitive.normalBuffer[i] *= -1;
    }
    
    primitive.max.x *= -1;
    primitive.min.x *= -1;

    // ensure min.x < max.x
    std::swap(primitive.max.x, primitive.min.x);
}

//------------------------------------------------------------------------------
/**
*/
void
RecalculateRHPrimitives(std::vector<Primitive>& primitives)
{
    for (auto& primitive : primitives)
    {
        FlipXAxis(primitive);
        std::reverse(primitive.indexBuffer.begin(), primitive.indexBuffer.end());
    }
}

