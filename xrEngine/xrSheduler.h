#pragma once

#include "ISheduled.h"

class ENGINE_API CSheduler
{
public:
	void ProcessStep();
	void Update();

	void Register(ISheduled* A, bool realtime = false);
	void Unregister(ISheduled* A);
	void EnsureOrder(ISheduled* Before, ISheduled* After);

	void Initialize();
	void Destroy();

#ifdef DEBUG
	bool Registered(ISheduled* object) const;
#endif // DEBUG

private:
	struct Item
	{
		u32 dwTimeForExecute;
		u32 dwTimeOfLastExecute;
		shared_str scheduled_name;
		ISheduled *Object;
		u32 dwPadding; // for align-issues

		IC bool operator<(Item &I)
		{
			return dwTimeForExecute > I.dwTimeForExecute;
		}
	};

	struct ItemReg
	{
		ISheduled *Object;
		bool OP;
		bool realtime;
	};

	xr_vector<Item> ItemsRT;
	xr_vector<Item> Items;
	xr_vector<Item> ItemsProcessed;
	xr_vector<ItemReg> Registration;
	bool m_processing_now;

	IC void Push(Item &I);
	IC void Pop();
	IC Item &Top() { return Items.front(); }

	void internal_Register(ISheduled *A, BOOL RT = FALSE);
	bool internal_Unregister(ISheduled *A, BOOL RT, bool warn_on_not_found = true);
	void internal_Registration();

	u64 cycles_start;
	u64 cycles_limit;
};
