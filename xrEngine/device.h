#pragma once

// Note:
// ZNear - always 0.0f
// ZFar  - always 1.0f

#include "pure.h"
#include "hw.h"
#include "ftimer.h"
#include "stats.h"

#define VIEWPORT_NEAR 0.2f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "../Include/xrRender/RenderDeviceRender.h"

// refs
class ENGINE_API CRenderDevice
{
private:
	// Main objects used for creating and rendering the 3D scene

	u32 m_SystemLocalTimersDelta;
	CTimer_paused m_FrameTimer;
	CTimer_paused m_GlobalTimer;
	CTimer TimerMM;

	void _Create(LPCSTR shName);
	void _Destroy(BOOL bKeepTextures);
	void _SetupStates();
	ICF void ProcessRender();
	void PrepareToRun();

public:
	HWND m_hWnd;

	u32 CurrentFrameNumber;
	u32 dwPrecacheFrame;
	u32 dwPrecacheTotal;

	u32 dwWidth, dwHeight;
	float fWidth_2, fHeight_2;
	BOOL b_is_Ready;
	BOOL b_is_Active;
	void OnWM_Activate(WPARAM wParam, LPARAM lParam);

public:
	IRenderDeviceRender *m_pRender;
	BOOL m_bNearer;

	void SetNearer(BOOL enabled)
	{
		if (enabled && !m_bNearer)
		{
			m_bNearer = TRUE;
			mProject._43 -= EPS_L;
		}
		else if (!enabled && m_bNearer)
		{
			m_bNearer = FALSE;
			mProject._43 += EPS_L;
		}
		m_pRender->SetCacheXform(mView, mProject);
	}

public:
	// Registrators
	CRegistrator<pureRender> seqRender;
	CRegistrator<pureAppActivate> seqAppActivate;
	CRegistrator<pureAppDeactivate> seqAppDeactivate;
	CRegistrator<pureAppStart> seqAppStart;
	CRegistrator<pureAppEnd> seqAppEnd;
	CRegistrator<pureFrame> seqFrame;
	CRegistrator<pureFrame> seqFrameMT;
	CRegistrator<pureDeviceReset> seqDeviceReset;
	xr_vector<fastdelegate::FastDelegate0<>> seqParallel;

	// Dependent classes
	CStats *Statistic;	

	// Engine flow-control
	float fTimeDelta;
	float fTimeGlobal;
	u32 dwTimeDelta;
	u32 dwTimeGlobal;
	u32 dwTimeContinual;

	// Cameras & projection
	Fvector vCameraPosition;
	Fvector vCameraDirection;
	Fvector vCameraTop;
	Fvector vCameraRight;
	Fmatrix mView;
	Fmatrix mProject;
	Fmatrix mFullTransform;
	Fmatrix mInvFullTransform;
	float fFOV;
	float fASPECT;

	CRenderDevice()
#ifdef PROFILE_CRITICAL_SECTIONS
		: mt_csEnter(MUTEX_PROFILE_ID(CRenderDevice::mt_csEnter)), mt_csLeave(MUTEX_PROFILE_ID(CRenderDevice::mt_csLeave))
#endif // PROFILE_CRITICAL_SECTIONS
	{
		m_hWnd = NULL;
		b_is_Active = FALSE;
		b_is_Ready = FALSE;
		m_FrameTimer.Start();
		m_bNearer = FALSE;
		m_pRender = nullptr;
	};

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL Paused();

	// Scene control
	void PreCache(u32 frames);
	BOOL Begin();
	void Clear();
	void End();
	void FrameMove();

	void overdrawBegin();
	void overdrawEnd();

	// Mode control
	void DumpFlags();
	IC CTimer_paused *GetTimerGlobal() { return &m_GlobalTimer; }
	u32 TimerAsync() { return m_GlobalTimer.GetElapsed_ms(); }
	u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + m_SystemLocalTimersDelta; }

	// Creation & Destroying
	void ConnectToRender();
	void Create(void);
	void Run(void);
	void Destroy(void);
	void Reset(bool precache = true);

	void Initialize(void);

public:

	// Multi-threading
	xrCriticalSection mt_csEnter;
	xrCriticalSection mt_csLeave;
	volatile BOOL mt_bMustExit;

	ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<> &delegate)
	{
		auto It = find(seqParallel.begin(), seqParallel.end(), delegate);

		if (It != seqParallel.end())
			seqParallel.erase(It);
	}
};

extern ENGINE_API CRenderDevice Device;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;
