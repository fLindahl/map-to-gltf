#include "map.h"

//------------------------------------------------------------------------------
/**
*/
std::vector<Poly>
DerivePolys(std::vector<Face> const& faces)
{
	std::vector<Poly> ret;

	// Create the polygons from the faces
	ret.resize(faces.size());

	// Loop through faces and create polygons
	Poly* pi = ret.data();

	for (size_t i = 0; i < faces.size() - 2; i++)
	{
		Face const* fi = &faces[i];
		Poly* pj = pi + 1;

		for (size_t j = i + 1; j < faces.size() - 1; j++)
		{
			Face const* fj = &faces[j];
			Poly* pk = pj + 1;

			for (size_t k = j + 1; k < faces.size(); k++)
			{
				Face const* fk = &faces[k];
				Vector3 p;

				if (fi->plane.GetIntersection(fj->plane, fk->plane, p))
				{
					size_t faceIndex;
					for (faceIndex = 0; faceIndex < faces.size(); faceIndex++)
					{
						Face const* f = &faces[faceIndex];

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
