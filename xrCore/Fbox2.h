#pragma once

class Fbox2
{
public:
	union
	{
		struct
		{
			Fvector2 min;
			Fvector2 max;
		};

		struct
		{
			float x1, y1;
			float x2, y2;
		};
	};

	IC Fbox2& set(const Fvector2 &_min, const Fvector2 &_max)
	{
		min.set(_min);
		max.set(_max);
		return *this;
	}

	IC Fbox2& set(float x_1, float y_1, float x_2, float y_2)
	{
		min.set(x_1, y_1);
		max.set(x_2, y_2);
		return *this;
	}

	IC Fbox2& set(const Fbox2& b)
	{
		min.set(b.min);
		max.set(b.max);
		return *this;
	}

	IC Fbox2& null()
	{
		min.set(0.f, 0.f);
		max.set(0.f, 0.f);
		return *this;
	}

	IC Fbox2& identity()
	{
		min.set(-0.5, -0.5);
		max.set(0.5, 0.5);
		return *this;
	}

	IC Fbox2& invalidate()
	{
		min.set(type_max(float), type_max(float));
		max.set(type_min(float), type_min(float));
		return *this;
	}

	IC Fbox2& shrink(float s)
	{
		min.add(s);
		max.sub(s);
		return *this;
	}

	IC Fbox2& shrink(const Fvector2 &s)
	{
		min.add(s);
		max.sub(s);
		return *this;
	}

	IC Fbox2& grow(float s)
	{
		min.sub(s);
		max.add(s);
		return *this;
	}

	IC Fbox2& grow(const Fvector2 &s)
	{
		min.sub(s);
		max.add(s);
		return *this;
	}

	IC Fbox2& add(const Fvector2 &p)
	{
		min.add(p);
		max.add(p);
		return *this;
	}

	IC Fbox2& offset(const Fvector2 &p)
	{
		min.add(p);
		max.add(p);
		return *this;
	}

	IC Fbox2& add(const Fbox2& b, const Fvector2 &p)
	{
		min.add(b.min, p);
		max.add(b.max, p);
		return *this;
	}

