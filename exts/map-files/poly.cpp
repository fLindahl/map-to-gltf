#include "map.h"
#include <cstring>

void Poly::AddVertex(Vertex const& vert)
{
	this->min.Minimize(vert.p);
	this->min.Maximize(vert.p);
	this->verts.push_back(vert);
}

void Poly::Triangulate()
{
	uint32_t const faceNumVertices = (uint32_t)this->verts.size();

	std::vector<Vertex> vertexBuffer;
	vertexBuffer.reserve(this->verts.size() + 2); // pretty standard that we have a polygon of 4 vertices, and want to expand it to 6.

	uint32_t triOffset = 0;

	Vertex v0 = this->verts[triOffset + 0];
	Vertex v1 = this->verts[triOffset + 1];
	Vertex v2 = this->verts[triOffset + 2];

	vertexBuffer.push_back(v0);
	vertexBuffer.push_back(v1);
	vertexBuffer.push_back(v2);

	triOffset++;
	uint32_t vert = faceNumVertices;
	// loop over remaining vertices that make up any polygon that is > 3 vertices
	while (vert > 3)
	{
		uint32_t vertexOffset = 3 + faceNumVertices - vert;
		// fan triangulate
		Vertex v3 = this->verts[vertexOffset];
		vertexBuffer.push_back(v0);
		vertexBuffer.push_back(v2);
		vertexBuffer.push_back(v3);
		vert--;
		v2 = v3;
	}

	this->verts = vertexBuffer;
}

bool Poly::CalculatePlane()
{
	Vector3	centerOfMass;
	double magnitude;
	int i;
	int j;

	if (this->verts.size() < 3)
	{
		std::cout << "Polygon has less than 3 vertices!" << std::endl;
		return false;
	}

	plane.n.x = 0.0f;
	plane.n.y = 0.0f;
	plane.n.z = 0.0f;
	centerOfMass.x = 0.0f;
	centerOfMass.y = 0.0f;
	centerOfMass.z = 0.0f;

	for (i = 0; i < this->verts.size(); i++)
	{
		j = i + 1;

		if (j >= this->verts.size())
		{
			j = 0;
		}

		plane.n.x += (this->verts[i].p.y - this->verts[j].p.y) * (this->verts[i].p.z + this->verts[j].p.z);
		plane.n.y += (this->verts[i].p.z - this->verts[j].p.z) * (this->verts[i].p.x + this->verts[j].p.x);
		plane.n.z += (this->verts[i].p.x - this->verts[j].p.x) * (this->verts[i].p.y + this->verts[j].p.y);

		centerOfMass.x += this->verts[i].p.x;
		centerOfMass.y += this->verts[i].p.y;
		centerOfMass.z += this->verts[i].p.z;
	}

	if ((fabs(plane.n.x) < epsilon) && (fabs(plane.n.y) < epsilon) &&
		(fabs(plane.n.z) < epsilon))
	{
		return false;
	}

	magnitude = sqrt(plane.n.x * plane.n.x + plane.n.y * plane.n.y + plane.n.z * plane.n.z);

	if (magnitude < epsilon)
	{
		return false;
	}

	plane.n.x /= magnitude;
	plane.n.y /= magnitude;
	plane.n.z /= magnitude;

	centerOfMass.x /= (double)this->verts.size();
	centerOfMass.y /= (double)this->verts.size();
	centerOfMass.z /= (double)this->verts.size();

	plane.d = -(centerOfMass.Dot(plane.n));

	return true;
}

