#pragma once
#include "../Include/xrRender/WallMarkArray.h"
#include "../Include/xrRender/RenderFactory.h"

inline constexpr u32 GAMEMTLPAIR_CHUNK_PAIR = 0x1000;
inline constexpr u32 GAMEMTLPAIR_CHUNK_BREAKING = 0x1002;
inline constexpr u32 GAMEMTLPAIR_CHUNK_STEP = 0x1003;
inline constexpr u32 GAMEMTLPAIR_CHUNK_COLLIDE = 0x1005;
inline constexpr u32 GAMEMTL_CHUNK_DENSITY = 0x1007;
inline constexpr u32 GAMEMTL_CHUNK_FACTORS_MP = 0x1008;

inline constexpr u32 GAMEMTL_NONE_ID = static_cast<u32>(-1);
inline constexpr u16 GAMEMTL_NONE_IDX = static_cast<u16>(-1);

using SoundVec = xr_vector<ref_sound>;
using PSVec = xr_vector<shared_str>;

struct ENGINE_API SGameMtl
{
    friend class CGameMtlLibrary;

protected:
    int ID; // auto number
public:
    enum
    {
        flBreakable = (1ul << 0ul),
        flBounceable = (1ul << 2ul),
        flSkidmark = (1ul << 3ul),
        flBloodmark = (1ul << 4ul),
        flClimable = (1ul << 5ul),
        flPassable = (1ul << 7ul),
        flDynamic = (1ul << 8ul),
        flLiquid = (1ul << 9ul),
        flSuppressShadows = (1ul << 10ul),
        flSuppressWallmarks = (1ul << 11ul),
        flActorObstacle = (1ul << 12ul),
        flNoRicoshet = (1ul << 13ul),

        flInjurious = (1ul << 28ul), // flInjurious = fInjuriousSpeed > 0.f
        flShootable = (1ul << 29ul),
        flTransparent = (1ul << 30ul),
        flSlowDown = (1ul << 31ul) // flSlowDown = (fFlotationFactor<1.f)
    };

public:
    shared_str m_Name;
    Flags32 Flags;

    // physics part
    float fPHFriction;
    float fPHDamping;
    float fPHSpring;
    float fPHBounceStartVelocity;
    float fPHBouncing;

    float fFlotationFactor;       // 0.f - 1.f (1.f-полностью проходимый)
    float fShootFactor;           // 0.f - 1.f (1.f-полностью простреливаемый)
    float fShootFactorMP;			// 0.f - 1.f(1.f-полностью простреливаемый)
    float fBounceDamageFactor;    // 0.f - 100.f
    float fInjuriousSpeed;        // 0.f - ... (0.f-не отбирает здоровье (скорость уменьшения здоровья))
    float fVisTransparencyFactor; // 0.f - 1.f (1.f-полностью прозрачный)
    float fDensityFactor;
public:

    SGameMtl()
    {
        ID = -1;
        m_Name = "unknown";
        Flags.zero();

        // factors
        fFlotationFactor = 1.f;
        fShootFactor = 0.f;
        fShootFactorMP = 0.f;
        fBounceDamageFactor = 1.f;
        fInjuriousSpeed = 0.f;
        fVisTransparencyFactor = 0.f;

        // physics
        fPHFriction = 1.f;
        fPHDamping = 1.f;
        fPHSpring = 1.f;
        fPHBounceStartVelocity = 0.f;
        fPHBouncing = 0.1f;
        fDensityFactor = 0.0f;
    }

    void Load(IReader &fs);
    IC int GetID() const { return ID; }
};

using GameMtlVec = xr_vector<SGameMtl>;
using GameMtlIt = GameMtlVec::iterator;

struct ENGINE_API SGameMtlPair
{
    friend class CGameMtlLibrary;

private:
    int m_Mtl0;
    int m_Mtl1;

public:
    // properties
    SoundVec BreakingSounds;
    SoundVec StepSounds;
    SoundVec CollideSounds;
    PSVec CollideParticles;
    FactoryPtr<IWallMarkArray> m_pCollideMarks;

public:
    SGameMtlPair()
    {
        m_Mtl0 = -1;
        m_Mtl1 = -1;
    }

    ~SGameMtlPair();

    IC int GetMtl0() const { return m_Mtl0; }
    IC int GetMtl1() const { return m_Mtl1; }

    IC void SetPair(int m0, int m1)
    {
        m_Mtl0 = m0;
        m_Mtl1 = m1;
    }

    IC bool IsPair(int m0, int m1) const { return !!((m_Mtl0 == m0 && m_Mtl1 == m1) || (m_Mtl0 == m1 && m_Mtl1 == m0)); }

    void Load(IReader &fs);

#ifdef DEBUG
    LPCSTR dbg_Name() const;
#endif
};

using GameMtlPairVec = xr_vector<SGameMtlPair>;

class ENGINE_API CGameMtlLibrary
{
    GameMtlVec m_Materials;

    // game part
    u32 m_MaterialCount;
    GameMtlPairVec m_MaterialPairs;

public:
    CGameMtlLibrary();
    ~CGameMtlLibrary() = default;

    IC void Unload()
    {
        m_MaterialCount = 0;
        m_MaterialPairs.clear();
        m_Materials.clear();
    }

    // material routine
    IC GameMtlIt GetMaterialIt(LPCSTR name)
    {
        return std::find_if(m_Materials.begin(), m_Materials.end(), [&name](const SGameMtl& mtl)
        {
            return !strcmpi(*mtl.m_Name, name);
        });
    }

    IC GameMtlIt GetMaterialItByID(int id)
    {
        return std::find_if(m_Materials.begin(), m_Materials.end(), [&id](const SGameMtl& mtl)
        {
            return mtl.ID == id;
        });
    }

    // game
    IC u16 GetMaterialIdx(int ID)
    {
        GameMtlIt it = GetMaterialItByID(ID);
        VERIFY(m_Materials.end() != it);
        return static_cast<u16>(it - m_Materials.begin());
    }

    IC u16 GetMaterialIdx(LPCSTR name)
    {
        GameMtlIt it = GetMaterialIt(name);
        VERIFY(m_Materials.end() != it);
        return static_cast<u16>(it - m_Materials.begin());
    }

    IC SGameMtl *GetMaterialByIdx(u16 idx)
    {
        VERIFY(idx < m_Materials.size());
        return &m_Materials[idx];
    }

    IC const GameMtlVec& Materials() { return m_Materials; }
    IC u32 CountMaterial() const { return m_Materials.size(); }

    // game
    IC SGameMtlPair *GetMaterialPair(u16 idx0, u16 idx1)
    {
        R_ASSERT((idx0 < m_MaterialCount) && (idx1 < m_MaterialCount));
        return &m_MaterialPairs[idx1 * m_MaterialCount + idx0];
    }

    void Load();

#ifdef DEBUG
    IC SGameMtl* GetMaterialByID(s32 id) { return GetMaterialByIdx(GetMaterialIdx(id)); }
#endif
};

inline ref_sound GET_RANDOM(const SoundVec &sndVec)
{
    return sndVec[Random.randI(sndVec.size())];
}

inline void CLONE_MTL_SOUND(ref_sound &resSnd, const SGameMtlPair* mtlPair)
{
    VERIFY2(!mtlPair->CollideSounds.empty(), mtlPair->dbg_Name());
    resSnd.clone(GET_RANDOM(mtlPair->CollideSounds), st_Effect, sg_SourceType);
}

extern ENGINE_API CGameMtlLibrary GMLib;
