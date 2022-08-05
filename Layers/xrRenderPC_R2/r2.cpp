#include "stdafx.h"
#include "r2.h"
#include "fbasicvisual.h"
#include "xr_object.h"
#include "CustomHUD.h"
#include "igame_persistent.h"
#include "environment.h"
#include "SkeletonCustom.h"
#include "..\xrRender\LightTrack.h"
#include "GameFont.h"
#include "..\xrRender\r__shaders_cache.h"

#include <boost/crc.hpp>

CRender RImplementation;

//////////////////////////////////////////////////////////////////////////
class CGlow : public IRender_Glow
{
public:
	bool bActive;

public:
	CGlow() : bActive(false) {}
	virtual void set_active(bool b) { bActive = b; }
	virtual bool get_active() { return bActive; }
	virtual void set_position(const Fvector& P) {}
	virtual void set_direction(const Fvector& D) {}
	virtual void set_radius(float R) {}
	virtual void set_texture(LPCSTR name) {}
	virtual void set_color(const Fcolor& C) {}
	virtual void set_color(float r, float g, float b) {}
};

float r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq)
{
	int id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq)
{
	int id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
static class cl_parallax : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float h = ps_r2_df_parallax_h;
		RCache.set_c(C, h, -h / 2.f, 1.f / r_dtex_range, 1.f / r_dtex_range);
	}
} binder_parallax;

