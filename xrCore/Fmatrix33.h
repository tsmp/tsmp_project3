#pragma once

struct Fmatrix33
{
public:
    union
    {
        struct
        { // Direct definition
            float _11, _12, _13;
            float _21, _22, _23;
            float _31, _32, _33;
        };

        struct
        {
            Fvector i;
            Fvector j;
            Fvector k;
        };

        float m[3][3]; // Array
    };

    // Class members
    IC Fmatrix33& set_rapid(const Fmatrix &a)
    {
        m[0][0] = a.m[0][0];
        m[0][1] = a.m[0][1];
        m[0][2] = -a.m[0][2];
        m[1][0] = a.m[1][0];
        m[1][1] = a.m[1][1];
        m[1][2] = -a.m[1][2];
        m[2][0] = -a.m[2][0];
        m[2][1] = -a.m[2][1];
        m[2][2] = a.m[2][2];
        return *this;
    }

    IC Fmatrix33& set(const Fmatrix33& a)
    {
        CopyMemory(this, &a, 9 * sizeof(float));
        return *this;
    }

    IC Fmatrix33& set(const Fmatrix &a)
    {
        _11 = a._11;
        _12 = a._12;
        _13 = a._13;
        _21 = a._21;
        _22 = a._22;
        _23 = a._23;
        _31 = a._31;
        _32 = a._32;
        _33 = a._33;
        return *this;
    }

    IC Fmatrix33& identity(void)
    {
        _11 = 1.f;
        _12 = 0.f;
        _13 = 0.f;
        _21 = 0.f;
        _22 = 1.f;
        _23 = 0.f;
        _31 = 0.f;
        _32 = 0.f;
        _33 = 1.f;
        return *this;
    }

    IC Fmatrix33& transpose(const Fmatrix33& matSource) // faster version of transpose
    {
        _11 = matSource._11;
        _12 = matSource._21;
        _13 = matSource._31;
        _21 = matSource._12;
        _22 = matSource._22;
        _23 = matSource._32;
        _31 = matSource._13;
        _32 = matSource._23;
        _33 = matSource._33;
        return *this;
    }

    IC Fmatrix33& transpose(const Fmatrix &matSource) // faster version of transpose
    {
        _11 = matSource._11;
        _12 = matSource._21;
        _13 = matSource._31;
        _21 = matSource._12;
        _22 = matSource._22;
        _23 = matSource._32;
        _31 = matSource._13;
        _32 = matSource._23;
        _33 = matSource._33;
        return *this;
    }

    IC Fmatrix33& transpose(void) // self transpose - slower
    {
        Fmatrix33 a;
        CopyMemory(&a, this, 9 * sizeof(float)); // save matrix
        transpose(a);
        return *this;
    }

    IC Fmatrix33& MxM(const Fmatrix33& M1, const Fmatrix33& M2)
    {
        m[0][0] = (M1.m[0][0] * M2.m[0][0] +
                   M1.m[0][1] * M2.m[1][0] +
                   M1.m[0][2] * M2.m[2][0]);
        m[1][0] = (M1.m[1][0] * M2.m[0][0] +
                   M1.m[1][1] * M2.m[1][0] +
                   M1.m[1][2] * M2.m[2][0]);
        m[2][0] = (M1.m[2][0] * M2.m[0][0] +
                   M1.m[2][1] * M2.m[1][0] +
                   M1.m[2][2] * M2.m[2][0]);
        m[0][1] = (M1.m[0][0] * M2.m[0][1] +
                   M1.m[0][1] * M2.m[1][1] +
                   M1.m[0][2] * M2.m[2][1]);
        m[1][1] = (M1.m[1][0] * M2.m[0][1] +
                   M1.m[1][1] * M2.m[1][1] +
                   M1.m[1][2] * M2.m[2][1]);
        m[2][1] = (M1.m[2][0] * M2.m[0][1] +
                   M1.m[2][1] * M2.m[1][1] +
                   M1.m[2][2] * M2.m[2][1]);
        m[0][2] = (M1.m[0][0] * M2.m[0][2] +
                   M1.m[0][1] * M2.m[1][2] +
                   M1.m[0][2] * M2.m[2][2]);
        m[1][2] = (M1.m[1][0] * M2.m[0][2] +
                   M1.m[1][1] * M2.m[1][2] +
                   M1.m[1][2] * M2.m[2][2]);
        m[2][2] = (M1.m[2][0] * M2.m[0][2] +
                   M1.m[2][1] * M2.m[1][2] +
                   M1.m[2][2] * M2.m[2][2]);
        return *this;
    }

