#pragma once

#include "../TSMP3_Build_Config.h"

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
	ID_RANK_8,
	ID_RANK_9,
	ID_RANK_10,
	ID_RANK_11,
	ID_RANK_12,
	ID_RANK_13,
	ID_RANK_14,
	ID_RANK_15,
	ID_RANK_16,
	ID_RANK_17,
	ID_RANK_18,
	ID_RANK_19,
	ID_RANK_20,
	ID_RANK_21,
	ID_RANK_22,
	ID_RANK_23,
	ID_RANK_24,
	ID_RANK_25,
	ID_RANK_26,
	ID_RANK_27,
	ID_RANK_28,
	ID_RANK_29,
	ID_RANK_30,
	ID_RANK_31,

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