extern ENGINE_API BOOL r2_sun_static;
//////////////////////////////////////////////////////////////////////////
// Just two static storage
void CRender::create()
{
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	m_skinning = -1;

	// hardware
	o.smapsize = 2048;
	o.mrt = (HW.Caps.raster.dwMRT_count >= 3);
	o.mrtmixdepth = (HW.Caps.raster.b_MRT_mixdepth);

	// Check for NULL render target support
	D3DFORMAT nullrt = (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L');
	o.nullrt = HW.support(nullrt, D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET);
	/*
	if (o.nullrt)		{
	Msg				("* NULLRT supported and used");
	};
	*/
	if (o.nullrt)
	{
		Msg("* NULLRT supported");

		//.	    _tzset			();
		//.		??? _strdate	( date, 128 );	???
		//.		??? if (date < 22-march-07)
		if (0)
		{
			u32 device_id = HW.Caps.id_device;
			bool disable_nullrt = false;
			switch (device_id)
			{
			case 0x190:
			case 0x191:
			case 0x192:
			case 0x193:
			case 0x194:
			case 0x197:
			case 0x19D:
			case 0x19E:
			{
				disable_nullrt = true; //G80
				break;
			}
			case 0x400:
			case 0x401:
			case 0x402:
			case 0x403:
			case 0x404:
			case 0x405:
			case 0x40E:
			case 0x40F:
			{
				disable_nullrt = true; //G84
				break;
			}
			case 0x420:
			case 0x421:
			case 0x422:
			case 0x423:
			case 0x424:
			case 0x42D:
			case 0x42E:
			case 0x42F:
			{
				disable_nullrt = true; // G86
				break;
			}
			}
			if (disable_nullrt)
				o.nullrt = false;
		};
		if (o.nullrt)
			Msg("* ...and used");
	};

	// SMAP / DST
	o.HW_smap_FETCH4 = FALSE;
	o.HW_smap = HW.support(D3DFMT_D24X8, D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);
	o.HW_smap_PCF = o.HW_smap;
	if (o.HW_smap)
	{
		o.HW_smap_FORMAT = D3DFMT_D24X8;
		Msg("* HWDST/PCF supported and used");
	}

	o.fp16_filter = HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_FILTER);
	o.fp16_blend = HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);

	// search for ATI formats
	if (!o.HW_smap && (0 == strstr(Core.Params, "-nodf24")))
	{
		o.HW_smap = HW.support((D3DFORMAT)(MAKEFOURCC('D', 'F', '2', '4')), D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);
		if (o.HW_smap)
		{
			o.HW_smap_FORMAT = MAKEFOURCC('D', 'F', '2', '4');
			o.HW_smap_PCF = FALSE;
			o.HW_smap_FETCH4 = TRUE;
		}
		Msg("* DF24/F4 supported and used [%X]", o.HW_smap_FORMAT);
	}

	// emulate ATI-R4xx series
	if (strstr(Core.Params, "-r4xx"))
	{
		o.mrtmixdepth = FALSE;
		o.HW_smap = FALSE;
		o.HW_smap_PCF = FALSE;
		o.fp16_filter = FALSE;
		o.fp16_blend = FALSE;
	}

	VERIFY2(o.mrt && (HW.Caps.raster.dwInstructions >= 256), "Hardware doesn't meet minimum feature-level");
	if (o.mrtmixdepth)
		o.albedo_wo = FALSE;
	else if (o.fp16_blend)
		o.albedo_wo = FALSE;
	else
		o.albedo_wo = TRUE;

	// nvstencil on NV40 and up
	o.nvstencil = FALSE;
	if ((HW.Caps.id_vendor == 0x10DE) && (HW.Caps.id_device >= 0x40))
		o.nvstencil = TRUE;
	if (strstr(Core.Params, "-nonvs"))
		o.nvstencil = FALSE;

	// nv-dbt
	o.nvdbt = HW.support((D3DFORMAT)MAKEFOURCC('N', 'V', 'D', 'B'), D3DRTYPE_SURFACE, 0);
	if (o.nvdbt)
		Msg("* NV-DBT supported and used");

	// options (smap-pool-size)
	if (strstr(Core.Params, "-smap1536"))
		o.smapsize = 1536;
	if (strstr(Core.Params, "-smap2048"))
		o.smapsize = 2048;
	if (strstr(Core.Params, "-smap2560"))
		o.smapsize = 2560;
	if (strstr(Core.Params, "-smap3072"))
		o.smapsize = 3072;
	if (strstr(Core.Params, "-smap4096"))
		o.smapsize = 4096;

	// gloss
	char* g = strstr(Core.Params, "-gloss ");
	o.forcegloss = g ? TRUE : FALSE;
	if (g)
	{
		o.forcegloss_v = float(atoi(g + xr_strlen("-gloss "))) / 255.f;
	}

	// options
	o.bug = (strstr(Core.Params, "-bug")) ? TRUE : FALSE;
	o.sunfilter = (strstr(Core.Params, "-sunfilter")) ? TRUE : FALSE;
	//.	o.sunstatic			= (strstr(Core.Params,"-sunstatic"))?	TRUE	:FALSE	;
	o.sunstatic = r2_sun_static;
	o.sjitter = (strstr(Core.Params, "-sjitter")) ? TRUE : FALSE;
	o.depth16 = (strstr(Core.Params, "-depth16")) ? TRUE : FALSE;
	o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
	o.Tshadows = (strstr(Core.Params, "-tsh")) ? TRUE : FALSE;
	o.mblur = (strstr(Core.Params, "-mblur")) ? TRUE : FALSE;
	o.distortion_enabled = (strstr(Core.Params, "-nodistort")) ? FALSE : TRUE;
	o.distortion = o.distortion_enabled;
	o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

	// constants
	::Device.Resources->RegisterConstantSetup("parallax", &binder_parallax);

	c_lmaterial = "L_material";
	c_sbase = "s_base";
	m_bMakeAsyncSS = false;

	Target = xr_new<CRenderTarget>(); // Main target

	Models = xr_new<CModelPool>();
	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	//rmNormal					();
	marker = 0;
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[0]));
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[1]));

	xrRender_apply_tf();
	::PortalTraverser.initialize();
}

