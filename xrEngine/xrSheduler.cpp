#include "stdafx.h"
#include "xrSheduler.h"
#include "xr_object.h"

//#define DEBUG_SCHEDULER

float psShedulerCurrent = 10.f;
float psShedulerTarget = 10.f;
const float psShedulerReaction = 0.1f;
BOOL g_bSheduleInProgress = FALSE;

void CSheduler::Initialize()
{
	m_processing_now = false;
}

void CSheduler::Destroy()
{
	internal_ProcessRegistration();

#ifdef DEBUG
	for (u32 it = 0; it < Items.size(); it++)
	{
		if (!Items[it].Object)
		{
			Items.erase(Items.begin() + it);
			it--;
		}
	}

	if (!Items.empty())
	{
		string1024 _objects;
		_objects[0] = 0;

		Msg("! Sheduler work-list is not empty");

		for (u32 it = 0; it < Items.size(); it++)
			Msg("%s", *Items[it].Object->shedule_Name().c_str());
	}
#endif // DEBUG

	ItemsRT.clear();
	Items.clear();
	ItemsProcessed.clear();
	m_RegistrationVector.clear();
}

void CSheduler::Register(ISheduled* A, bool realtime)
{
	DEBUG_VERIFY(!Registered(A));

	RegistratorItem R;
	R.unregister = false;
	R.realtime = realtime;
	R.objectPtr = A;
	R.objectPtr->shedule.b_RT = realtime;

#ifdef DEBUG_SCHEDULER
	Msg("SCHEDULER: register [%s][%x]", *A->shedule_Name(), A);
#endif // DEBUG_SCHEDULER

	m_RegistrationVector.push_back(R);
}

void CSheduler::Unregister(ISheduled* A)
{
	DEBUG_VERIFY(Registered(A));

#ifdef DEBUG_SCHEDULER
	Msg("SCHEDULER: unregister [%s][%x]", *A->shedule_Name(), A);
#endif // DEBUG_SCHEDULER

	if (m_processing_now)
	{
		if (internal_Unregister(A, A->shedule.b_RT, false))
			return;
	}

	RegistratorItem R;
	R.unregister = true;
	R.realtime = A->shedule.b_RT;
	R.objectPtr = A;

	m_RegistrationVector.push_back(R);
}

void CSheduler::internal_ProcessRegistration()
{
	for (u32 it = 0; it < m_RegistrationVector.size(); it++)
	{
		RegistratorItem& item = m_RegistrationVector[it];
		
		if (!item.unregister)
		{
			// ищем пару на удаление. нельзя удалять этот код
			// иначе будет ломаться на самом первом апдейте
			bool foundUnregisterPair = false;

			for (u32 pairIt = it + 1; pairIt < m_RegistrationVector.size(); pairIt++)
			{
				RegistratorItem& itemPair = m_RegistrationVector[pairIt];

				if (itemPair.unregister && itemPair.objectPtr == item.objectPtr)
				{
					foundUnregisterPair = true;
					m_RegistrationVector.erase(m_RegistrationVector.begin() + pairIt);
					break;
				}
			}

			if (!foundUnregisterPair)
				internal_Register(item.objectPtr, item.realtime);
		}
		else
			internal_Unregister(item.objectPtr, item.realtime);
	}

	m_RegistrationVector.clear();
}

void CSheduler::internal_Register(ISheduled *Obj, BOOL RT)
{
	VERIFY(!Obj->shedule.b_locked);

#ifdef DEBUG_SCHEDULER
	Msg("SCHEDULER: internal register [%s][%x][%s]", O->shedule_Name(), O, RT ? "true" : "false");
#endif // DEBUG_SCHEDULER

	// Fill item structure
	Item TNext;
	TNext.dwTimeForExecute = Device.dwTimeGlobal;
	TNext.dwTimeOfLastExecute = Device.dwTimeGlobal;
	TNext.Object = Obj;
	TNext.scheduled_name = Obj->shedule_Name();
	Obj->shedule.b_RT = RT;

	if (RT)
		ItemsRT.push_back(TNext);
	else
		Push(TNext);
}

bool CSheduler::internal_Unregister(ISheduled *Obj, BOOL RT, bool warn_on_not_found)
{
	auto &itemsVector = RT ? ItemsRT : Items;

	for (u32 i = 0; i < itemsVector.size(); i++)
	{
		if (itemsVector[i].Object == Obj)
		{
#ifdef DEBUG_SCHEDULER
			Msg("SCHEDULER: internal unregister [%s][%x][%s]", "unknown", Obj, "true");
#endif // DEBUG_SCHEDULER

			itemsVector.erase(itemsVector.begin() + i);
			return true;
		}
	}

#ifdef DEBUG_SCHEDULER
	if (warn_on_not_found)
		Msg("! scheduled object %s tries to unregister but is not registered", *Obj->shedule_Name());
#endif // DEBUG_SCHEDULER

	return false;
}

