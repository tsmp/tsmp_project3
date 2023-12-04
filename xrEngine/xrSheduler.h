#pragma once

#include "ISheduled.h"

// Планировщик обновления игровых объектов
class ENGINE_API CSheduler
{
public:	
	void Update();

	void Register(ISheduled* A, bool realtime = false);
	void Unregister(ISheduled* A);

	void Initialize();
	void Destroy();

private:

	struct Item
	{
		u32 dwTimeForExecute;
		u32 dwTimeOfLastExecute;
		shared_str scheduled_name;
		ISheduled *Object;
		u32 dwPadding; // for align-issues

		IC bool operator<(const Item &I)
		{
			return dwTimeForExecute > I.dwTimeForExecute;
		}
	};

	xr_vector<Item> ItemsRT;
	xr_vector<Item> Items;
	xr_vector<Item> ItemsProcessed;

	u64 cycles_start;
	u64 cycles_limit;
	
	bool m_processing_now;

#ifdef DEBUG
	friend class ISheduled;
	bool Registered(ISheduled* object) const;	
#endif // DEBUG

	void ProcessStep();

	// Регистрация объектов в планировщике
	struct RegistratorItem
	{
		ISheduled* objectPtr;
		bool unregister;
		bool realtime;
	};

	xr_vector<RegistratorItem> m_RegistrationVector;

	void internal_Register(ISheduled* A, BOOL RT = FALSE);
	bool internal_Unregister(ISheduled* A, BOOL RT, bool warn_on_not_found = true);
	void internal_ProcessRegistration();

	// Очередь
	IC void Push(Item& I)
	{
		Items.push_back(I);
		push_heap(Items.begin(), Items.end());
	}

	IC void Pop()
	{
		pop_heap(Items.begin(), Items.end());
		Items.pop_back();
	}

	IC Item& Top() 
	{ 
		return Items.front(); 
	}
};