void CRender::destroy()
{
	m_bMakeAsyncSS = false;
	::PortalTraverser.destroy();
	_RELEASE(q_sync_point[1]);
	_RELEASE(q_sync_point[0]);
	HWOCC.occq_destroy();
	xr_delete(Models);
	xr_delete(Target);
	PSLibrary.OnDestroy();
	Device.seqFrame.Remove(this);
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it = 0;
		for (it = 0; it < Lights_LastFrame.size(); it++)
		{
			if (0 == Lights_LastFrame[it])
				continue;
			try
			{
				Lights_LastFrame[it]->svis.resetoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear();
	}

	xr_delete(Target);
	HWOCC.occq_destroy();
	_RELEASE(q_sync_point[1]);
	_RELEASE(q_sync_point[0]);
}

void CRender::reset_end()
{
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[0]));
	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[1]));
	HWOCC.occq_create(occq_size);

	Target = xr_new<CRenderTarget>();

	xrRender_apply_tf();
}
/*
void CRender::OnFrame()
{
	Models->DeleteQueue			();
	if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))	{
		Device.seqParallel.insert	(Device.seqParallel.begin(),
			fastdelegate::FastDelegate0<>(&HOM,&CHOM::MT_RENDER));
	}
}*/
void CRender::OnFrame()
{
	Models->DeleteQueue();
	if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_CALC))
	{
		// MT-details (@front)
		Device.seqParallel.insert(Device.seqParallel.begin(),
								  fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));

		// MT-HOM (@front)
		Device.seqParallel.insert(Device.seqParallel.begin(),
								  fastdelegate::FastDelegate0<>(&HOM, &CHOM::MT_RENDER));
	}
}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent) { return xr_new<CROS_impl>(); }
void CRender::ros_destroy(IRender_ObjectSpecific*& p) { xr_delete(p); }
IRender_Visual* CRender::model_Create(LPCSTR name, IReader* data) { return Models->Create(name, data); }
IRender_Visual* CRender::model_CreateChild(LPCSTR name, IReader* data) { return Models->CreateChild(name, data); }
IRender_Visual* CRender::model_Duplicate(IRender_Visual* V) { return Models->Instance_Duplicate(V); }
void CRender::model_Delete(IRender_Visual*& V, BOOL bDiscard) { Models->Delete(V, bDiscard); }
IRender_DetailModel* CRender::model_CreateDM(IReader* F)
{
	CDetail* D = xr_new<CDetail>();
	D->Load(F);
	return D;
}
void CRender::model_Delete(IRender_DetailModel*& F)
{
	if (F)
	{
		CDetail* D = (CDetail*)F;
		D->Unload();
		xr_delete(D);
		F = NULL;
	}
}
IRender_Visual* CRender::model_CreatePE(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	R_ASSERT3(SE, "Particle effect doesn't exist", name);
	return Models->CreatePE(SE);
}
IRender_Visual* CRender::model_CreateParticles(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	if (SE)
		return Models->CreatePE(SE);
	else
	{
		PS::CPGDef* SG = PSLibrary.FindPGD(name);
		R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
		return Models->CreatePG(SG);
	}
}
void CRender::models_Prefetch() { Models->Prefetch(); }
void CRender::models_Clear(BOOL b_complete) { Models->ClearPool(b_complete); }

ref_shader CRender::getShader(int id)
{
	VERIFY(id < int(Shaders.size()));
	return Shaders[id];
}
IRender_Portal* CRender::getPortal(int id)
{
	VERIFY(id < int(Portals.size()));
	return Portals[id];
}
IRender_Sector* CRender::getSector(int id)
{
	VERIFY(id < int(Sectors.size()));
	return Sectors[id];
}
IRender_Sector* CRender::getSectorActive() { return pLastSector; }
IRender_Visual* CRender::getVisual(int id)
{
	VERIFY(id < int(Visuals.size()));
	return Visuals[id];
}
D3DVERTEXELEMENT9* CRender::getVB_Format(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xDC.size()));
		return xDC[id].begin();
	}
	else
	{
		VERIFY(id < int(nDC.size()));
		return nDC[id].begin();
	}
}
IDirect3DVertexBuffer9* CRender::getVB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xVB.size()));
		return xVB[id];
	}
	else
	{
		VERIFY(id < int(nVB.size()));
		return nVB[id];
	}
}
IDirect3DIndexBuffer9* CRender::getIB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xIB.size()));
		return xIB[id];
	}
	else
	{
		VERIFY(id < int(nIB.size()));
		return nIB[id];
	}
}
FSlideWindowItem* CRender::getSWI(int id)
{
	VERIFY(id < int(SWIs.size()));
	return &SWIs[id];
}
IRender_Target* CRender::getTarget() { return Target; }

