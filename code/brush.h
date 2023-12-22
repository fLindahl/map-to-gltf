#pragma once
#include <cstdint>

struct Face
{
	Plane plane;
	Plane texAxis[2];
	double texScale[2];
	uint32_t textureId;
};

std::vector<Poly> DerivePolys(std::vector<Face> const& faces);

struct Brush
{
	Vector3 min, max;
	std::vector<Poly> polys;
	
	void CalculateAABB();
};

namespace CSG
{
	std::vector<Poly> Union(std::vector<Brush> const& brushes);
}