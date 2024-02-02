#include "stdafx.h"
#include "frustum.h"
#include "xr_input.h"

#pragma TODO("TSMP: port device files from from cs if they are better")

#pragma warning(disable : 4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>
// d3dx9.h
#include <d3dx9.h>
#pragma warning(default : 4995)

#include "xrApplication.h"
#include "render.h"
#include "IGame_Persistent.h"

#include "..\TSMP3_Build_Config.h"

ENGINE_API CRenderDevice Device;
ENGINE_API BOOL g_bRendering = FALSE;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;

BOOL CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
	switch (m_pRender->GetDeviceState())
	{
	case IRenderDeviceRender::dsOK:
		break;

	case IRenderDeviceRender::dsLost:
		// If the device was lost, do not render until we get it back
		Sleep(33);
		return FALSE;
		break;

	case IRenderDeviceRender::dsNeedReset:
		// Check if the device is ready to be reset
		Reset();
		break;

	default:
		R_ASSERT(0);
	}

	m_pRender->Begin();
	FPU::m24r();
	g_bRendering = TRUE;
#endif
	return TRUE;
}

void CRenderDevice::Clear()
{
	m_pRender->Clear();
}


void CRenderDevice::End(void)
{
	if (dwPrecacheFrame)
	{
		::Sound->set_master_volume(0.f);
		dwPrecacheFrame--;
		pApp->load_draw_internal();

		if (!dwPrecacheFrame)
		{
			m_pRender->updateGamma();

			if (precache_light)
				precache_light->set_active(false);

			if (precache_light)
				precache_light.destroy();

			::Sound->set_master_volume(1.f);
			pApp->destroy_loading_shaders();

			m_pRender->ResourcesDestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
		}
	}

	g_bRendering = FALSE;
	// end scene

	m_pRender->End();
}

// ����������� ������ �� � �������� �� �� ������ ������
ICF void ProcessTasksForMT()
{
	for (u32 pit = 0; pit < Device.seqParallel.size(); pit++)
		Device.seqParallel[pit]();
	Device.seqParallel.clear_not_free();
	Device.seqFrameMT.Process(rp_Frame);
}

volatile u32 mt_Thread_marker = 0x12345678;

void mt_Thread(void *ptr)
{
	while (true)
	{
		// waiting for Device permission to execute
		Device.mt_csEnter.Enter();

		if (Device.mt_bMustExit)
		{
			Device.mt_bMustExit = FALSE; // Important!!!
			Device.mt_csEnter.Leave();	 // Important!!!
			return;
		}
		// we has granted permission to execute
		mt_Thread_marker = Device.CurrentFrameNumber;

		ProcessTasksForMT();

		// now we give control to device - signals that we are ended our work
		Device.mt_csEnter.Leave();
		// waits for device signal to continue - to start again
		Device.mt_csLeave.Enter();
		// returns sync signal to device
		Device.mt_csLeave.Leave();
	}
}

#include "igame_level.h"
void CRenderDevice::PreCache(u32 amount)
{
	if (m_pRender->GetForceGPU_REF())
		amount = 0;
#ifdef DEDICATED_SERVER
	amount = 0;
#endif
	// Msg			("* PCACHE: start for %d...",amount);
	dwPrecacheFrame = dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel) 
	{
		precache_light = ::Render->light_create();
		precache_light->set_shadow(false);
		precache_light->set_position(vCameraPosition);
		precache_light->set_color(255, 255, 255);
		precache_light->set_range(5.0f);
		precache_light->set_active(true);
	}
}

int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

void CRenderDevice::ProcessRender()
{
	Statistic->RenderTOTAL_Real.FrameStart();
	Statistic->RenderTOTAL_Real.Begin();

	if (b_is_Active)
	{
		if (Begin())
		{
			seqRender.Process(rp_Render);
			if (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || Statistic->errors.size())
				Statistic->Show();
			End();
		}
	}
	Statistic->RenderTOTAL_Real.End();
	Statistic->RenderTOTAL_Real.FrameEnd();
	Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;
}

