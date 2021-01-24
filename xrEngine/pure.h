#pragma once

// Приоритеты для задания нужного порядка вызовов функций
#define REG_PRIORITY_LOW 0x11111111ul
#define REG_PRIORITY_NORMAL 0x22222222ul
#define REG_PRIORITY_HIGH 0x33333333ul
#define REG_PRIORITY_INVALID 0xfffffffful

typedef void __fastcall RP_FUNC(void *obj);

#define DECLARE_MESSAGE(name)            \
	extern ENGINE_API RP_FUNC rp_##name; \
	class ENGINE_API pure##name          \
	{                                    \
	public:                              \
		virtual void On##name(void) = 0; \
	}

DECLARE_MESSAGE(Frame);
DECLARE_MESSAGE(Render);
DECLARE_MESSAGE(AppActivate);
DECLARE_MESSAGE(AppDeactivate);
DECLARE_MESSAGE(AppStart);
DECLARE_MESSAGE(AppEnd);
DECLARE_MESSAGE(DeviceReset);

template <class ClientType>
class CRegistrator
{
	struct
	{
		u32 in_process : 1;
		u32 changed : 1;
	};

	struct RegistratorClient
	{
		void* Object;
		int Priority;
	};

	xr_vector<RegistratorClient> m_Subscribers;

public:

	CRegistrator()
	{
		in_process = false;
		changed = false;
	}

	// Добавить нового подписавшегося на событие
	void Add(ClientType *obj, int priority = REG_PRIORITY_NORMAL)
	{
#ifdef DEBUG
		VERIFY(priority != REG_PRIORITY_INVALID);
		VERIFY(obj);
		for (u32 i = 0; i < m_Subscribers.size(); i++)
			VERIFY(!((m_Subscribers[i].Priority != REG_PRIORITY_INVALID) && (m_Subscribers[i].Object == (void *)obj)));
#endif

		RegistratorClient newObject;
		newObject.Object = obj;
		newObject.Priority = priority;
		m_Subscribers.push_back(newObject);

		if (in_process)
			changed = true;
		else
			Resort();
	}

	// Убрать конкретного подписавшегося на событие
	void Remove(ClientType *obj)
	{
		for (u32 i = 0; i < m_Subscribers.size(); i++)
		{
			if (m_Subscribers[i].Object == obj)
				m_Subscribers[i].Priority = REG_PRIORITY_INVALID;
		}

		if (in_process)
			changed = true;
		else
			Resort();
	}

	// Вызвать функцию у всех подписавшихся
	void Process(RP_FUNC *f)
	{
		in_process = true;

		if (m_Subscribers.empty())
			return;

		for (u32 i = 0; i < m_Subscribers.size(); i++)
			if (m_Subscribers[i].Priority != REG_PRIORITY_INVALID)
				f(m_Subscribers[i].Object);

		if (changed)
			Resort();

		in_process = false;
	}

	// Пересортировать по приоритетам
	void Resort(void)
	{		
		sort(m_Subscribers.begin(), m_Subscribers.end(),
			[](const RegistratorClient &reg1, const RegistratorClient &reg2) -> bool
			{
				return reg1.Priority > reg2.Priority;
			});

		while ((m_Subscribers.size()) && (m_Subscribers[m_Subscribers.size() - 1].Priority == REG_PRIORITY_INVALID))
			m_Subscribers.pop_back();

		if (m_Subscribers.empty())
			m_Subscribers.clear();

		changed = false;
	}

	// Очистить список подписавшихся
	void Clear()
	{
		m_Subscribers.clear();
	}
};