IRender_Light* CRender::light_create() { return Lights.Create(); }
IRender_Glow* CRender::glow_create() { return xr_new<CGlow>(); }

void CRender::flush() { r_dsgraph_render_graph(0); }

BOOL CRender::occ_visible(vis_data& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(sPoly& P) { return HOM.visible(P); }
BOOL CRender::occ_visible(Fbox& P) { return HOM.visible(P); }

void CRender::add_Visual(IRender_Visual* V) { add_leafs_Dynamic(V); }
void CRender::add_Geometry(IRender_Visual* V) { add_Static(V, View->getMask()); }
void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)
		return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::clear_static_wallmarks()
{
	Wallmarks->clear();
}

void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark(wm);
}
void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
void CRender::add_Occluder(Fbox2& bb_screenspace)
{
	HOM.occlude(bb_screenspace);
}
void CRender::set_Object(IRenderable* O)
{
	val_pObject = O;
}
void CRender::rmNear()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = { 0, 0, T->get_width(), T->get_height(), 0, 0.02f };
	CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmFar()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = { 0, 0, T->get_width(), T->get_height(), 0.99999f, 1.f };
	CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmNormal()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = { 0, 0, T->get_width(), T->get_height(), 0, 1.f };
	CHK_DX(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender()
{
}

CRender::~CRender()
{
}

void CRender::Statistics(CGameFont* _F)
{
	CGameFont& F = *_F;
	F.OutNext(" **** LT:%2d,LV:%2d **** ", stats.l_total, stats.l_visible);
	stats.l_visible = 0;
	F.OutNext("    S(%2d)   | (%2d)NS   ", stats.l_shadowed, stats.l_unshadowed);
	F.OutNext("smap use[%2d], merge[%2d], finalclip[%2d]", stats.s_used, stats.s_merged - stats.s_used, stats.s_finalclip);
	stats.s_used = 0;
	stats.s_merged = 0;
	stats.s_finalclip = 0;
	F.OutSkip();
	F.OutNext(" **** Occ-Q(%03.1f) **** ", 100.f * f32(stats.o_culled) / f32(stats.o_queries ? stats.o_queries : 1));
	F.OutNext(" total  : %2d", stats.o_queries);
	stats.o_queries = 0;
	F.OutNext(" culled : %2d", stats.o_culled);
	stats.o_culled = 0;
	F.OutSkip();
	u32 ict = stats.ic_total + stats.ic_culled;
	F.OutNext(" **** iCULL(%03.1f) **** ", 100.f * f32(stats.ic_culled) / f32(ict ? ict : 1));
	F.OutNext(" visible: %2d", stats.ic_total);
	stats.ic_total = 0;
	F.OutNext(" culled : %2d", stats.ic_culled);
	stats.ic_culled = 0;
#ifdef DEBUG
	HOM.stats();
#endif
}

static const std::map<NewFlagsR2, const char*> NewFlagDefines
{
	{ NewFlagsR2::SSAO, "USE_SSAO"},
	{ NewFlagsR2::DOF, "USE_DEPTH_OF_FIELD"},
	{ NewFlagsR2::COLOR_FRINGE, "USE_COLOR_FRINGE"},
	{ NewFlagsR2::SOFT_SHADOWS, "USE_SOFT_SHADOWS"},
	{ NewFlagsR2::SHAFTS, "USE_SHAFTS"},
	{ NewFlagsR2::SHAFTS_HQ, "SHAFTS_HQ"},
	{ NewFlagsR2::SHAFTS_ENHANCED, "SHAFTS_ENHANCED"},
	{ NewFlagsR2::SHAFTS_DUST, "SHAFTS_DUST"},
	{ NewFlagsR2::SATURATE, "USE_SATURATION"},
	{ NewFlagsR2::FOG, "USE_HORIZON_FOG"},
	{ NewFlagsR2::RAINBOW, "USE_RAINBOW"},
	{ NewFlagsR2::SOFT_WATER, "USE_SOFT_WATER"},
	{ NewFlagsR2::SOFT_PARTICLES, "USE_SOFT_PARTICLES"},
	{ NewFlagsR2::TREES_DARK, "USE_TREES_DARK"},
	{ NewFlagsR2::TREES_FAST, "USE_TREES_FAST"},
	{ NewFlagsR2::MODELS_BRIGHT, "USE_MODELS_BRIGHT"},
	{ NewFlagsR2::COLOR_B_FILTER, "USE_COLOR_B_FILTER"},
	{ NewFlagsR2::CINEMATIC, "USE_SUPER_CINEMATIC"},
	{ NewFlagsR2::CONSTRAST, "USE_SUPER_CONTRAST"},
	{ NewFlagsR2::HYPERSONIC, "USE_HYPERSONIC"},
	{ NewFlagsR2::SURERGLOSS, "USE_SUPER_GLOSS"}
};

#pragma comment(lib, "d3dx9.lib")

HRESULT	CRender::shader_compile(
		LPCSTR name,
		char const* pSrcData,
		UINT SrcDataLen,
		LPCSTR pFunctionName,
		LPCSTR pTarget,
		DWORD Flags,
		void*& result)
{
	D3DXMACRO defines[128]{};
	int def_it = 0;
	char c_smapsize[32];
	char c_gloss[32];

	char sh_name[MAX_PATH] = "";
	size_t len = 0;

	// options
	{
		sprintf(c_smapsize, "%d", u32(o.smapsize));
		defines[def_it].Name = "SMAP_size";
		defines[def_it].Definition = c_smapsize;
		def_it++;
		strcat(sh_name, c_smapsize);
		len += 4;
	}

	if (o.fp16_filter) 
	{
		defines[def_it].Name = "FP16_FILTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.fp16_filter);
	++len;

	if (o.fp16_blend) 
	{
		defines[def_it].Name = "FP16_BLEND";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.fp16_blend);
	++len;

	if (o.HW_smap) 
	{
		defines[def_it].Name = "USE_HWSMAP";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.HW_smap);
	++len;

	if (o.HW_smap_PCF) 
	{
		defines[def_it].Name = "USE_HWSMAP_PCF";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.HW_smap_PCF);
	++len;

	if (o.HW_smap_FETCH4) 
	{
		defines[def_it].Name = "USE_FETCH4";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.HW_smap_FETCH4);
	++len;

	if (o.sjitter) 
	{
		defines[def_it].Name = "USE_SJITTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.sjitter);
	++len;

	if (HW.Caps.raster_major >= 3) 
	{
		defines[def_it].Name = "USE_BRANCHING";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(HW.Caps.raster_major >= 3);
	++len;

	if (HW.Caps.geometry.bVTF) 
	{
		defines[def_it].Name = "USE_VTF";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(HW.Caps.geometry.bVTF);
	++len;

	if (o.Tshadows) 
	{
		defines[def_it].Name = "USE_TSHADOWS";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.Tshadows);
	++len;

	if (o.mblur) 
	{
		defines[def_it].Name = "USE_MBLUR";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.mblur);
	++len;

	if (o.sunfilter) 
	{
		defines[def_it].Name = "USE_SUNFILTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.sunfilter);
	++len;

	if (o.sunstatic) 
	{
		defines[def_it].Name = "USE_R2_STATIC_SUN";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.sunstatic);
	++len;

	if (o.forcegloss) 
	{
		sprintf(c_gloss, "%f", o.forcegloss_v);
		defines[def_it].Name = "FORCE_GLOSS";
		defines[def_it].Definition = c_gloss;
		def_it++;
	}
	sh_name[len] = '0' + char(o.forcegloss);
	++len;

	if (o.forceskinw) 
	{
		defines[def_it].Name = "SKIN_COLOR";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(o.forceskinw);
	++len;

	// skinning
	if (m_skinning < 0) 
	{
		defines[def_it].Name = "SKIN_NONE";
		defines[def_it].Definition = "1";
		def_it++;
		sh_name[len] = '1';
		++len;
	}
	else
	{
		sh_name[len] = '0';
		++len;
	}

	if (0 == m_skinning) 
	{
		defines[def_it].Name = "SKIN_0";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(0 == m_skinning);
	++len;

	if (1 == m_skinning) 
	{
		defines[def_it].Name = "SKIN_1";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(1 == m_skinning);
	++len;

	if (2 == m_skinning) 
	{
		defines[def_it].Name = "SKIN_2";
		defines[def_it].Definition = "1";
		def_it++;
	}
	sh_name[len] = '0' + char(2 == m_skinning);
	++len;

	for (auto const& elem : NewFlagDefines)
	{
		if (ps_r2_new_flags.test(static_cast<u32>(elem.first)))
		{
			defines[def_it].Name = elem.second;
			defines[def_it].Definition = "1";
			def_it++;
		}
	}

	// finish
	defines[def_it].Name = 0;
	defines[def_it].Definition = 0;
	def_it++;

	//Precache
	HRESULT _result = E_FAIL;

	string_path	folder_name, folder;
	strcpy(folder, "r2\\objects\\r2\\");
	strcat(folder, name);
	strcat(folder, ".");

	char extension[3];
	strncpy_s(extension, pTarget, 2);
	strcat(folder, extension);

	FS.update_path(folder_name, "$game_shaders$", folder);
	strcat(folder_name, "\\");

	m_file_set.clear();
	FS.file_list(m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

	string_path temp_file_name, file_name;
	if (!match_shader_id(name, sh_name, m_file_set, temp_file_name)) 
	{
		string_path file;
		strcpy(file, "shaders_cache\\r2\\");
		strcat(file, name);
		strcat(file, ".");
		strcat(file, extension);
		strcat(file, "\\");
		strcat(file, sh_name);
		FS.update_path(file_name, "$app_data_root$", file);
	}
	else
	{
		strcpy(file_name, folder_name);
		strcat(file_name, temp_file_name);
	}

	if (FS.exist(file_name))
	{
		IReader* file = FS.r_open(file_name);
		if (file->length() > 4)
		{
			u32 crc = 0;
			crc = file->r_u32();

			boost::crc_32_type processor;
			processor.process_block(file->pointer(), ((char*)file->pointer()) + file->elapsed());
			u32 const real_crc = processor.checksum();

			if (real_crc == crc)
			{
				_result = create_shader(pTarget, (DWORD*)file->pointer(), static_cast<u32>(file->elapsed()), file_name, result, o.disasm);
			}
		}
		file->close();
	}

	if (FAILED(_result))
	{
		if (0 == xr_strcmp(pFunctionName, "main"))
		{
			if ('v' == pTarget[0])
				pTarget = D3DXGetVertexShaderProfile(HW.pDevice);	// vertex	"vs_2_a"; //
			else
				pTarget = D3DXGetPixelShaderProfile(HW.pDevice);	// pixel	"ps_2_a"; //
		}

		includer Includer;
		LPD3DXBUFFER pShaderBuf = nullptr;
		LPD3DXBUFFER pErrorBuf = nullptr;
		LPD3DXCONSTANTTABLE pConstants = nullptr;
		LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)&Includer;

		_result = D3DXCompileShader((LPCSTR)pSrcData, SrcDataLen, defines, pInclude, pFunctionName, pTarget, Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &pShaderBuf, &pErrorBuf, &pConstants);

		if (SUCCEEDED(_result)) 
		{
			IWriter* file = FS.w_open(file_name);

			boost::crc_32_type processor;
			processor.process_block(pShaderBuf->GetBufferPointer(), ((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
			u32 const crc = processor.checksum();

			file->w_u32(crc);
			file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
			FS.w_close(file);

			_result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), file_name, result, o.disasm);
		}
		else
		{
			Log("! ", file_name);
			if (pErrorBuf)
				Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
			else
				Msg("Can't compile shader hr=0x%08x", _result);
		}
	}

	return _result;
}
