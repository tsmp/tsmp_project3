#pragma once
#include <atomic>	

namespace discord
{
	class Activity;
	class Core;
}

class ENGINE_API DiscordSDK final
{
	xr_string m_StatusDiscord;
	xr_string m_PhaseDiscord;

	std::atomic_bool m_NeedUpdateActivity;

	discord::Activity* m_ActivityDiscord = nullptr;
	discord::Core* m_DiscordCore = nullptr;

public:
	DiscordSDK() = default;
	~DiscordSDK();

	void InitSDK();
	void UpdateSDK();
	void UpdateActivity();

	void SetPhase(const xr_string& phase);
	void SetStatus(const xr_string& status);
};

extern ENGINE_API DiscordSDK Discord;
