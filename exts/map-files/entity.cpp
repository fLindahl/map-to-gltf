#include "map.h"

std::vector<Primitive> GeneratePrimitives(std::vector<MapPoly> polygons)
{
	for (size_t i = 0; i < polygons.size(); i++)
	{
		MapPoly& poly = polygons[i];
		poly.Triangulate();	
	}

	std::vector<Primitive> ret;

	// texture id -> ret vector index
	std::unordered_map<uint32_t, uint32_t> map;

	for (size_t i = 0; i < polygons.size(); i++)
	{
		MapPoly const& poly = polygons[i];

		Primitive* prim;

		auto it = map.find(poly.textureId);
		if (it == map.end())
		{
			map[poly.textureId] = (uint32_t)ret.size();
			Primitive primitive;
			primitive.min = poly.min;
			primitive.max = poly.max;
			primitive.textureId = poly.textureId;

			ret.push_back(primitive);
			prim = &ret.back();
		}
		else
		{
			prim = &ret[it->second];
		}

		// merge
		for (size_t i = 0; i < poly.verts.size(); i++)
		{
			// NOTE: currently assuming vertices are in triangle list order, without indexbuffer
			prim->positionBuffer.push_back((float)poly.verts[i].p.x);
			prim->positionBuffer.push_back((float)poly.verts[i].p.y);
			prim->positionBuffer.push_back((float)poly.verts[i].p.z);
			prim->normalBuffer.push_back((float)poly.plane.n.x);
			prim->normalBuffer.push_back((float)poly.plane.n.y);
			prim->normalBuffer.push_back((float)poly.plane.n.z);
			prim->texcoordBuffer.push_back((float)poly.verts[i].tex[0]);
			prim->texcoordBuffer.push_back((float)poly.verts[i].tex[1]);
		}

		prim->min.minimize(poly.min);
		prim->max.maximize(poly.max);
	}

	return ret;
}
