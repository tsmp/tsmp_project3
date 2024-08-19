#include "stdafx.h"
#include "xrThreadingEvent.h"

xrThreadingEvent::xrThreadingEvent(std::string_view event_name)
{
	handle_event = CreateEvent(nullptr, false, false, event_name.data());
}

xrThreadingEvent::~xrThreadingEvent()
{
	CloseHandle(handle_event);
}

void xrThreadingEvent::Reset() const
{
	ResetEvent(handle_event);
}

void xrThreadingEvent::Set() const
{
	SetEvent(handle_event);
}

void xrThreadingEvent::Wait() const
{
	WaitForSingleObject(handle_event, INFINITE);
}

bool xrThreadingEvent::Wait(u32 millisecond) const
{
	return WaitForSingleObject(handle_event, millisecond) != WAIT_TIMEOUT
}