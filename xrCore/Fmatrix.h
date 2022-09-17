#pragma once 

struct Fquaternion;

/*
*	DirectX-compliant, ie row-column order, ie m[Row][Col].
*	Same as:
*	m11  m12  m13  m14	first row.
*	m21  m22  m23  m24	second row.
*	m31  m32  m33  m34	third row.
*	m41  m42  m43  m44	fourth row.
*	Translation is (m41, m42, m43), (m14, m24, m34, m44) = (0, 0, 0, 1).
*	Stored in memory as m11 m12 m13 m14 m21...
*
*	Multiplication rules:
*
*	[x'y'z'1] = [xyz1][M]
*
*	x' = x*m11 + y*m21 + z*m31 + m41
*	y' = x*m12 + y*m22 + z*m32 + m42
*	z' = x*m13 + y*m23 + z*m33 + m43
*	1' =     0 +     0 +     0 + m44
*/

// NOTE_1: positive angle means clockwise rotation
// NOTE_2: mul(A,B) means transformation B, followed by A
// NOTE_3: I,J,K,C equals to R,N,D,T
// NOTE_4: The rotation sequence is ZXY

struct Fmatrix
{
public:
	union
	{
		struct
		{ // Direct definition
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};

		struct
		{
			Fvector i;
			float _14_;
			Fvector j;
			float _24_;
			Fvector k;
			float _34_;
			Fvector c;
			float _44_;
		};

		float m[4][4]; // Array
	};

	// Class members
	ICF Fmatrix& set(const Fmatrix &a)
	{
		i.set(a.i);
		_14_ = a._14;
		j.set(a.j);
		_24_ = a._24;
		k.set(a.k);
		_34_ = a._34;
		c.set(a.c);
		_44_ = a._44;
		return *this;
	}

	ICF Fmatrix& set(const Fvector &R, const Fvector &N, const Fvector &D, const Fvector &C)
	{
		i.set(R);
		_14_ = 0;
		j.set(N);
		_24_ = 0;
		k.set(D);
		_34_ = 0;
		c.set(C);
		_44_ = 1;
		return *this;
	}

	ICF Fmatrix& identity(void)
	{
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = 1;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = 1;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}

	IC Fmatrix& rotation(const Fquaternion &Q);
	IC Fmatrix& mk_xform(const Fquaternion &Q, const Fvector3 &V);