void CRenderDevice::PrepareToRun()
{
	//	DUMP_PHASE;
	g_bLoaded = FALSE;

	Log("Starting engine...");
	thread_name("X-RAY Primary thread");

	// Startup timers and calculate timer delta
	dwTimeGlobal = 0;

	// ��������� ����� (����� � ������� ������� �������)
	u32 time_mm = timeGetTime();

	while (timeGetTime() == time_mm)
		; // wait for next tick

	u32 time_system = timeGetTime();
	u32 time_local = TimerAsync();
	m_SystemLocalTimersDelta = time_system - time_local;

#ifndef DEDICATED_SERVER
	// Start all threads
	mt_csEnter.Enter();
	mt_bMustExit = FALSE;
	thread_spawn(mt_Thread, "X-RAY Secondary thread", 0, 0);
#endif
}

void CRenderDevice::Run()
{
	PrepareToRun();

	// Message cycle
	MSG msg;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	seqAppStart.Process(rp_AppStart);
	m_pRender->ClearTarget();

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) // has message
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		if (!b_is_Ready)
		{
			Sleep(100);
			continue;
		}

		u32 frameStartTime = m_GlobalTimer.GetElapsed_ms();

		g_bEnableStatGather = psDeviceFlags.test(rsStatistic);

		if (g_loading_events.size())
		{
			if (g_loading_events.front()())
				g_loading_events.pop_front();

			pApp->LoadDraw();
			continue;
		}
		else
			FrameMove();

#ifndef DEDICATED_SERVER
		// Precache
		if (dwPrecacheFrame)
		{
			float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
			float angle = PI_MUL_2 * factor;
			vCameraDirection.set(_sin(angle), 0, _cos(angle));
			vCameraDirection.normalize();
			vCameraTop.set(0, 1, 0);
			vCameraRight.crossproduct(vCameraTop, vCameraDirection);

			mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
		}

		// Matrices
		mFullTransform.mul(mProject, mView);
		m_pRender->SetCacheXform(mView, mProject);
		D3DXMatrixInverse((D3DXMATRIX*)&mInvFullTransform, 0, (D3DXMATRIX*)&mFullTransform);

		// *** Resume threads
		// Capture end point - thread must run only ONE cycle
		// Release start point - allow thread to run
		mt_csLeave.Enter();
		mt_csEnter.Leave();
		Sleep(0);

		ProcessRender();

		// *** Suspend threads
		// Capture startup point
		// Release end point - allow thread to wait for startup point
		mt_csEnter.Enter();
		mt_csLeave.Leave();

		// Ensure, that second thread gets chance to execute anyway
		if (CurrentFrameNumber != mt_Thread_marker)
			ProcessTasksForMT();

		u32 frameEndTime = m_GlobalTimer.GetElapsed_ms();
		u32 frameTime = frameEndTime - frameStartTime;   

		if (Device.Paused() || g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive())
		{
			u32 UpdateDelta_ = 16; //16 ms = ~60 FPS

			if (frameTime < UpdateDelta_)
				Sleep(UpdateDelta_ - frameTime);
		}

		if (!b_is_Active)
			Sleep(1);

#else // DEDICATED_SERVER
		Device.seqFrameMT.Process(rp_Frame);
		u32 frameEndTime = m_GlobalTimer.GetElapsed_ms();
		u32 frameTime = frameEndTime - frameStartTime;

		u32 dedicatedSrvUpdateDelta = 1000 / g_svDedicateServerUpdateReate;

		if (frameTime < dedicatedSrvUpdateDelta)
			Sleep(dedicatedSrvUpdateDelta - frameTime);
#endif // DEDICATED_SERVER
	}

	seqAppEnd.Process(rp_AppEnd);

#ifndef DEDICATED_SERVER
	// Stop Balance-Thread
	mt_bMustExit = TRUE;
	mt_csEnter.Leave();

	while (mt_bMustExit)
		Sleep(0);
#endif
}

void ProcessLoading(RP_FUNC* f)
{
	Device.seqFrame.Process(rp_Frame);
	g_bLoaded = TRUE;
}