    IC Fmatrix33& MTxM(const Fmatrix33& M1, const Fmatrix33& M2)
    {
        m[0][0] = (M1.m[0][0] * M2.m[0][0] +
                   M1.m[1][0] * M2.m[1][0] +
                   M1.m[2][0] * M2.m[2][0]);
        m[1][0] = (M1.m[0][1] * M2.m[0][0] +
                   M1.m[1][1] * M2.m[1][0] +
                   M1.m[2][1] * M2.m[2][0]);
        m[2][0] = (M1.m[0][2] * M2.m[0][0] +
                   M1.m[1][2] * M2.m[1][0] +
                   M1.m[2][2] * M2.m[2][0]);
        m[0][1] = (M1.m[0][0] * M2.m[0][1] +
                   M1.m[1][0] * M2.m[1][1] +
                   M1.m[2][0] * M2.m[2][1]);
        m[1][1] = (M1.m[0][1] * M2.m[0][1] +
                   M1.m[1][1] * M2.m[1][1] +
                   M1.m[2][1] * M2.m[2][1]);
        m[2][1] = (M1.m[0][2] * M2.m[0][1] +
                   M1.m[1][2] * M2.m[1][1] +
                   M1.m[2][2] * M2.m[2][1]);
        m[0][2] = (M1.m[0][0] * M2.m[0][2] +
                   M1.m[1][0] * M2.m[1][2] +
                   M1.m[2][0] * M2.m[2][2]);
        m[1][2] = (M1.m[0][1] * M2.m[0][2] +
                   M1.m[1][1] * M2.m[1][2] +
                   M1.m[2][1] * M2.m[2][2]);
        m[2][2] = (M1.m[0][2] * M2.m[0][2] +
                   M1.m[1][2] * M2.m[1][2] +
                   M1.m[2][2] * M2.m[2][2]);
        return *this;
    }

#define ROT(a, i, j, k, l)             \
    g = a.m[i][j];                     \
    h = a.m[k][l];                     \
    a.m[i][j] = g - s * (h + g * tau); \
    a.m[k][l] = h + s * (g - h * tau);

