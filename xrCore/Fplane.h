#pragma once

class Fplane
{
public:
	Fvector n;
	float d;

public:
	IC Fplane& set(Fplane &P)
	{
		n.set(P.n);
		d = P.d;
		return *this;
	}

	IC bool similar(Fplane &P, float eps_n = EPS, float eps_d = EPS)
	{
		return (n.similar(P.n, eps_n) && (_abs(d - P.d) < eps_d));
	}

	ICF Fplane& build(const Fvector3 &v1, const Fvector &v2, const Fvector &v3)
	{
		Fvector t1, t2;
		n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3)).normalize();
		d = -n.dotproduct(v1);
		return *this;
	}

	ICF Fplane& build_precise(const Fvector &v1, const Fvector &v2, const Fvector &v3)
	{
		Fvector t1, t2;
		n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3));
		exact_normalize(n);
		d = -n.dotproduct(v1);
		return *this;
	}

	ICF Fplane& build(const Fvector &_p, const Fvector &_n)
	{
		d = -n.normalize(_n).dotproduct(_p);
		return *this;
	}

	ICF Fplane& build_unit_normal(const Fvector &_p, const Fvector &_n)
	{
		VERIFY(fsimilar(_n.magnitude(), 1, EPS));
		d = -n.set(_n).dotproduct(_p);
		return *this;
	}

	IC Fplane& project(Fvector &pdest, Fvector &psrc)
	{
		pdest.mad(psrc, n, -classify(psrc));
		return *this;
	}

	ICF float classify(const Fvector &v) const
	{
		return n.dotproduct(v) + d;
	}

	IC Fplane& normalize()
	{
		float denom = 1.f / n.magnitude();
		n.mul(denom);
		d *= denom;
		return *this;
	}

	IC float distance(const Fvector &v)
	{
		return _abs(classify(v));
	}

	IC bool intersectRayDist(const Fvector &P, const Fvector &D, float &dist)
	{
		float numer = classify(P);
		float denom = n.dotproduct(D);

		if (_abs(denom) < EPS_S) // normal is orthogonal to vector3, cant intersect
			return FALSE;

		dist = -(numer / denom);
		return ((dist > 0.f) || fis_zero(dist));
	}

	ICF bool intersectRayPoint(const Fvector &P, const Fvector &D, Fvector &dest)
	{
		float numer = classify(P);
		float denom = n.dotproduct(D);

		if (_abs(denom) < EPS_S)
			return FALSE; // normal is orthogonal to vector3, cant intersect
		else
		{
			float dist = -(numer / denom);
			dest.mad(P, D, dist);
			return ((dist > 0.f) || fis_zero(dist));
		}
	}

	IC bool intersect(const Fvector &u, const Fvector &v, // segment
		Fvector &isect)							// intersection point
	{
		float denom, dist;
		Fvector t;

		t.sub(v, u);
		denom = n.dotproduct(t);

		if (_abs(denom) < EPS)
			return false; // they are parallel

		dist = -(n.dotproduct(u) + d) / denom;

		if (dist < -EPS || dist > 1 + EPS)
			return false;

		isect.mad(u, t, dist);
		return true;
	}

	IC bool intersect_2(const Fvector &u, const Fvector &v, // segment
		Fvector &isect)							// intersection point
	{
		float dist1, dist2;
		Fvector t;

		dist1 = n.dotproduct(u) + d;
		dist2 = n.dotproduct(v) + d;

		if (dist1 * dist2 < 0.0f)
			return false;

		t.sub(v, u);
		isect.mad(u, t, dist1 / _abs(dist1 - dist2));

		return true;
	}

	IC Fplane& transform(Fmatrix &M)
	{
		// rotate the normal
		M.transform_dir(n);
		// slide the offset
		d -= M.c.dotproduct(n);
		return *this;
	}
};

ICF bool _valid(const Fplane&s) { return _valid(s.n) && _valid(s.d); }
