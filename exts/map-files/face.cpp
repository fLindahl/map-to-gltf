#include "map.h"

std::vector<MapPoly> DerivePolys(std::vector<MapFace> const& faces)
{
	std::vector<MapPoly> ret;

	// Create the polygons from the faces
	ret.resize(faces.size());

	// Loop through faces and create polygons
	MapPoly* pi = ret.data();

	for (size_t i = 0; i < faces.size() - 2; i++)
	{
		MapFace const* fi = &faces[i];
		MapPoly* pj = pi + 1;

		for (size_t j = i + 1; j < faces.size() - 1; j++)
		{
			MapFace const* fj = &faces[j];
			MapPoly* pk = pj + 1;

			for (size_t k = j + 1; k < faces.size(); k++)
			{
				MapFace const* fk = &faces[k];
				Vector3 p;

				if (fi->plane.GetIntersection(fj->plane, fk->plane, p))
				{
					size_t faceIndex;
					for (faceIndex = 0; faceIndex < faces.size(); faceIndex++)
					{
						MapFace const* f = &faces[faceIndex];

						if (f->plane.ClassifyPoint(p) == Plane::eCP::FRONT)
							break;
					}

					if (faceIndex == faces.size())
					{
						// The point is not outside the brush
						Vertex v = { p };
						pi->AddVertex(v);
						pj->AddVertex(v);
						pk->AddVertex(v);
					}
				}
				pk++;
			}
			pj++;
		}
		pi++;
	}

	return ret;
}