    int IC Meigen(Fvector &dout, Fmatrix33& a)
    {
        float tresh, theta, tau, t, sm, s, h, g, c;
        int nrot;
        Fvector b;
        Fvector z;
        Fmatrix33 v;
        Fvector d;

        v.identity();

        b.set(a.m[0][0], a.m[1][1], a.m[2][2]);
        d.set(a.m[0][0], a.m[1][1], a.m[2][2]);
        z.set(0, 0, 0);

        nrot = 0;
        int ii;

        for (ii = 0; ii < 50; ii++)
        {
            sm = 0.0f;
            sm += _abs(a.m[0][1]);
            sm += _abs(a.m[0][2]);
            sm += _abs(a.m[1][2]);
            
            if (sm == 0.0)
            {
                set(v);
                dout.set(d);
                return ii;
            }

            if (ii < 3)
                tresh = 0.2f * sm / (3.0f * 3.0f);
            else
                tresh = 0.0f;
            {
                g = 100.0f * _abs(a.m[0][1]);

                if (ii > 3 && _abs(d.x) + g == _abs(d.x) && _abs(d.y) + g == _abs(d.y))
                    a.m[0][1] = 0.0;
                else if (_abs(a.m[0][1]) > tresh)
                {
                    h = d.y - d.x;

                    if (_abs(h) + g == _abs(h))
                        t = (a.m[0][1]) / h;
                    else
                    {
                        theta = 0.5f * h / (a.m[0][1]);
                        t = 1.0f / (_abs(theta) + _sqrt(1.0f + theta * theta));

                        if (theta < 0.0f)
                            t = -t;
                    }

                    c = 1.0f / _sqrt(1 + t * t);
                    s = t * c;
                    tau = s / (1.0f + c);
                    h = t * a.m[0][1];
                    z.x -= h;
                    z.y += h;
                    d.x -= h;
                    d.y += h;
                    a.m[0][1] = 0.0f;
                    ROT(a, 0, 2, 1, 2);
                    ROT(v, 0, 0, 0, 1);
                    ROT(v, 1, 0, 1, 1);
                    ROT(v, 2, 0, 2, 1);
                    nrot++;
                }
            }
            {
                g = 100.0f * _abs(a.m[0][2]);

                if (ii > 3 && _abs(d.x) + g == _abs(d.x) && _abs(d.z) + g == _abs(d.z))
                    a.m[0][2] = 0.0f;
                else if (_abs(a.m[0][2]) > tresh)
                {
                    h = d.z - d.x;

                    if (_abs(h) + g == _abs(h))
                        t = (a.m[0][2]) / h;
                    else
                    {
                        theta = 0.5f * h / (a.m[0][2]);
                        t = 1.0f / (_abs(theta) + _sqrt(1.0f + theta * theta));

                        if (theta < 0.0f)
                            t = -t;
                    }

                    c = 1.0f / _sqrt(1 + t * t);
                    s = t * c;
                    tau = s / (1.0f + c);
                    h = t * a.m[0][2];
                    z.x -= h;
                    z.z += h;
                    d.x -= h;
                    d.z += h;
                    a.m[0][2] = 0.0f;
                    ROT(a, 0, 1, 1, 2);
                    ROT(v, 0, 0, 0, 2);
                    ROT(v, 1, 0, 1, 2);
                    ROT(v, 2, 0, 2, 2);
                    nrot++;
                }
            }
            {
                g = 100.0f * _abs(a.m[1][2]);

                if (ii > 3 && _abs(d.y) + g == _abs(d.y) && _abs(d.z) + g == _abs(d.z))
                    a.m[1][2] = 0.0f;
                else if (_abs(a.m[1][2]) > tresh)
                {
                    h = d.z - d.y;

                    if (_abs(h) + g == _abs(h))
                        t = (a.m[1][2]) / h;
                    else
                    {
                        theta = 0.5f * h / (a.m[1][2]);
                        t = 1.0f / (_abs(theta) + _sqrt(1.0f + theta * theta));

                        if (theta < 0.0)
                            t = -t;
                    }

                    c = 1.0f / _sqrt(1 + t * t);
                    s = t * c;
                    tau = s / (1.0f + c);
                    h = t * a.m[1][2];
                    z.y -= h;
                    z.z += h;
                    d.y -= h;
                    d.z += h;
                    a.m[1][2] = 0.0f;
                    ROT(a, 0, 1, 0, 2);
                    ROT(v, 0, 1, 0, 2);
                    ROT(v, 1, 1, 1, 2);
                    ROT(v, 2, 1, 2, 2);
                    nrot++;
                }
            }

            b.add(z);
            d.set(b);
            z.set(0, 0, 0);
        }
        //        Log.Msg("eigen: too many iterations in Jacobi transform (%d).\n", ii);
        return ii;
    }
#undef ROT

    //--------------------------------------------------------------------------------
    // other unused function
    //--------------------------------------------------------------------------------
    IC Fmatrix33& McolcMcol(int cr, const Fmatrix33& M, int c)
    {
        m[0][cr] = M.m[0][c];
        m[1][cr] = M.m[1][c];
        m[2][cr] = M.m[2][c];
        return *this;
    }

