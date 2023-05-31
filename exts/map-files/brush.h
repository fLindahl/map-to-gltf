#pragma once
#include <cstdint>

struct MapFace
{
	Plane plane;
	Plane texAxis[2];
	double texScale[2];
	uint32_t textureId;
};

std::vector<MapPoly> DerivePolys(std::vector<MapFace> const& faces);

struct MapBrush
{
	Vector3 min, max;
	std::vector<MapPoly> polys;
	
	void CalculateAABB();
};

namespace CSG
{
	std::vector<MapPoly> Union(std::vector<MapBrush> const& brushes);
}