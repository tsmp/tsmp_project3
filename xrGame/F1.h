#pragma once

#include "grenade.h"
#include "script_export_space.h"

class CF1 : public CGrenade
{
	typedef CGrenade inherited;

public:
	CF1() = default;
	virtual ~CF1() = default;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CF1)
#undef script_type_list
#define script_type_list save_type_list(CF1)