    IC Fmatrix33& MxMpV(const Fmatrix33& M1, const Fmatrix33& M2, const Fvector &T)
    {
        m[0][0] = (M1.m[0][0] * M2.m[0][0] +
                   M1.m[0][1] * M2.m[1][0] +
                   M1.m[0][2] * M2.m[2][0] + T.x);
        m[1][0] = (M1.m[1][0] * M2.m[0][0] +
                   M1.m[1][1] * M2.m[1][0] +
                   M1.m[1][2] * M2.m[2][0] + T.y);
        m[2][0] = (M1.m[2][0] * M2.m[0][0] +
                   M1.m[2][1] * M2.m[1][0] +
                   M1.m[2][2] * M2.m[2][0] + T.z);
        m[0][1] = (M1.m[0][0] * M2.m[0][1] +
                   M1.m[0][1] * M2.m[1][1] +
                   M1.m[0][2] * M2.m[2][1] + T.x);
        m[1][1] = (M1.m[1][0] * M2.m[0][1] +
                   M1.m[1][1] * M2.m[1][1] +
                   M1.m[1][2] * M2.m[2][1] + T.y);
        m[2][1] = (M1.m[2][0] * M2.m[0][1] +
                   M1.m[2][1] * M2.m[1][1] +
                   M1.m[2][2] * M2.m[2][1] + T.z);
        m[0][2] = (M1.m[0][0] * M2.m[0][2] +
                   M1.m[0][1] * M2.m[1][2] +
                   M1.m[0][2] * M2.m[2][2] + T.x);
        m[1][2] = (M1.m[1][0] * M2.m[0][2] +
                   M1.m[1][1] * M2.m[1][2] +
                   M1.m[1][2] * M2.m[2][2] + T.y);
        m[2][2] = (M1.m[2][0] * M2.m[0][2] +
                   M1.m[2][1] * M2.m[1][2] +
                   M1.m[2][2] * M2.m[2][2] + T.z);
        return *this;
    }

    IC Fmatrix33& Mqinverse(const Fmatrix33& M)
    {
        int ii, jj;

        for (ii = 0; ii < 3; ii++)
            for (jj = 0; jj < 3; jj++)
            {
                int i1 = (ii + 1) % 3;
                int i2 = (ii + 2) % 3;
                int j1 = (jj + 1) % 3;
                int j2 = (jj + 2) % 3;
                m[ii][jj] = (M.m[j1][i1] * M.m[j2][i2] - M.m[j1][i2] * M.m[j2][i1]);
            }
        return *this;
    }

    IC Fmatrix33& MxMT(const Fmatrix33& M1, const Fmatrix33& M2)
    {
        m[0][0] = (M1.m[0][0] * M2.m[0][0] +
                   M1.m[0][1] * M2.m[0][1] +
                   M1.m[0][2] * M2.m[0][2]);
        m[1][0] = (M1.m[1][0] * M2.m[0][0] +
                   M1.m[1][1] * M2.m[0][1] +
                   M1.m[1][2] * M2.m[0][2]);
        m[2][0] = (M1.m[2][0] * M2.m[0][0] +
                   M1.m[2][1] * M2.m[0][1] +
                   M1.m[2][2] * M2.m[0][2]);
        m[0][1] = (M1.m[0][0] * M2.m[1][0] +
                   M1.m[0][1] * M2.m[1][1] +
                   M1.m[0][2] * M2.m[1][2]);
        m[1][1] = (M1.m[1][0] * M2.m[1][0] +
                   M1.m[1][1] * M2.m[1][1] +
                   M1.m[1][2] * M2.m[1][2]);
        m[2][1] = (M1.m[2][0] * M2.m[1][0] +
                   M1.m[2][1] * M2.m[1][1] +
                   M1.m[2][2] * M2.m[1][2]);
        m[0][2] = (M1.m[0][0] * M2.m[2][0] +
                   M1.m[0][1] * M2.m[2][1] +
                   M1.m[0][2] * M2.m[2][2]);
        m[1][2] = (M1.m[1][0] * M2.m[2][0] +
                   M1.m[1][1] * M2.m[2][1] +
                   M1.m[1][2] * M2.m[2][2]);
        m[2][2] = (M1.m[2][0] * M2.m[2][0] +
                   M1.m[2][1] * M2.m[2][1] +
                   M1.m[2][2] * M2.m[2][2]);
        return *this;
    }

