#pragma once

#ifdef NEW_RANKS
enum
{	
	ID_RANK_0 = 101,
	ID_RANK_1,
	ID_RANK_2,
	ID_RANK_3,
	ID_RANK_4,
	ID_RANK_5,
	ID_RANK_6,
	ID_RANK_7,

	ID_DM_forcedword = u32(-1)
};
#else
enum
{
	ID_RANK_0 = 101,
	ID_RANK_1,
	ID_RANK_2,
	ID_RANK_3,
	ID_RANK_4,

	ID_DM_forcedword = u32(-1)
};
#endif