#pragma once
#include <map>

class Vertex
{
public:
	Vector3	p;
	double	tex[ 2 ];
};

struct MapTexture
{
	uint32_t id;
	uint32_t width;
	uint32_t height;
	std::string name;
};

struct MapPoly
{
	Vector3 min, max;
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
	Vector3 min, max;
	std::vector<float> positionBuffer; // three floats per pos
	std::vector<float> normalBuffer; // three floats per normal
	std::vector<float> texcoordBuffer; // two floats per UV
	uint32_t textureId;

};

// Merges all polygons that share the same texture
std::vector<Primitive> GeneratePrimitives(std::vector<MapPoly> polygons);


using PropertyName = std::string;
using PropertyValue = std::string;

struct MapEntity
{
	std::map<PropertyName, PropertyValue> properties;
	std::vector<MapPoly> polys;
	std::vector<Primitive> primitives;
	Vector3 bboxMin;
	Vector3 bboxMax;
};