    IC Fmatrix33& MskewV(const Fvector &v)
    {
        m[0][0] = m[1][1] = m[2][2] = 0.0;
        m[1][0] = v.z;
        m[0][1] = -v.z;
        m[0][2] = v.y;
        m[2][0] = -v.y;
        m[1][2] = -v.x;
        m[2][1] = v.x;
        return *this;
    }

    IC const Fmatrix33& sMxVpV(Fvector &R, float s1, const Fvector &V1, const Fvector &V2) const
    {
        R.x = s1 * (m[0][0] * V1.x + m[0][1] * V1.y + m[0][2] * V1.z) + V2.x;
        R.y = s1 * (m[1][0] * V1.x + m[1][1] * V1.y + m[1][2] * V1.z) + V2.y;
        R.z = s1 * (m[2][0] * V1.x + m[2][1] * V1.y + m[2][2] * V1.z) + V2.z;
        return *this;
    }

    IC void MTxV(Fvector &R, const Fvector &V1) const
    {
        R.x = (m[0][0] * V1.x + m[1][0] * V1.y + m[2][0] * V1.z);
        R.y = (m[0][1] * V1.x + m[1][1] * V1.y + m[2][1] * V1.z);
        R.z = (m[0][2] * V1.x + m[1][2] * V1.y + m[2][2] * V1.z);
    }

    IC void MTxVpV(Fvector &R, const Fvector &V1, const Fvector &V2) const
    {
        R.x = (m[0][0] * V1.x + m[1][0] * V1.y + m[2][0] * V1.z + V2.x);
        R.y = (m[0][1] * V1.x + m[1][1] * V1.y + m[2][1] * V1.z + V2.y);
        R.z = (m[0][2] * V1.x + m[1][2] * V1.y + m[2][2] * V1.z + V2.z);
    }

    IC void MTxVmV(Fvector &R, const Fvector &V1, const Fvector &V2) const
    {
        R.x = (m[0][0] * V1.x + m[1][0] * V1.y + m[2][0] * V1.z - V2.x);
        R.y = (m[0][1] * V1.x + m[1][1] * V1.y + m[2][1] * V1.z - V2.y);
        R.z = (m[0][2] * V1.x + m[1][2] * V1.y + m[2][2] * V1.z - V2.z);
    }

    IC void sMTxV(Fvector &R, float s1, const Fvector &V1) const
    {
        R.x = s1 * (m[0][0] * V1.x + m[1][0] * V1.y + m[2][0] * V1.z);
        R.y = s1 * (m[0][1] * V1.x + m[1][1] * V1.y + m[2][1] * V1.z);
        R.z = s1 * (m[0][2] * V1.x + m[1][2] * V1.y + m[2][2] * V1.z);
    }

    IC void MxV(Fvector &R, const Fvector &V1) const
    {
        R.x = (m[0][0] * V1.x + m[0][1] * V1.y + m[0][2] * V1.z);
        R.y = (m[1][0] * V1.x + m[1][1] * V1.y + m[1][2] * V1.z);
        R.z = (m[2][0] * V1.x + m[2][1] * V1.y + m[2][2] * V1.z);
    }

    IC void transform_dir(Fvector &dest, const _vector2<float> &v) const // preferred to use
    {
        dest.x = v.x * _11 + v.y * _21;
        dest.y = v.x * _12 + v.y * _22;
        dest.z = v.x * _13 + v.y * _23;
    }

    IC void MxVpV(Fvector &R, const Fvector &V1, const Fvector &V2) const
    {
        R.x = (m[0][0] * V1.x + m[0][1] * V1.y + m[0][2] * V1.z + V2.x);
        R.y = (m[1][0] * V1.x + m[1][1] * V1.y + m[1][2] * V1.z + V2.y);
        R.z = (m[2][0] * V1.x + m[2][1] * V1.y + m[2][2] * V1.z + V2.z);
    }
};

ICF bool _valid(const Fmatrix33 &m)
{
    return _valid(m.i) &&
           _valid(m.j) &&
           _valid(m.k);
}
