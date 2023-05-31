#include "map.h"

////////////////////////////////////////////////////////////////////
// Entity member functions
////////////////////////////////////////////////////////////////////

void Entity::WriteEntity ( std::ofstream &ofsFile_ ) const
{
/*	Entity:
	x char		Entity class (zero terminated)
	1 uint		Number of properties
	x Property	Entities properties
	1 uint		Number of polygons
	x Polygon	Polygons */

	ofsFile_ << m_pProperties->GetValue ( ) << ( char )0x00;

	unsigned int ui = GetNumberOfProperties ( ) - 1;

	ofsFile_.write ( ( char * )&ui, sizeof ( ui ) );

	if ( !m_pProperties->IsLast ( ) )
	{
		m_pProperties->GetNext ( )->WriteProperty ( ofsFile_ );
	}

	ui = GetNumberOfPolys ( );

	ofsFile_.write ( ( char * )&ui, sizeof ( ui ) );

	if ( GetNumberOfPolys ( ) > 0 )
	{
		m_pPolys->WritePoly ( ofsFile_ );
	}

	if ( !IsLast ( ) )
	{
		GetNext ( )->WriteEntity ( ofsFile_ );
	}
}


Entity::Entity ( )
{
	m_pNext			= NULL;
	m_pProperties	= NULL;
	m_pPolys		= NULL;
}


Entity::~Entity ( )
{
	if ( m_pProperties != NULL )
	{
		delete m_pProperties;
		m_pProperties = NULL;
	}

	if ( m_pPolys != NULL )
	{
		delete m_pPolys;
		m_pPolys = NULL;
	}

	if ( m_pNext != NULL )
	{
		delete m_pNext;
		m_pNext = NULL;
	}
}


bool Entity::IsLast ( ) const
{
	if ( m_pNext == NULL )
	{
		return true;
	}

	return false;
}


void Entity::AddEntity ( Entity *pEntity_ )
{
	if ( IsLast ( ) )
	{
		m_pNext = pEntity_;

		return;
	}

	Entity *pEntity = m_pNext;

	while ( !pEntity->IsLast ( ) )
	{
		pEntity = pEntity->GetNext ( );
	}

	pEntity->m_pNext = pEntity_;
}


void Entity::AddProperty ( Property *pProperty_ )
{
	if ( m_pProperties == NULL )
	{
		m_pProperties = pProperty_;

		return;
	}

	Property *pProperty = m_pProperties;

	while ( !pProperty->IsLast () )
	{
		pProperty = pProperty->GetNext ( );
	}

	pProperty->SetNext ( pProperty_ );
}


void Entity::AddPoly ( Poly *pPoly_ )
{
	if ( m_pPolys == NULL )
	{
		m_pPolys = pPoly_;

		return;
	}

	Poly *pPoly = m_pPolys;

	while ( !pPoly->IsLast ( ) )
	{
		pPoly = pPoly->GetNext ( );
	}

	pPoly->SetNext ( pPoly_ );
}


unsigned int Entity::GetNumberOfProperties ( ) const
{
	Property		*pProperty	= m_pProperties;
	unsigned int	uiCount		= 0;

	while ( pProperty != NULL )
	{
		pProperty = pProperty->GetNext ( );
		uiCount++;
	}

	return uiCount;
}


unsigned int Entity::GetNumberOfPolys ( ) const
{
	Poly			*pPoly		= m_pPolys;
	unsigned int	uiCount		= 0;

	while ( pPoly != NULL )
	{
		pPoly = pPoly->GetNext ( );
		uiCount++;
	}

	return uiCount;
}

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
