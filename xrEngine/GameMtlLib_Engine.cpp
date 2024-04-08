#include "stdafx.h"
#include "GameMtlLib.h"

static const int GAMEMTL_SUBITEM_COUNT = 4;

void DestroySounds(SoundVec &lst)
{
	for (ref_sound &snd: lst)
		snd.destroy();
}

void CreateSounds(SoundVec &lst, LPCSTR buf)
{
	string128 tmp;
	int cnt = _GetItemCount(buf);
	R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);
	lst.resize(cnt);

	for (int k = 0; k < cnt; ++k)
		lst[k].create(_GetItem(buf, k, tmp), st_Effect, sg_SourceType);
}

void CreateMarks(IWallMarkArray* pMarks, LPCSTR buf)
{
	string256 tmp;
	int cnt = _GetItemCount(buf);
	R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);

	for (int k = 0; k < cnt; ++k)
		pMarks->AppendMark(_GetItem(buf, k, tmp));
}

void CreatePSs(PSVec &lst, LPCSTR buf)
{
	string256 tmp;
	int cnt = _GetItemCount(buf);
	R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);

	for (int k = 0; k < cnt; ++k)
		lst.push_back(_GetItem(buf, k, tmp));
}

SGameMtlPair::~SGameMtlPair()
{
	// destroy all media
	DestroySounds(BreakingSounds);
	DestroySounds(StepSounds);
	DestroySounds(CollideSounds);
}

void SGameMtlPair::Load(IReader &fs)
{
	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_PAIR));
	m_Mtl0 = fs.r_u32();
	m_Mtl1 = fs.r_u32();

	shared_str buf;
	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_BREAKING));
	fs.r_stringZ(buf);
	CreateSounds(BreakingSounds, *buf);

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_STEP));
	fs.r_stringZ(buf);
	CreateSounds(StepSounds, *buf);

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_COLLIDE));
	fs.r_stringZ(buf);
	CreateSounds(CollideSounds, *buf);

	fs.r_stringZ(buf);
	CreatePSs(CollideParticles, *buf);

	fs.r_stringZ(buf);
	CreateMarks(&*m_pCollideMarks, *buf);
}