void CSheduler::ProcessStep()
{
	// Normal priority
	u32 dwTime = Device.dwTimeGlobal;
	CTimer eTimer;

	for (int i = 0; !Items.empty() && Top().dwTimeForExecute < dwTime; ++i)
	{
		Item T = Top();

#ifdef DEBUG_SCHEDULER
		Msg("SCHEDULER: process step [%s][%x][false]", *T.scheduled_name, T.Object);
#endif // DEBUG_SCHEDULER

		u32 Elapsed = dwTime - T.dwTimeOfLastExecute;

		if (!T.Object || !T.Object->shedule_Needed())
		{
			// Erase element
#ifdef DEBUG_SCHEDULER
			Msg("SCHEDULER: process unregister [%s][%x][%s]", *T.scheduled_name, T.Object, "false");
#endif // DEBUG_SCHEDULER

			Pop();
			continue;
		}

		// Insert into priority Queue
		Pop();

#ifdef DEBUG
		T.Object->dbg_startframe = Device.CurrentFrameNumber;
		eTimer.Start();
		LPCSTR _obj_name = T.Object->shedule_Name().c_str();
#endif // DEBUG

		// Calc next update interval
		u32 dwMin = _max(u32(30), T.Object->shedule.t_min);
		u32 dwMax = (1000 + T.Object->shedule.t_max) / 2;
		float scale = T.Object->shedule_Scale();
		u32 dwUpdate = dwMin + iFloor(float(dwMax - dwMin) * scale);
		clamp(dwUpdate, u32(_max(dwMin, u32(20))), dwMax);

		T.Object->shedule_Update(clampr(Elapsed, u32(1), u32(_max(u32(T.Object->shedule.t_max), u32(1000)))));

#ifdef DEBUG
		u32 execTime = eTimer.GetElapsed_ms();
#endif

		// Fill item structure
		Item TNext;
		TNext.dwTimeForExecute = dwTime + dwUpdate;
		TNext.dwTimeOfLastExecute = dwTime;
		TNext.Object = T.Object;
		TNext.scheduled_name = T.Object->shedule_Name();

		ItemsProcessed.push_back(TNext);

#ifdef DEBUG
		if (execTime > 15)
		{
			Msg("* xrSheduler: too much time consumed by object [%s] (%dms)", _obj_name, execTime);
		}
#endif // DEBUG

		if ((i % 3) != (3 - 1))
			continue;

		if (CPU::QPC() > cycles_limit)
		{
			// we have maxed out the load - increase heap
			psShedulerTarget += (psShedulerReaction * 3);
			break;
		}
	}

	// Push "processed" back
	while (!ItemsProcessed.empty())
	{
		Push(ItemsProcessed.back());
		ItemsProcessed.pop_back();
	}

	// always try to decrease target
	psShedulerTarget -= psShedulerReaction;
}

void CSheduler::Update()
{
	R_ASSERT(Device.Statistic);
	// Initialize
	Device.Statistic->Sheduler.Begin();
	cycles_start = CPU::QPC();
	cycles_limit = CPU::qpc_freq * u64(iCeil(psShedulerCurrent)) / 1000i64 + cycles_start;
	internal_ProcessRegistration();
	g_bSheduleInProgress = TRUE;

#ifdef DEBUG_SCHEDULER
	Msg("SCHEDULER: PROCESS STEP %d", Device.CurrentFrameNumber);
#endif // DEBUG_SCHEDULER

	// Realtime priority
	m_processing_now = true;
	u32 dwTime = Device.dwTimeGlobal;

	for (u32 it = 0; it < ItemsRT.size(); it++)
	{
		Item &T = ItemsRT[it];
		R_ASSERT(T.Object);

#ifdef DEBUG_SCHEDULER
		Msg("SCHEDULER: process step [%s][%x][true]", *T.Object->shedule_Name(), T.Object);
#endif // DEBUG_SCHEDULER

		if (!T.Object->shedule_Needed())
		{

#ifdef DEBUG_SCHEDULER
			Msg("SCHEDULER: process unregister [%s][%x][%s]", *T.Object->shedule_Name(), T.Object, "false");
#endif // DEBUG_SCHEDULER

			T.dwTimeOfLastExecute = dwTime;
			continue;
		}

		u32 Elapsed = dwTime - T.dwTimeOfLastExecute;

#ifdef DEBUG
		VERIFY(T.Object->dbg_startframe != Device.CurrentFrameNumber);
		T.Object->dbg_startframe = Device.CurrentFrameNumber;
#endif

		T.Object->shedule_Update(Elapsed);
		T.dwTimeOfLastExecute = dwTime;
	}

	// Normal (sheduled)
	ProcessStep();
	m_processing_now = false;

#ifdef DEBUG_SCHEDULER
	Msg("SCHEDULER: PROCESS STEP FINISHED %d", Device.CurrentFrameNumber);
#endif // DEBUG_SCHEDULER

	clamp(psShedulerTarget, 3.f, 66.f);
	psShedulerCurrent = 0.9f * psShedulerCurrent + 0.1f * psShedulerTarget;
	Device.Statistic->fShedulerLoad = psShedulerCurrent;

	// Finalize
	g_bSheduleInProgress = FALSE;
	internal_ProcessRegistration();
	Device.Statistic->Sheduler.End();
}

#ifdef DEBUG
bool CSheduler::Registered(ISheduled* object) const
{
	u32 foundCount = 0;

	for (const Item &it: ItemsRT)
	{
		if (it.Object == object)
		{
			foundCount++;
			break;
		}
	}

	for (const Item &it : Items)
	{
		if (it.Object == object)
		{
			foundCount++;
			break;
		}
	}

	for (const Item &it : ItemsProcessed)
	{
		if (it.Object == object)
		{
			VERIFY(!foundCount);
			foundCount++;
			break;
		}
	}

	for (const RegistratorItem &it : m_RegistrationVector)
	{
		if (it.objectPtr == object)
		{
			if (it.unregister)
			{
				VERIFY(foundCount == 1);
				--foundCount;
			}
			else
			{
				VERIFY(!foundCount);
				++foundCount;
			}
		}
	}

	VERIFY(!foundCount || (foundCount == 1));
	return (foundCount == 1);
}
#endif // DEBUG