void Poly::SortVerticesCW()
{
	// Calculate center of polygon
	Vector3	center;

	for (int i = 0; i < this->verts.size(); i++)
		center = center + this->verts[i].p;

	center = center / static_cast<double>(this->verts.size());

	// Sort vertices
	int numVertices = static_cast<int>(this->verts.size());
	for (int i = 0; i < numVertices - 2; i++)
	{
		Vector3	a;
		Plane	p;
		double	SmallestAngle = -1;
		int		Smallest = -1;

		a = this->verts[i].p - center;
		a.Normalize();

		p.PointsToPlane(this->verts[i].p, center, center + plane.n);

		for (int j = i + 1; j < numVertices; j++)
		{
			if (p.ClassifyPoint(this->verts[j].p) != Plane::eCP::BACK)
			{
				Vector3	b;
				double	Angle;

				b = this->verts[j].p - center;
				b.Normalize();

				Angle = a.Dot(b);

				if (Angle > SmallestAngle)
				{
					SmallestAngle = Angle;
					Smallest = j;
				}
			}
		}

		if (Smallest == -1)
		{
			std::cout << "Error: Degenerate polygon!" << std::endl;
			abort();
		}

		Vertex	t = this->verts[Smallest];
		this->verts[Smallest] = this->verts[i + 1];
		this->verts[i + 1] = t;
	}

	// Check if vertex order needs to be reversed for back-facing polygon
	Plane	oldPlane = plane;

	CalculatePlane();

	if (plane.n.Dot(oldPlane.n) < 0)
	{
		size_t j = this->verts.size();

		for (size_t i = 0; i < j / 2; i++)
		{
			Vertex v = this->verts[i];
			this->verts[i] = this->verts[j - i - 1];
			this->verts[j - i - 1] = v;
		}
	}
}

void Poly::CalculateTextureCoordinates(int const texWidth, int const texHeight, Plane const texAxis[2], double const texScale[2])
{
	// Calculate texture coordinates
	for (int i = 0; i < this->verts.size(); i++)
	{
		double U, V;

		U = texAxis[0].n.Dot(this->verts[i].p);
		U = U / ((double)texWidth) / texScale[0];
		U = U + (texAxis[0].d / (double)texWidth);

		V = texAxis[1].n.Dot(this->verts[i].p);
		V = V / ((double)texHeight) / texScale[1];
		V = V + (texAxis[1].d / (double)texHeight);

		this->verts[i].tex[0] = U;
		this->verts[i].tex[1] = V;
	}

	// Check which axis should be normalized
	bool	bDoU = true;
	bool	bDoV = true;

	for (int i = 0; i < this->verts.size(); i++)
	{
		if ((this->verts[i].tex[0] < 1) && (this->verts[i].tex[0] > -1))
		{
			bDoU = false;
		}

		if ((this->verts[i].tex[1] < 1) && (this->verts[i].tex[1] > -1))
		{
			bDoV = false;
		}
	}

	// Calculate coordinate nearest to 0
	if (bDoU || bDoV)
	{
		double	NearestU = 0;
		double	U = this->verts[0].tex[0];

		double	NearestV = 0;
		double	V = this->verts[0].tex[1];

		if (bDoU)
		{
			if (U > 1)
			{
				NearestU = floor(U);
			}
			else
			{
				NearestU = ceil(U);
			}
		}

		if (bDoV)
		{
			if (V > 1)
			{
				NearestV = floor(V);
			}
			else
			{
				NearestV = ceil(V);
			}
		}

		for (int i = 0; i < this->verts.size(); i++)
		{
			if (bDoU)
			{
				U = this->verts[i].tex[0];

				if (fabs(U) < fabs(NearestU))
				{
					if (U > 1)
					{
						NearestU = floor(U);
					}
					else
					{
						NearestU = ceil(U);
					}
				}
			}

			if (bDoV)
			{
				V = this->verts[i].tex[1];

				if (fabs(V) < fabs(NearestV))
				{
					if (V > 1)
					{
						NearestV = floor(V);
					}
					else
					{
						NearestV = ceil(V);
					}
				}
			}
		}

		// Normalize texture coordinates!
		for (int i = 0; i < this->verts.size(); i++)
		{
			this->verts[i].tex[0] = this->verts[i].tex[0] - NearestU;
			this->verts[i].tex[1] = this->verts[i].tex[1] - NearestV;
		}
	}
}
