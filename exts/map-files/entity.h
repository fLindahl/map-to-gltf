#pragma once
#include <map>
#include <cstdint>

class Vertex
{
public:
    Vector3	p;
    double	tex[2];
};

struct Texture
{
    uint32_t id;
    uint32_t width;
    uint32_t height;
    std::string name;
};

struct Poly
{
    Vector3 min{ 1e30, 1e30, 1e30 };
    Vector3 max{ -1e30, -1e30, -1e30 };
    std::vector<Vertex> verts;
    Plane plane;
    uint32_t textureId;

    void AddVertex(Vertex const& vert);

    void Triangulate();

    bool CalculatePlane();
    void SortVerticesCW();
    void CalculateTextureCoordinates(int const texWidth, int const texHeight, Plane const texAxis[2], double const texScale[2]);
};

struct Primitive
{
    Vector3 min{ 1e30, 1e30, 1e30 };
    Vector3 max{ -1e30, -1e30, -1e30 };
    std::vector<float> positionBuffer; // three floats per pos
    std::vector<float> normalBuffer; // three floats per normal
    std::vector<float> texcoordBuffer; // two floats per UV
    std::vector<uint32_t> indexBuffer;
    uint32_t textureId;

};

// Merges all polygons that share the same texture
std::vector<Primitive> GeneratePrimitives(std::vector<Poly> polygons);


using PropertyName = std::string;
using PropertyValue = std::string;

struct Entity
{
    std::map<PropertyName, PropertyValue> properties;
    std::vector<Poly> polys;
    std::vector<Primitive> primitives;
    Vector3 bboxMin;
    Vector3 bboxMax;
};