void CRenderDevice::FrameMove()
{
	CurrentFrameNumber++;

	dwTimeContinual = TimerMM.GetElapsed_ms();
	if (psDeviceFlags.test(rsConstantFPS))
	{
		// 20ms = 50fps
		fTimeDelta = 0.020f;
		fTimeGlobal += 0.020f;
		dwTimeDelta = 20;
		dwTimeGlobal += 20;
	}
	else
	{
		// Timer
		float fPreviousFrameTime = m_FrameTimer.GetElapsed_sec();
		m_FrameTimer.Start();												// previous frame
		fTimeDelta = 0.1f * fTimeDelta + 0.9f * fPreviousFrameTime; // smooth random system activity - worst case ~7% error
		
		if (fTimeDelta > .1f)
			fTimeDelta = .1f; // limit to 15fps minimum

		if (Paused())
			fTimeDelta = 0.0f;

		//		u64	qTime		= m_GlobalTimer.GetElapsed_clk();
		fTimeGlobal = m_GlobalTimer.GetElapsed_sec(); //float(qTime)*CPU::cycles2seconds;
		u32 _old_global = dwTimeGlobal;
		dwTimeGlobal = m_GlobalTimer.GetElapsed_ms(); //u32((qTime*u64(1000))/CPU::cycles_per_second);
		dwTimeDelta = dwTimeGlobal - _old_global;
	}

	// Frame move
	Statistic->EngineTOTAL.Begin();

	if (!g_bLoaded)
		ProcessLoading(rp_Frame);
	else
		seqFrame.Process(rp_Frame);

	Statistic->EngineTOTAL.End();
}

ENGINE_API BOOL bShowPauseString = TRUE;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	static int snd_emitters_ = -1;

#ifdef DEBUG
	Msg("pause [%s] timer=[%s] sound=[%s] reason=%s", bOn ? "ON" : "OFF", bTimer ? "ON" : "OFF", bSound ? "ON" : "OFF", reason);
#endif // DEBUG

#ifndef DEDICATED_SERVER

	if (bOn)
	{
		if (!Paused())
			bShowPauseString = TRUE;

		if (bTimer && g_pGamePersistent->CanBePaused())
			g_pauseMngr.Pause(TRUE);

		if (bSound)
		{
			snd_emitters_ = ::Sound->pause_emitters(true);
#ifdef DEBUG
			Log("snd_emitters_[true]", snd_emitters_);
#endif // DEBUG
		}
	}
	else
	{
		if (bTimer && /*g_pGamePersistent->CanBePaused() &&*/ g_pauseMngr.Paused())		
			g_pauseMngr.Pause(FALSE);		

		if (bSound)
		{
			if (snd_emitters_ > 0) //avoid crash
			{
				snd_emitters_ = ::Sound->pause_emitters(false);
#ifdef DEBUG
				Log("snd_emitters_[false]", snd_emitters_);
#endif // DEBUG
			}
			else
			{
#ifdef DEBUG
				Log("Sound->pause_emitters underflow");
#endif // DEBUG
			}
		}
	}

#endif
}

BOOL CRenderDevice::Paused()
{
	return g_pauseMngr.Paused();
};

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM lParam)
{
	u16 fActive = LOWORD(wParam);
	BOOL fMinimized = (BOOL)HIWORD(wParam);
	BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;

	if (strstr(Core.Params, "-always_active"))
	{
		Device.b_is_Active = TRUE;

		if (fActive)
			pInput->OnAppActivate();
		else
			pInput->OnAppDeactivate();

		return;
	}

	if (bActive != Device.b_is_Active)
	{
		Device.b_is_Active = bActive;

		if (Device.b_is_Active)
		{
			Device.seqAppActivate.Process(rp_AppActivate);
#ifndef DEDICATED_SERVER
				ShowCursor(FALSE);
#endif
		}
		else
		{
			Device.seqAppDeactivate.Process(rp_AppDeactivate);
			ShowCursor(TRUE);
		}
	}
}