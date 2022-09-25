//  ритическа€ секци€ - класс дл€ защиты общих данных 
// от параллельного изменени€ разными потоками

#include "pch_xrcore.h"
#include "xrCriticalSection.h"

#pragma TODO(TSMP: test profile critical sections)

#ifdef PROFILE_CRITICAL_SECTIONS
static add_profile_portion_callback add_profile_portion = 0;

void set_add_profile_portion(add_profile_portion_callback callback)
{
	add_profile_portion = callback;
}

struct profiler
{
	u64 m_time;
	LPCSTR m_timer_id;

	IC profiler::profiler(LPCSTR timer_id)
	{
		if (!add_profile_portion)
			return;

		m_timer_id = timer_id;
		m_time = CPU::QPC();
	}

	IC profiler::~profiler()
	{
		if (!add_profile_portion)
			return;

		u64 time = CPU::QPC();
		(*add_profile_portion)(m_timer_id, time - m_time);
	}
};
#endif // PROFILE_CRITICAL_SECTIONS

#ifdef PROFILE_CRITICAL_SECTIONS
xrCriticalSection::xrCriticalSection(LPCSTR id) : m_szId(id)
#else  // PROFILE_CRITICAL_SECTIONS
xrCriticalSection::xrCriticalSection()
#endif // PROFILE_CRITICAL_SECTIONS
{
	m_pCritSection = xr_alloc<CRITICAL_SECTION>(1);
	InitializeCriticalSection(m_pCritSection);
}

xrCriticalSection::~xrCriticalSection()
{
	DeleteCriticalSection(m_pCritSection);
	xr_free(m_pCritSection);
}

#ifdef DEBUG
extern void OutputDebugStackTrace(const char *header);
#endif // DEBUG

void xrCriticalSection::Enter()
{
#ifdef PROFILE_CRITICAL_SECTIONS

#if 0  //def DEBUG
		static bool					show_call_stack = false;
		if (show_call_stack)
			OutputDebugStackTrace	("----------------------------------------------------");
#endif // DEBUG

	profiler temp(m_szId);
#endif // PROFILE_CRITICAL_SECTIONS

	EnterCriticalSection(m_pCritSection);
}

void xrCriticalSection::Leave()
{
	LeaveCriticalSection(m_pCritSection);
}

bool xrCriticalSection::TryEnter()
{
	return !!TryEnterCriticalSection(m_pCritSection);
}
