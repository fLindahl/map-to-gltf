#pragma once


////////////////////////////////////////////////////////////////////
// Class definitions
////////////////////////////////////////////////////////////////////

class Face
{
private:
	Face	*m_pNext;

public:
	Plane	plane;
	Plane	texAxis[ 2 ];
	double	texScale[ 2 ];
	Texture	*pTexture;

	Face *GetNext ( ) const { return m_pNext; }

	void AddFace ( Face *pFace_ );
	void SetNext ( Face *pFace_ );

	Poly *GetPolys ( );

	bool IsLast ( ) const;

	Face ( );
	~Face ( );
};


class Brush
{
private:
	Vector3	min, max;	// AABB around brush used to quickly reject far off brushes in CSG
	Brush	*m_pNext;
	Poly	*m_pPolys;

public:
	Brush *GetNext ( ) const { return m_pNext; }
	Poly *GetPolys ( ) { return m_pPolys; }
	Brush *CopyList ( ) const;

	void SetNext ( Brush *pBrush_ );
	void AddPoly ( Poly *pPoly_ );

	Poly *MergeList ( );
	void ClipToBrush ( Brush *pBrush_, bool bClipOnPlane_ );
	void CalculateAABB ( );

	unsigned int GetNumberOfPolys ( ) const;
	unsigned int GetNumberOfBrushes ( ) const;

	bool IsLast ( ) const;
	bool AABBIntersect ( Brush *pBrush_ );

	Brush ( );
	~Brush ( );
};

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