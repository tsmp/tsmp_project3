#include "stdafx.h"
#include "GameMtlLib.h"

static const u16 GAMEMTL_CURRENT_VERSION = 0x0001;
static const char* GAMEMTL_FILENAME = "gamemtl.xr";

static const u32 GAMEMTLS_CHUNK_VERSION = 0x1000;
static const u32 GAMEMTLS_CHUNK_MTLS = 0x1002;
static const u32 GAMEMTLS_CHUNK_MTLS_PAIR = 0x1003;

static const u32 GAMEMTL_CHUNK_MAIN = 0x1000;
static const u32 GAMEMTL_CHUNK_FLAGS = 0x1001;
static const u32 GAMEMTL_CHUNK_PHYSICS = 0x1002;
static const u32 GAMEMTL_CHUNK_FACTORS = 0x1003;
static const u32 GAMEMTL_CHUNK_FLOTATION = 0x1004;
static const u32 GAMEMTL_CHUNK_INJURIOUS = 0x1006;

CGameMtlLibrary GMLib;

CGameMtlLibrary::CGameMtlLibrary()
{
    m_MaterialCount = 0;
}

void SGameMtl::Load(IReader &fs)
{
    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_MAIN));
    ID = fs.r_u32();
    fs.r_stringZ(m_Name);

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FLAGS));
    Flags.assign(fs.r_u32());

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_PHYSICS));
    fPHFriction = fs.r_float();
    fPHDamping = fs.r_float();
    fPHSpring = fs.r_float();
    fPHBounceStartVelocity = fs.r_float();
    fPHBouncing = fs.r_float();

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FACTORS));
    fShootFactor = fs.r_float();
    fBounceDamageFactor = fs.r_float();
    fVisTransparencyFactor = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_FACTORS_MP))
        fShootFactorMP = fs.r_float();
    else
        fShootFactorMP = fShootFactor;

    if (fs.find_chunk(GAMEMTL_CHUNK_FLOTATION))
        fFlotationFactor = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_INJURIOUS))
        fInjuriousSpeed = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_DENSITY))
        fDensityFactor = fs.r_float();
}

void CGameMtlLibrary::Load()
{
    string_path name;

    if (!FS.exist(name, _game_data_, GAMEMTL_FILENAME))
    {
        Log("! Can't find game material file: ", name);
        return;
    }

    R_ASSERT(m_Materials.empty());

    IReader *F = FS.r_open(name);
    IReader &fs = *F;

    R_ASSERT(fs.find_chunk(GAMEMTLS_CHUNK_VERSION));
    u16 version = fs.r_u16();

    if (GAMEMTL_CURRENT_VERSION != version)
    {
        Log("CGameMtlLibrary: invalid version. Library can't load.");
        FS.r_close(F);
        return;
    }

    m_Materials.clear();

    if (IReader* OBJ = fs.open_chunk(GAMEMTLS_CHUNK_MTLS))
    {
        u32 count;
        for (IReader *O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count, O))
        {
            SGameMtl &M = m_Materials.emplace_back();
            M.Load(*O);
        }
        OBJ->close();
    }

    GameMtlPairVec material_pairs;

    if (IReader* OBJ = fs.open_chunk(GAMEMTLS_CHUNK_MTLS_PAIR))
    {
        u32 count;
        for (IReader *O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count, O))
        {
            SGameMtlPair &M = material_pairs.emplace_back();
            M.Load(*O);           
        }
        OBJ->close();
    }

    m_MaterialCount = static_cast<u32>(m_Materials.size());
    m_MaterialPairs.resize(m_MaterialCount * m_MaterialCount);

    for (SGameMtlPair& S : material_pairs)
    {
        const u16 idxMtl0 = GetMaterialIdx(S.m_Mtl0);
        const u16 idxMtl1 = GetMaterialIdx(S.m_Mtl1);

        const int idx0 = idxMtl0 * m_MaterialCount + idxMtl1;
        const int idx1 = idxMtl1 * m_MaterialCount + idxMtl0;

        m_MaterialPairs[idx0] = S;
        m_MaterialPairs[idx1] = S;
    }

    FS.r_close(F);
}

#ifdef DEBUG
LPCSTR SGameMtlPair::dbg_Name() const
{
    static string256 nm;
    SGameMtl *M0 = GMLib.GetMaterialByID(GetMtl0());
    SGameMtl *M1 = GMLib.GetMaterialByID(GetMtl1());
    sprintf_s(nm, "Pair: %s - %s", *M0->m_Name, *M1->m_Name);
    return nm;
}
#endif
