#pragma once
class XRCORE_API xrThreadingEvent final
{
	HANDLE handle_event;

public:
	xrThreadingEvent(std::string_view event_name);
	~xrThreadingEvent();

	void Reset() const;
	void Set() const;
	void Wait() const;
	bool Wait(u32 millisecond) const;
};

