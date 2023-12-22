#pragma once

#include <iostream>
#include <cmath>
#include <array>

////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////

const double	epsilon = 1e-5;		// Used to compensate for floating point inaccuracy.
const double	scale	= 64;		// Scale

////////////////////////////////////////////////////////////////////
// Name:		Vector3
// Description:	3D vector class with all operators implemented.
//				"double" is used for greater accuracy.
////////////////////////////////////////////////////////////////////
class Vector3
{
public:
	double x, y, z;

	void Minimize(Vector3 const& rhs)
	{
		x = x < rhs.x ? x : rhs.x;
		y = y < rhs.y ? y : rhs.y;
		z = z < rhs.z ? z : rhs.z;
	}

	void Maximize(Vector3 const& rhs)
	{
		x = x > rhs.x ? x : rhs.x;
		y = y > rhs.y ? y : rhs.y;
		z = z > rhs.z ? z : rhs.z;
	}

	const bool operator == ( const Vector3 &arg_ ) const
	{
		if ( ( x == arg_.x ) && ( y == arg_.y ) && ( z == arg_.z ) )
		{
			return true;
		}

		return false;
	}

	const Vector3 operator - ( const Vector3 &arg_ ) const
	{
		Vector3 temp;

		temp.x = x - arg_.x;
		temp.y = y - arg_.y;
		temp.z = z - arg_.z;

		return temp;
	}

	const Vector3 operator + ( const Vector3 &arg_ ) const
	{
		Vector3 temp;

		temp.x = x + arg_.x;
		temp.y = y + arg_.y;
		temp.z = z + arg_.z;

		return temp;
	}

	const Vector3 operator * ( const double fArg_ ) const
	{
		Vector3 temp;

		temp.x = x * fArg_;
		temp.y = y * fArg_;
		temp.z = z * fArg_;

		return temp;
	}

	const Vector3 operator / ( const double fArg_ ) const
	{
		Vector3 temp;

		temp.x = x / fArg_;
		temp.y = y / fArg_;
		temp.z = z / fArg_;

		return temp;
	}

	const Vector3 operator - ( ) const
	{
		Vector3 temp;

		temp.x = -x;
		temp.y = -y;
		temp.z = -z;

		return temp;
	}

	const double Dot ( const Vector3 &arg_ ) const
	{
		return x * arg_.x + y * arg_.y + z * arg_.z;
	}

	const Vector3 Cross ( const Vector3 &arg_ ) const
	{
		Vector3 temp;

		temp.x = y * arg_.z - z * arg_.y;
		temp.y = z * arg_.x - x * arg_.z;
		temp.z = x * arg_.y - y * arg_.x;

		return temp;
	}

	const double Magnitude ( ) const
	{
		return sqrt ( x * x + y * y + z * z );
	}

	const double MagnitudeSquared ( ) const
	{
		return ( x * x + y * y + z * z );
	}

	void Normalize ( )
	{
		const double fLength = Magnitude ( );

		x /= fLength;
		y /= fLength;
		z /= fLength;

		return;
	}

	Vector3 ( )
	{
		x = 0;
		y = 0;
		z = 0;
	}

	Vector3 ( const double x, const double y, const double z )
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
};


////////////////////////////////////////////////////////////////////
// Name:		Plane
// Description:	Plane class.  "double" is used for greater accuracy.
//				Follows N dot P + D = 0 equation.
////////////////////////////////////////////////////////////////////
class Plane
{
public:
	Vector3	n;	// Plane normal
	double	d;	// D

	enum eCP { FRONT = 0, BACK, ONPLANE };

	Plane ( )
	{
		d = 0;
	}

	Plane ( const Vector3 n, const double d )
	{
		this->n = n;
		this->d = d;
	}

	Plane ( const Vector3 &a, const Vector3 &b, const Vector3 &c )
	{
		n = ( c - b ).Cross ( a - b );
		n.Normalize ( );

		d = -n.Dot ( a );
	}

	void PointsToPlane ( const Vector3 &a, const Vector3 &b, const Vector3 &c )
	{
		n = ( c - b ).Cross ( a - b );
		n.Normalize ( );

		d = -n.Dot ( a );
	}

	double DistanceToPlane ( const Vector3 &v ) const
	{
		return ( n.Dot ( v ) + d );
	}

	eCP ClassifyPoint ( const Vector3 &v ) const
	{
		double Distance = DistanceToPlane ( v );

		if ( Distance > epsilon )
		{
			return eCP::FRONT;
		}
		else if ( Distance < -epsilon )
		{
			return eCP::BACK;
		}

		return eCP::ONPLANE;
	}

	bool GetIntersection ( const Plane &a, const Plane &b, Vector3 &v ) const
	{
		double	denom;

		denom = n.Dot ( a.n.Cross ( b.n ) );

		if ( fabs ( denom ) < epsilon )
		{
			return false;
		}

		v = ( ( a.n.Cross ( b.n ) ) * -d - ( b.n.Cross ( n ) ) * a.d - ( n.Cross ( a.n ) ) * b.d ) / denom;

		return true;
	}

	bool GetIntersection ( const Vector3 &Start, const Vector3 &End, Vector3 &Intersection, double &Percentage ) const
	{
		Vector3	Direction = End - Start;
		double	Num, Denom;

		Direction.Normalize ( );

		Denom = n.Dot ( Direction );

		if ( fabs ( Denom ) < epsilon )
		{
			return false;
		}

		Num				= -DistanceToPlane ( Start );
		Percentage		= Num / Denom;
		Intersection	= Start + ( Direction * Percentage );
		Percentage		= Percentage / ( End - Start ).Magnitude ( );

		return true;
	}
};

////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////

inline constexpr float
deg2rad(float d) noexcept
{
	constexpr double pi = 3.1415926535897932384626433832795028841971693993751;
	return (float)((d * pi) / 180.0f);
}

inline std::array<float, 4> QuatFromEuler(float x, float y, float z) noexcept
{
	x = deg2rad(x);
	y = deg2rad(y);
	z = deg2rad(z);

	const float cx = cos(x * 0.5f);
	const float sx = sin(x * 0.5f);
	const float cy = cos(y * 0.5f);
	const float sy = sin(y * 0.5f);
	const float cz = cos(z * 0.5f);
	const float sz = sin(z * 0.5f);

	return {
		  sx * cy * cz - cx * sy * sz
		, cx * sy * cz + sx * cy * sz
		, cx * cy * sz - sx * sy * cz
		, cx * cy * cz + sx * sy * sz
	};
}