	// Multiply RES = A[4x4]*B[4x4] (WITH projection)
	ICF Fmatrix& mul(const Fmatrix &A, const Fmatrix &B)
	{
		VERIFY((this != &A) && (this != &B));
		m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2] + A.m[3][0] * B.m[0][3];
		m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2] + A.m[3][1] * B.m[0][3];
		m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2] + A.m[3][2] * B.m[0][3];
		m[0][3] = A.m[0][3] * B.m[0][0] + A.m[1][3] * B.m[0][1] + A.m[2][3] * B.m[0][2] + A.m[3][3] * B.m[0][3];

		m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2] + A.m[3][0] * B.m[1][3];
		m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2] + A.m[3][1] * B.m[1][3];
		m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2] + A.m[3][2] * B.m[1][3];
		m[1][3] = A.m[0][3] * B.m[1][0] + A.m[1][3] * B.m[1][1] + A.m[2][3] * B.m[1][2] + A.m[3][3] * B.m[1][3];

		m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2] + A.m[3][0] * B.m[2][3];
		m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2] + A.m[3][1] * B.m[2][3];
		m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2] + A.m[3][2] * B.m[2][3];
		m[2][3] = A.m[0][3] * B.m[2][0] + A.m[1][3] * B.m[2][1] + A.m[2][3] * B.m[2][2] + A.m[3][3] * B.m[2][3];

		m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0] * B.m[3][3];
		m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1] * B.m[3][3];
		m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2] * B.m[3][3];
		m[3][3] = A.m[0][3] * B.m[3][0] + A.m[1][3] * B.m[3][1] + A.m[2][3] * B.m[3][2] + A.m[3][3] * B.m[3][3];
		return *this;
	}

	// Multiply RES = A[4x3]*B[4x3] (no projection), faster than ordinary multiply
	ICF Fmatrix& mul_43(const Fmatrix &A, const Fmatrix &B)
	{
		VERIFY((this != &A) && (this != &B));
		m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2];
		m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2];
		m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2];
		m[0][3] = 0;

		m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2];
		m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2];
		m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2];
		m[1][3] = 0;

		m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2];
		m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2];
		m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2];
		m[2][3] = 0;

		m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0];
		m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1];
		m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2];
		m[3][3] = 1;
		return *this;
	}

	IC Fmatrix& mulA_44(const Fmatrix &A) // mul after
	{
		Fmatrix B;
		B.set(*this);
		mul(A, B);
		return *this;
	}

	IC Fmatrix& mulB_44(const Fmatrix &B) // mul before
	{
		Fmatrix A;
		A.set(*this);
		mul(A, B);
		return *this;
	}

	ICF Fmatrix& mulA_43(const Fmatrix&A) // mul after (no projection)
	{
		Fmatrix B;
		B.set(*this);
		mul_43(A, B);
		return *this;
	}

	ICF Fmatrix& mulB_43(const Fmatrix &B) // mul before (no projection)
	{
		Fmatrix A;
		A.set(*this);
		mul_43(A, B);
		return *this;
	}

	IC Fmatrix& invert(const Fmatrix &a)
	{ // important: this is 4x3 invert, not the 4x4 one
		// faster than self-invert
		float fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
					 a._12 * (a._21 * a._33 - a._23 * a._31) +
					 a._13 * (a._21 * a._32 - a._22 * a._31));

		VERIFY(_abs(fDetInv) > flt_zero);
		fDetInv = 1.0f / fDetInv;

		_11 = fDetInv * (a._22 * a._33 - a._23 * a._32);
		_12 = -fDetInv * (a._12 * a._33 - a._13 * a._32);
		_13 = fDetInv * (a._12 * a._23 - a._13 * a._22);
		_14 = 0.0f;

		_21 = -fDetInv * (a._21 * a._33 - a._23 * a._31);
		_22 = fDetInv * (a._11 * a._33 - a._13 * a._31);
		_23 = -fDetInv * (a._11 * a._23 - a._13 * a._21);
		_24 = 0.0f;

		_31 = fDetInv * (a._21 * a._32 - a._22 * a._31);
		_32 = -fDetInv * (a._11 * a._32 - a._12 * a._31);
		_33 = fDetInv * (a._11 * a._22 - a._12 * a._21);
		_34 = 0.0f;

		_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
		_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
		_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
		_44 = 1.0f;
		return *this;
	}

	IC bool invert_b(const Fmatrix &a)
	{ // important: this is 4x3 invert, not the 4x4 one
		// faster than self-invert
		float fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
					 a._12 * (a._21 * a._33 - a._23 * a._31) +
					 a._13 * (a._21 * a._32 - a._22 * a._31));

		if (_abs(fDetInv) <= flt_zero)
			return false;

		fDetInv = 1.0f / fDetInv;

		_11 = fDetInv * (a._22 * a._33 - a._23 * a._32);
		_12 = -fDetInv * (a._12 * a._33 - a._13 * a._32);
		_13 = fDetInv * (a._12 * a._23 - a._13 * a._22);
		_14 = 0.0f;

		_21 = -fDetInv * (a._21 * a._33 - a._23 * a._31);
		_22 = fDetInv * (a._11 * a._33 - a._13 * a._31);
		_23 = -fDetInv * (a._11 * a._23 - a._13 * a._21);
		_24 = 0.0f;

		_31 = fDetInv * (a._21 * a._32 - a._22 * a._31);
		_32 = -fDetInv * (a._11 * a._32 - a._12 * a._31);
		_33 = fDetInv * (a._11 * a._22 - a._12 * a._21);
		_34 = 0.0f;

		_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
		_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
		_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
		_44 = 1.0f;
		return true;
	}

	IC Fmatrix& invert() // slower than invert other matrix
	{
		Fmatrix a;
		a.set(*this);
		invert(a);
		return *this;
	}

	IC Fmatrix& transpose(const Fmatrix &matSource) // faster version of transpose
	{
		_11 = matSource._11;
		_12 = matSource._21;
		_13 = matSource._31;
		_14 = matSource._41;
		_21 = matSource._12;
		_22 = matSource._22;
		_23 = matSource._32;
		_24 = matSource._42;
		_31 = matSource._13;
		_32 = matSource._23;
		_33 = matSource._33;
		_34 = matSource._43;
		_41 = matSource._14;
		_42 = matSource._24;
		_43 = matSource._34;
		_44 = matSource._44;
		return *this;
	}

	IC Fmatrix& transpose() // self transpose - slower
	{
		Fmatrix a;
		a.set(*this);
		transpose(a);
		return *this;
	}

	IC Fmatrix& translate(const Fvector &Loc) // setup translation matrix
	{
		identity();
		c.set(Loc.x, Loc.y, Loc.z);
		return *this;
	}

	IC Fmatrix& translate(float _x, float _y, float _z) // setup translation matrix
	{
		identity();
		c.set(_x, _y, _z);
		return *this;
	}

	IC Fmatrix& translate_over(const Fvector &Loc) // modify only translation
	{
		c.set(Loc.x, Loc.y, Loc.z);
		return *this;
	}

	IC Fmatrix& translate_over(float _x, float _y, float _z) // modify only translation
	{
		c.set(_x, _y, _z);
		return *this;
	}

	IC Fmatrix& translate_add(const Fvector &Loc) // combine translation
	{
		c.add(Loc);
		return *this;
	}

	IC Fmatrix& scale(float x, float y, float z) // setup scale matrix
	{
		identity();
		m[0][0] = x;
		m[1][1] = y;
		m[2][2] = z;
		return *this;
	}

	IC Fmatrix& scale(const Fvector &v) // setup scale matrix
	{
		return scale(v.x, v.y, v.z);
	}

	IC Fmatrix& rotateX(float Angle) // rotation about X axis
	{
		float cosa = _cos(Angle);
		float sina = _sin(Angle);
		i.set(1, 0, 0);
		_14 = 0;
		j.set(0, cosa, sina);
		_24 = 0;
		k.set(0, -sina, cosa);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& rotateY(float Angle) // rotation about Y axis
	{
		float cosa = _cos(Angle);
		float sina = _sin(Angle);
		i.set(cosa, 0, -sina);
		_14 = 0;
		j.set(0, 1, 0);
		_24 = 0;
		k.set(sina, 0, cosa);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& rotateZ(float Angle) // rotation about Z axis
	{
		float cosa = _cos(Angle);
		float sina = _sin(Angle);
		i.set(cosa, sina, 0);
		_14 = 0;
		j.set(-sina, cosa, 0);
		_24 = 0;
		k.set(0, 0, 1);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& rotation(const Fvector &vdir, const Fvector &vnorm)
	{
		Fvector vright;
		vright.crossproduct(vnorm, vdir).normalize();
		m[0][0] = vright.x;
		m[0][1] = vright.y;
		m[0][2] = vright.z;
		m[0][3] = 0;
		m[1][0] = vnorm.x;
		m[1][1] = vnorm.y;
		m[1][2] = vnorm.z;
		m[1][3] = 0;
		m[2][0] = vdir.x;
		m[2][1] = vdir.y;
		m[2][2] = vdir.z;
		m[2][3] = 0;
		m[3][0] = 0;
		m[3][1] = 0;
		m[3][2] = 0;
		m[3][3] = 1;
		return *this;
	}

	IC Fmatrix& mapXYZ()
	{
		i.set(1, 0, 0);
		_14 = 0;
		j.set(0, 1, 0);
		_24 = 0;
		k.set(0, 0, 1);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& mapXZY()
	{
		i.set(1, 0, 0);
		_14 = 0;
		j.set(0, 0, 1);
		_24 = 0;
		k.set(0, 1, 0);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& mapYXZ()
	{
		i.set(0, 1, 0);
		_14 = 0;
		j.set(1, 0, 0);
		_24 = 0;
		k.set(0, 0, 1);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& mapYZX()
	{
		i.set(0, 1, 0);
		_14 = 0;
		j.set(0, 0, 1);
		_24 = 0;
		k.set(1, 0, 0);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& mapZXY()
	{
		i.set(0, 0, 1);
		_14 = 0;
		j.set(1, 0, 0);
		_24 = 0;
		k.set(0, 1, 0);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}
	IC Fmatrix& mapZYX()
	{
		i.set(0, 0, 1);
		_14 = 0;
		j.set(0, 1, 0);
		_24 = 0;
		k.set(1, 0, 0);
		_34 = 0;
		c.set(0, 0, 0);
		_44 = 1;
		return *this;
	}

	IC Fmatrix& rotation(const Fvector &axis, float Angle)
	{
		float Cosine = _cos(Angle);
		float Sine = _sin(Angle);
		m[0][0] = axis.x * axis.x + (1 - axis.x * axis.x) * Cosine;
		m[0][1] = axis.x * axis.y * (1 - Cosine) + axis.z * Sine;
		m[0][2] = axis.x * axis.z * (1 - Cosine) - axis.y * Sine;
		m[0][3] = 0;
		m[1][0] = axis.x * axis.y * (1 - Cosine) - axis.z * Sine;
		m[1][1] = axis.y * axis.y + (1 - axis.y * axis.y) * Cosine;
		m[1][2] = axis.y * axis.z * (1 - Cosine) + axis.x * Sine;
		m[1][3] = 0;
		m[2][0] = axis.x * axis.z * (1 - Cosine) + axis.y * Sine;
		m[2][1] = axis.y * axis.z * (1 - Cosine) - axis.x * Sine;
		m[2][2] = axis.z * axis.z + (1 - axis.z * axis.z) * Cosine;
		m[2][3] = 0;
		m[3][0] = 0;
		m[3][1] = 0;
		m[3][2] = 0;
		m[3][3] = 1;
		return *this;
	}

	// mirror X
	IC Fmatrix& mirrorX()
	{
		identity();
		m[0][0] = -1;
		return *this;
	}

	IC Fmatrix& mirrorX_over()
	{
		m[0][0] = -1;
		return *this;
	}

	IC Fmatrix& mirrorX_add()
	{
		m[0][0] *= -1;
		return *this;
	}

	// mirror Y
	IC Fmatrix& mirrorY()
	{
		identity();
		m[1][1] = -1;
		return *this;
	}

	IC Fmatrix& mirrorY_over()
	{
		m[1][1] = -1;
		return *this;
	}

	IC Fmatrix& mirrorY_add()
	{
		m[1][1] *= -1;
		return *this;
	}

	// mirror Z
	IC Fmatrix& mirrorZ()
	{
		identity();
		m[2][2] = -1;
		return *this;
	}

	IC Fmatrix& mirrorZ_over()
	{
		m[2][2] = -1;
		return *this;
	}
	IC Fmatrix& mirrorZ_add()
	{
		m[2][2] *= -1;
		return *this;
	}

	IC Fmatrix& mul(const Fmatrix &A, float v)
	{
		m[0][0] = A.m[0][0] * v;
		m[0][1] = A.m[0][1] * v;
		m[0][2] = A.m[0][2] * v;
		m[0][3] = A.m[0][3] * v;
		m[1][0] = A.m[1][0] * v;
		m[1][1] = A.m[1][1] * v;
		m[1][2] = A.m[1][2] * v;
		m[1][3] = A.m[1][3] * v;
		m[2][0] = A.m[2][0] * v;
		m[2][1] = A.m[2][1] * v;
		m[2][2] = A.m[2][2] * v;
		m[2][3] = A.m[2][3] * v;
		m[3][0] = A.m[3][0] * v;
		m[3][1] = A.m[3][1] * v;
		m[3][2] = A.m[3][2] * v;
		m[3][3] = A.m[3][3] * v;
		return *this;
	}

	IC Fmatrix& mul(float v)
	{
		m[0][0] *= v;
		m[0][1] *= v;
		m[0][2] *= v;
		m[0][3] *= v;
		m[1][0] *= v;
		m[1][1] *= v;
		m[1][2] *= v;
		m[1][3] *= v;
		m[2][0] *= v;
		m[2][1] *= v;
		m[2][2] *= v;
		m[2][3] *= v;
		m[3][0] *= v;
		m[3][1] *= v;
		m[3][2] *= v;
		m[3][3] *= v;
		return *this;
	}

	IC Fmatrix& div(const Fmatrix &A, float v)
	{
		VERIFY(_abs(v) > 0.000001f);
		return mul(A, 1.0f / v);
	}

	IC Fmatrix& div(float v)
	{
		VERIFY(_abs(v) > 0.000001f);
		return mul(1.0f / v);
	}

	// fov
	IC Fmatrix& build_projection(float fFOV, float fAspect, float fNearPlane, float fFarPlane)
	{
		return build_projection_HAT(tanf(fFOV / 2.f), fAspect, fNearPlane, fFarPlane);
	}

	// half_fov-angle-tangent
	IC Fmatrix& build_projection_HAT(float HAT, float fAspect, float fNearPlane, float fFarPlane)
	{
		VERIFY(_abs(fFarPlane - fNearPlane) > EPS_S);
		VERIFY(_abs(HAT) > EPS_S);

		float cot = float(1) / HAT;
		float w = fAspect * cot;
		float h = float(1) * cot;
		float Q = fFarPlane / (fFarPlane - fNearPlane);

		_11 = w;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = h;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = Q;
		_34 = 1.0f;
		_41 = 0;
		_42 = 0;
		_43 = -Q * fNearPlane;
		_44 = 0;
		return *this;
	}

	IC Fmatrix& build_projection_ortho(float w, float h, float zn, float zf)
	{
		_11 = float(2) / w;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = float(2) / h;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = float(1) / (zf - zn);
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = zn / (zn - zf);
		_44 = float(1);
		return *this;
	}

	IC Fmatrix& build_camera(const Fvector &vFrom, const Fvector &vAt, const Fvector &vWorldUp)
	{
		// Get the z basis vector3, which points straight ahead. This is the
		// difference from the eyepoint to the lookat point.
		Fvector vView;
		vView.sub(vAt, vFrom).normalize();

		// Get the dot product, and calculate the projection of the z basis
		// vector3 onto the up vector3. The projection is the y basis vector3.
		float fDotProduct = vWorldUp.dotproduct(vView);

		Fvector vUp;
		vUp.mul(vView, -fDotProduct).add(vWorldUp).normalize();

		// The x basis vector3 is found simply with the cross product of the y
		// and z basis vectors
		Fvector vRight;
		vRight.crossproduct(vUp, vView);

		// Start building the Device.mView. The first three rows contains the basis
		// vectors used to rotate the view to point at the lookat point
		_11 = vRight.x;
		_12 = vUp.x;
		_13 = vView.x;
		_14 = 0.0f;
		_21 = vRight.y;
		_22 = vUp.y;
		_23 = vView.y;
		_24 = 0.0f;
		_31 = vRight.z;
		_32 = vUp.z;
		_33 = vView.z;
		_34 = 0.0f;

		// Do the translation values (rotations are still about the eyepoint)
		_41 = -vFrom.dotproduct(vRight);
		_42 = -vFrom.dotproduct(vUp);
		_43 = -vFrom.dotproduct(vView);
		_44 = 1.0f;
		return *this;
	}

	IC Fmatrix& build_camera_dir(const Fvector &vFrom, const Fvector &vView, const Fvector &vWorldUp)
	{
		// Get the dot product, and calculate the projection of the z basis
		// vector3 onto the up vector3. The projection is the y basis vector3.
		float fDotProduct = vWorldUp.dotproduct(vView);

		Fvector vUp;
		vUp.mul(vView, -fDotProduct).add(vWorldUp).normalize();

		// The x basis vector3 is found simply with the cross product of the y
		// and z basis vectors
		Fvector vRight;
		vRight.crossproduct(vUp, vView);

		// Start building the Device.mView. The first three rows contains the basis
		// vectors used to rotate the view to point at the lookat point
		_11 = vRight.x;
		_12 = vUp.x;
		_13 = vView.x;
		_14 = 0.0f;
		_21 = vRight.y;
		_22 = vUp.y;
		_23 = vView.y;
		_24 = 0.0f;
		_31 = vRight.z;
		_32 = vUp.z;
		_33 = vView.z;
		_34 = 0.0f;

		// Do the translation values (rotations are still about the eyepoint)
		_41 = -vFrom.dotproduct(vRight);
		_42 = -vFrom.dotproduct(vUp);
		_43 = -vFrom.dotproduct(vView);
		_44 = 1.0f;
		return *this;
	}

	IC Fmatrix& inertion(const Fmatrix &mat, float v)
	{
		float iv = 1.f - v;

		for (int it = 0; it < 4; it++)
		{
			m[it][0] = m[it][0] * v + mat.m[it][0] * iv;
			m[it][1] = m[it][1] * v + mat.m[it][1] * iv;
			m[it][2] = m[it][2] * v + mat.m[it][2] * iv;
			m[it][3] = m[it][3] * v + mat.m[it][3] * iv;
		}

		return *this;
	}

	ICF void transform_tiny(Fvector &dest, const Fvector &v) const // preferred to use
	{
		dest.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
		dest.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
		dest.z = v.x * _13 + v.y * _23 + v.z * _33 + _43;
	}

	ICF void transform_tiny32(Fvector2 &dest, const Fvector &v) const // preferred to use
	{
		dest.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
		dest.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
	}

	ICF void transform_tiny23(Fvector &dest, const Fvector2 &v) const // preferred to use
	{
		dest.x = v.x * _11 + v.y * _21 + _41;
		dest.y = v.x * _12 + v.y * _22 + _42;
		dest.z = v.x * _13 + v.y * _23 + _43;
	}

	ICF void transform_dir(Fvector &dest, const Fvector &v) const // preferred to use
	{
		dest.x = v.x * _11 + v.y * _21 + v.z * _31;
		dest.y = v.x * _12 + v.y * _22 + v.z * _32;
		dest.z = v.x * _13 + v.y * _23 + v.z * _33;
	}

	IC void transform(Fvector4 &dest, const Fvector &v) const // preferred to use
	{
		dest.w = v.x * _14 + v.y * _24 + v.z * _34 + _44;
		dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) / dest.w;
		dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) / dest.w;
		dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) / dest.w;
	}

	IC void transform(Fvector &dest, const Fvector &v) const // preferred to use
	{
		float iw = 1.f / (v.x * _14 + v.y * _24 + v.z * _34 + _44);
		dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) * iw;
		dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) * iw;
		dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) * iw;
	}

	ICF void transform_tiny(Fvector &v) const
	{
		Fvector res;
		transform_tiny(res, v);
		v.set(res);
	}

	IC void transform(Fvector &v) const
	{
		Fvector res;
		transform(res, v);
		v.set(res);
	}

	ICF void transform_dir(Fvector &v) const
	{
		Fvector res;
		transform_dir(res, v);
		v.set(res);
	}

	ICF Fmatrix& setHPB(float h, float p, float b)
	{
		float _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

		_sh = _sin(h);
		_ch = _cos(h);
		_sp = _sin(p);
		_cp = _cos(p);
		_sb = _sin(b);
		_cb = _cos(b);
		_cc = _ch * _cb;
		_cs = _ch * _sb;
		_sc = _sh * _cb;
		_ss = _sh * _sb;

		i.set(_cc - _sp * _ss, -_cp * _sb, _sp * _cs + _sc);
		_14_ = 0;
		j.set(_sp * _sc + _cs, _cp * _cb, _ss - _sp * _cc);
		_24_ = 0;
		k.set(-_cp * _sh, _sp, _cp * _ch);
		_34_ = 0;
		c.set(0, 0, 0);
		_44_ = 1;
		return *this;
	}

	IC Fmatrix& setXYZ(float x, float y, float z) { return setHPB(y, x, z); }
	IC Fmatrix& setXYZ(Fvector &xyz) { return setHPB(xyz.y, xyz.x, xyz.z); }
	IC Fmatrix& setXYZi(float x, float y, float z) { return setHPB(-y, -x, -z); }
	IC Fmatrix& setXYZi(Fvector &xyz) { return setHPB(-xyz.y, -xyz.x, -xyz.z); }
	
	IC void getHPB(float &h, float &p, float &b) const
	{
		float cy = _sqrt(j.y * j.y + i.y * i.y);

		if (cy > 16.0f * type_epsilon(float))
		{
			h = (float)-atan2(k.x, k.z);
			p = (float)-atan2(-k.y, cy);
			b = (float)-atan2(i.y, j.y);
		}
		else
		{
			h = (float)-atan2(-i.z, i.x);
			p = (float)-atan2(-k.y, cy);
			b = 0;
		}
	}

	IC void getHPB(Fvector &hpb) const { getHPB(hpb.x, hpb.y, hpb.z); }
	IC void getXYZ(float &x, float &y, float &z) const { getHPB(y, x, z); }
	IC void getXYZ(Fvector &xyz) const { getXYZ(xyz.x, xyz.y, xyz.z); }

	IC void getXYZi(float &x, float &y, float &z) const
	{
		getHPB(y, x, z);
		x *= -1.f;
		y *= -1.f;
		z *= -1.f;
	}

	IC void getXYZi(Fvector &xyz) const
	{
		getXYZ(xyz.x, xyz.y, xyz.z);
		xyz.mul(-1.f);
	}
};

ICF bool _valid(const Fmatrix &m)
{
	return _valid(m.i) && _valid(m._14_) &&
		   _valid(m.j) && _valid(m._24_) &&
		   _valid(m.k) && _valid(m._34_) &&
		   _valid(m.c) && _valid(m._44_);
}

extern XRCORE_API Fmatrix Fidentity;
