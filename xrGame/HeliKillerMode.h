#pragma once
// ’з как назвать, назвал Killer)

class xrClientData;

class HeliKillerMode
{
	bool m_Enabled = false;
	bool m_Finished = false;
	bool m_FirstEnemy = true;
	u32 m_EnemyTeam = 0;
	u32 m_GetEnemyCounter = 0;
	u32 m_EnabledTime = 0;
	u16 m_HeliId = 0;

public:
	static void SpawnHeli(xrClientData* caller);

	void SetHeliId(u16 id);
	void SetEnemyTeam(u32 team);
	bool Enabled() const;
	bool Finished();
	u16 GetEnemyId();
	void KillHeli();
};
