#include "map.h"

std::vector<Primitive> GeneratePrimitives(std::vector<Poly> polygons)
{
	//for (size_t i = 0; i < polygons.size(); i++)
	//{
	//	Poly& poly = polygons[i];
	//	poly.Triangulate();	
	//}

	std::vector<Primitive> ret;

	// texture id -> ret vector index
	std::unordered_map<uint32_t, uint32_t> map;

	for (size_t i = 0; i < polygons.size(); i++)
	{
		Poly const& poly = polygons[i];

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

		uint32_t indexOffset = (uint32_t)prim->positionBuffer.size() / 3;
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

		// Generate indices
		uint32_t const faceNumVertices = (uint32_t)poly.verts.size();
		
		uint32_t i0 = indexOffset + 0;
		uint32_t i1 = indexOffset + 1;
		uint32_t i2 = indexOffset + 2;

		prim->indexBuffer.push_back(i0);
		prim->indexBuffer.push_back(i1);
		prim->indexBuffer.push_back(i2);

		uint32_t vert = faceNumVertices;
		// loop over remaining vertices that make up any polygon that is > 3 vertices
		while (vert > 3)
		{
			// fan triangulate
			uint32_t i3 = indexOffset + 3 + faceNumVertices - vert;
			prim->indexBuffer.push_back(i0);
			prim->indexBuffer.push_back(i2);
			prim->indexBuffer.push_back(i3);
			vert--;
			i2 = i3;
		}


		prim->min.Minimize(poly.min);
		prim->max.Maximize(poly.max);
	}

	return ret;
}
