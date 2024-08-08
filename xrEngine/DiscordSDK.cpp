#include "stdafx.h"
#include <discord.h>
#include "DiscordSDK.h"

#pragma comment(lib, "discord_game_sdk.dll.lib")

ENGINE_API DiscordSDK Discord;

DiscordSDK::~DiscordSDK()
{
	delete m_DiscordCore;
	delete m_ActivityDiscord;
}

void DiscordSDK::InitSDK()
{
	auto resultSDK = discord::Core::Create(m_AppID, DiscordCreateFlags_NoRequireDiscord, &m_DiscordCore);
	
	if (!m_DiscordCore)
	{
		Msg("! [DISCORD SDK]: Error to init SDK. Code error: [%d]", static_cast<int>(resultSDK));
		return;
	}

	m_ActivityDiscord = new discord::Activity();
	m_ActivityDiscord->GetAssets().SetLargeImage("main_image");
	m_ActivityDiscord->GetAssets().SetSmallImage("main_image_small");

	m_ActivityDiscord->SetInstance(true);
	m_ActivityDiscord->SetType(discord::ActivityType::Playing);
	m_ActivityDiscord->GetTimestamps().SetStart(time(nullptr));

	m_NeedUpdateActivity = true;
}

void DiscordSDK::UpdateSDK()
{
	if (!m_DiscordCore)
		return;

	if (m_NeedUpdateActivity)
		UpdateActivity();

	m_DiscordCore->RunCallbacks();
}

void DiscordSDK::UpdateActivity()
{
	m_ActivityDiscord->SetState(ANSIToUTF8(m_PhaseDiscord).c_str());
	m_ActivityDiscord->SetDetails(ANSIToUTF8(m_StatusDiscord).c_str());

	m_DiscordCore->ActivityManager().UpdateActivity(*m_ActivityDiscord, [](discord::Result result)
	{
		if (result != discord::Result::Ok)
			Msg("! [DISCORD SDK]: Invalid UpdateActivity");
	});

	m_NeedUpdateActivity = false;
}

void DiscordSDK::SetPhase(const xr_string& phase)
{
	m_PhaseDiscord = phase;
	m_NeedUpdateActivity = true;
}

void DiscordSDK::SetStatus(const xr_string& status)
{
	m_StatusDiscord = status;
	m_NeedUpdateActivity = true;
}
