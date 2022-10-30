#include "stdafx.h"

#include "render.h"
#include "IGame_Persistent.h"
#include "..\include\xrRender\RenderFactory.h"

#include "..\TSMP3_Build_Config.h"

void CRenderDevice::_Destroy(BOOL bKeepTextures)
{
	DU.OnDeviceDestroy();

	// before destroy
	b_is_Ready = FALSE;
	Statistic->OnDeviceDestroy();
	::Render->destroy();
	m_pRender->OnDeviceDestroy(bKeepTextures);

	Memory.mem_compact();
}

void CRenderDevice::Destroy(void)
{
	if (!b_is_Ready)
		return;

	Log("Destroying Direct3D...");

	ShowCursor(TRUE);
	m_pRender->ValidateHW();

	_Destroy(FALSE);

	// real destroy
	m_pRender->DestroyHW();

	seqRender.Clear();
	seqAppActivate.Clear();
	seqAppDeactivate.Clear();
	seqAppStart.Clear();
	seqAppEnd.Clear();
	seqFrame.Clear();
	seqFrameMT.Clear();
	seqDeviceReset.Clear();
	seqParallel.clear();

	RenderFactory->DestroyRenderDeviceRender(m_pRender);
	m_pRender = 0;
	xr_delete(Statistic);
}

#include "IGame_Level.h"
#include "CustomHUD.h"

void CRenderDevice::Reset(bool precache)
{
	bool b_16_before = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);

	ShowCursor(TRUE);
	u32 tm_start = TimerAsync();

	m_pRender->Reset(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

	if (g_pGamePersistent)
		g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
	
	_SetupStates();
	if (precache)
		PreCache(20);
	u32 tm_end = TimerAsync();
	Msg("*** RESET [%d ms]", tm_end - tm_start);

#ifndef DEDICATED_SERVER
	ShowCursor(FALSE);
#endif

	seqDeviceReset.Process(rp_DeviceReset);

	bool b_16_after = (float)dwWidth / (float)dwHeight > (1024.0f / 768.0f + 0.01f);
	if (b_16_after != b_16_before && g_pGameLevel && g_pGameLevel->pHUD)
		g_pGameLevel->pHUD->OnScreenRatioChanged();	
}