	IC bool contains(float x, float y) { return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2); };
	IC bool contains(const Fvector2 &p) { return contains(p.x, p.y); };
	IC bool contains(const Fbox2& b) { return contains(b.min) && contains(b.max); };

	IC bool similar(const Fbox2& b) { return min.similar(b.min) && max.similar(b.max); };

	IC Fbox2& modify(const Fvector2 &p)
	{
		min.min(p);
		max.max(p);
		return *this;
	}

	IC Fbox2& merge(const Fbox2& b)
	{
		modify(b.min);
		modify(b.max);
		return *this;
	}

	IC Fbox2& merge(const Fbox2& b1, const Fbox2& b2)
	{
		invalidate();
		merge(b1);
		merge(b2);
		return *this;
	}

	IC void getsize(Fvector2 &R) const { R.sub(max, min); };

	IC void getradius(Fvector2 &R) const
	{
		getsize(R);
		R.mul(0.5f);
	}

	IC float getradius() const
	{
		Fvector2 R;
		getsize(R);
		R.mul(0.5f);
		return R.magnitude();
	}

	IC void getcenter(Fvector2 &C) const
	{
		C.x = (min.x + max.x) * 0.5f;
		C.y = (min.y + max.y) * 0.5f;
	}

	IC void getsphere(Fvector2 &C, float &R) const
	{
		getcenter(C);
		R = C.distance_to(max);
	}

	// Detects if this box intersect other
	IC bool intersect(const Fbox2& box)
	{
		if (max.x < box.min.x)
			return FALSE;
		if (max.y < box.min.y)
			return FALSE;
		if (min.x > box.max.x)
			return FALSE;
		if (min.y > box.max.y)
			return FALSE;
		return TRUE;
	};

	// Make's this box valid AABB
	IC Fbox2& sort()
	{
		float tmp;

		if (min.x > max.x)
		{
			tmp = min.x;
			min.x = max.x;
			max.x = tmp;
		}

		if (min.y > max.y)
		{
			tmp = min.y;
			min.y = max.y;
			max.y = tmp;
		}

		return *this;
	}

	// Does the vector3 intersects box
	IC bool Pick(const Fvector2 &start, const Fvector2 &dir)
	{
		float alpha, xt, yt;
		Fvector2 rvmin, rvmax;

		rvmin.sub(min, start);
		rvmax.sub(max, start);

		if (!fis_zero(dir.x))
		{
			alpha = rvmin.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y && yt <= rvmax.y)
				return true;

			alpha = rvmax.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y && yt <= rvmax.y)
				return true;
		}

		if (!fis_zero(dir.y))
		{
			alpha = rvmin.y / dir.y;
			xt = alpha * dir.x;

			if (xt >= rvmin.x && xt <= rvmax.x)
				return true;

			alpha = rvmax.y / dir.y;
			xt = alpha * dir.x;

			if (xt >= rvmin.x && xt <= rvmax.x)
				return true;
		}

		return false;
	}

	ICF bool pick_exact(const Fvector2 &start, const Fvector2 &dir)
	{
		float alpha, xt, yt;
		Fvector2 rvmin, rvmax;

		rvmin.sub(min, start);
		rvmax.sub(max, start);

		if (_abs(dir.x) != 0)
		{
			alpha = rvmin.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
				return true;

			alpha = rvmax.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
				return true;
		}

		if (_abs(dir.y) != 0)
		{
			alpha = rvmin.y / dir.y;
			xt = alpha * dir.x;

			if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
				return true;

			alpha = rvmax.y / dir.y;
			xt = alpha * dir.x;

			if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
				return true;
		}

		return false;
	}

	IC u32 &IR(float &x) { return (u32 &)x; }

	IC bool Pick2(const Fvector2 &origin, const Fvector2 &dir, Fvector2 &coord)
	{
		bool Inside = TRUE;
		Fvector2 MaxT;
		MaxT.x = MaxT.y = -1.0f;

		// Find candidate planes.
		{
			if (origin[0] < min[0])
			{
				coord[0] = min[0];
				Inside = FALSE;

				if (IR(dir[0]))
					MaxT[0] = (min[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
			}
			else if (origin[0] > max[0])
			{
				coord[0] = max[0];
				Inside = FALSE;

				if (IR(dir[0]))
					MaxT[0] = (max[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
			}
		}
		{
			if (origin[1] < min[1])
			{
				coord[1] = min[1];
				Inside = FALSE;

				if (IR(dir[1]))
					MaxT[1] = (min[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
			}
			else if (origin[1] > max[1])
			{
				coord[1] = max[1];
				Inside = FALSE;

				if (IR(dir[1]))
					MaxT[1] = (max[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
			}
		}

		// Ray origin inside bounding box
		if (Inside)
		{
			coord = origin;
			return true;
		}

		// Get largest of the maxT's for final choice of intersection
		u32 WhichPlane = 0;
		if (MaxT[1] > MaxT[0])
			WhichPlane = 1;

		// Check final candidate actually inside box
		if (IR(MaxT[WhichPlane]) & 0x80000000)
			return false;

		if (0 == WhichPlane)
		{
			// 1
			coord[1] = origin[1] + MaxT[0] * dir[1];
			if ((coord[1] < min[1]) || (coord[1] > max[1]))
				return false;
			return true;
		}
		else
		{
			// 0
			coord[0] = origin[0] + MaxT[1] * dir[0];
			if ((coord[0] < min[0]) || (coord[0] > max[0]))
				return false;
			return true;
		}
	}

	IC void getpoint(int index, Fvector2 &result)
	{
		switch (index)
		{
		case 0:
			result.set(min.x, min.y);
			break;
		case 1:
			result.set(min.x, max.y);
			break;
		case 2:
			result.set(max.x, min.y);
			break;
		case 3:
			result.set(max.x, max.y);
			break;
		default:
			result.set(0.f, 0.f);
			break;
		}
	}

	IC void getpoints(Fvector2 *result)
	{
		result[0].set(min.x, min.y);
		result[1].set(min.x, min.y);
		result[2].set(max.x, min.y);
		result[3].set(max.x, min.y);
	}
};

ICF bool _valid(const Fbox2 &c) { return _valid(c.min) && _valid(c.max); }
