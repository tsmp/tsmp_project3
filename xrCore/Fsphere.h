#pragma once

struct Fsphere
{
	Fvector P;
	float R;

public:
	IC void set(const Fvector &_P, float _R)
	{
		P.set(_P);
		R = _R;
	}

	IC void set(const Fsphere &S)
	{
		P.set(S.P);
		R = S.R;
	}

	IC void identity()
	{
		P.set(0, 0, 0);
		R = 1;
	}

	enum ERP_Result
	{
		rpNone = 0,
		rpOriginInside = 1,
		rpOriginOutside = 2,
		fcv_forcedword = u32(-1)
	};

	// Ray-sphere intersection
	ICF ERP_Result intersect(const Fvector &S, const Fvector &D, float range, int &quantity, float afT[2]) const
	{
		// set up quadratic Q(t) = a*t^2 + 2*b*t + c
		Fvector kDiff;
		kDiff.sub(S, P);
		float fA = range * range;
		float fB = kDiff.dotproduct(D) * range;
		float fC = kDiff.square_magnitude() - R * R;
		ERP_Result result = rpNone;
		float fDiscr = fB * fB - fA * fC;

		if (fDiscr < 0.0f)		
			quantity = 0;		
		else if (fDiscr > 0.0f)
		{
			float fRoot = _sqrt(fDiscr);
			float fInvA = (1.0f) / fA;
			afT[0] = range * (-fB - fRoot) * fInvA;
			afT[1] = range * (-fB + fRoot) * fInvA;

			if (afT[0] >= 0.0f)
			{
				quantity = 2;
				result = rpOriginOutside;
			}
			else if (afT[1] >= 0.0f)
			{
				quantity = 1;
				afT[0] = afT[1];
				result = rpOriginInside;
			}
			else
				quantity = 0;
		}
		else
		{
			afT[0] = range * (-fB / fA);

			if (afT[0] >= 0.0f)
			{
				quantity = 1;
				result = rpOriginOutside;
			}
			else
				quantity = 0;
		}
		return result;
	}

	ICF ERP_Result intersect_full(const Fvector &start, const Fvector &dir, float &dist) const
	{
		int quantity;
		float afT[2];
		Fsphere::ERP_Result result = intersect(start, dir, dist, quantity, afT);

		if (result == Fsphere::rpOriginInside || ((result == Fsphere::rpOriginOutside) && (afT[0] < dist)))
		{
			switch (result)
			{
			case Fsphere::rpOriginInside:
				dist = afT[0] < dist ? afT[0] : dist;
				break;
			case Fsphere::rpOriginOutside:
				dist = afT[0];
				break;
			}
		}

		return result;
	}

	ICF ERP_Result intersect(const Fvector &start, const Fvector &dir, float &dist) const
	{
		int quantity;
		float afT[2];
		ERP_Result result = intersect(start, dir, dist, quantity, afT);

		if (rpNone != result)
		{
			VERIFY(quantity > 0);
			if (afT[0] < dist)
			{
				dist = afT[0];
				return result;
			}
		}

		return rpNone;
	}

	IC ERP_Result intersect2(const Fvector &S, const Fvector &D, float &range) const
	{
		Fvector Q;
		Q.sub(P, S);

		float R2 = R * R;
		float c2 = Q.square_magnitude();
		float v = Q.dotproduct(D);
		float d = R2 - (c2 - v * v);

		if (d > 0.f)
		{
			float _range = v - _sqrt(d);

			if (_range < range)
			{
				range = _range;
				return (c2 < R2) ? rpOriginInside : rpOriginOutside;
			}
		}

		return rpNone;
	}

	ICF bool intersect(const Fvector &S, const Fvector &D) const
	{
		Fvector Q;
		Q.sub(P, S);

		float c = Q.magnitude();
		float v = Q.dotproduct(D);
		float d = R * R - (c * c - v * v);
		return (d > 0);
	}

	ICF bool intersect(const Fsphere &S) const
	{
		float SumR = R + S.R;
		return P.distance_to_sqr(S.P) < SumR * SumR;
	}

	IC bool contains(const Fvector &PT) const
	{
		return P.distance_to_sqr(PT) <= (R * R + EPS_S);
	}

	// returns true if this wholly contains the argument sphere
	IC bool contains(const Fsphere &S) const
	{
		// can't contain a sphere that's bigger than me !
		const float RDiff = R - S.R;

		if (RDiff < 0)
			return false;

		return (P.distance_to_sqr(S.P) <= RDiff * RDiff);
	}

	// return's volume of sphere
	IC float volume() const
	{
		return float(PI_MUL_4 / 3) * (R * R * R);
	}
};

ICF bool _valid(const Fsphere &s) { return _valid(s.P) && _valid(s.R); }
