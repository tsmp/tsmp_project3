#include "stdafx.h"

#include "dxApplicationRender.h"
#include "../../xrEngine/xrApplication.h"

#include "../../xrEngine/GameFont.h"

void dxApplicationRender::Copy(IApplicationRender &_in)
{
	*this = *(dxApplicationRender *)&_in;
}

#pragma TODO("TSMP check why in cs there is hLevelLogo_Add")

void dxApplicationRender::LoadBegin()
{
	ll_hGeom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
	sh_progress.create("hud\\default", "ui\\ui_load");
	ll_hGeom2.create(FVF::F_TL, RCache.Vertex.Buffer(), NULL);
}

void dxApplicationRender::destroy_loading_shaders()
{
	hLevelLogo.destroy();
	sh_progress.destroy();
}

void dxApplicationRender::setLevelLogo(LPCSTR pszLogoName)
{
	hLevelLogo.create("font", pszLogoName);
}

u32 calc_progress_color(u32 idx, u32 total, int stage, int max_stage)
{
	if (idx > (total / 2))
		idx = total - idx;

	float kk = (float(stage + 1) / float(max_stage)) * (total / 2.0f);
	float f = 1 / (exp((float(idx) - kk) * 0.5f) + 1.0f);

	return color_argb_f(f, 1.0f, 1.0f, 1.0f);
}

void dxApplicationRender::load_draw_internal(CApplication &owner)
{
	if (!sh_progress)
	{
		CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0));
		return;
	}

	// Draw logo
	u32 Offset;
	u32 C = 0xffffffff;
	u32 _w = Device.dwWidth;
	u32 _h = Device.dwHeight;
	FVF::TL* pv = NULL;

	//progress
	float bw = 1024.0f;
	float bh = 768.0f;
	Fvector2 k;
	k.set(float(_w) / bw, float(_h) / bh);

	RCache.set_Shader(sh_progress);
	CTexture* T = RCache.get_ActiveTexture(0);
	Fvector2 tsz;
	tsz.set((float)T->get_Width(), (float)T->get_Height());
	Frect back_text_coords;
	Frect back_coords;
	Fvector2 back_size;

	//progress background
	static float offs = -0.5f;

	back_size.set(1024, 768);
	back_text_coords.lt.set(0, 0);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(offs, offs);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;
	pv = (FVF::TL*)RCache.Vertex.Lock(4, ll_hGeom.stride(), Offset);
	pv->set(back_coords.lt.x, back_coords.rb.y, C, back_text_coords.lt.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.lt.x, back_coords.lt.y, C, back_text_coords.lt.x, back_text_coords.lt.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.rb.y, C, back_text_coords.rb.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.lt.y, C, back_text_coords.rb.x, back_text_coords.lt.y);
	pv++;
	RCache.Vertex.Unlock(4, ll_hGeom.stride());

	RCache.set_Geometry(ll_hGeom);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	//progress bar
	back_size.set(268, 37);
	back_text_coords.lt.set(0, 768);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(379, 726);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;

	u32 v_cnt = 40;
	pv = (FVF::TL*)RCache.Vertex.Lock(2 * (v_cnt + 1), ll_hGeom2.stride(), Offset);
	FVF::TL* _pv = pv;
	float pos_delta = back_coords.width() / v_cnt;
	float tc_delta = back_text_coords.width() / v_cnt;
	u32 clr = C;

	for (u32 idx = 0; idx < v_cnt + 1; ++idx)
	{
		clr = calc_progress_color(idx, v_cnt, owner.load_stage, owner.max_load_stage);
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.rb.y + offs, 0 + EPS_S, 1, clr, back_text_coords.lt.x + tc_delta * idx, back_text_coords.rb.y);
		pv++;
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.lt.y + offs, 0 + EPS_S, 1, clr, back_text_coords.lt.x + tc_delta * idx, back_text_coords.lt.y);
		pv++;
	}
	VERIFY(u32(pv - _pv) == 2 * (v_cnt + 1));
	RCache.Vertex.Unlock(2 * (v_cnt + 1), ll_hGeom2.stride());

	RCache.set_Geometry(ll_hGeom2);
	RCache.Render(D3DPT_TRIANGLESTRIP, Offset, 2 * v_cnt);

	// Draw title
	VERIFY(owner.pFontSystem);
	owner.pFontSystem->Clear();
	owner.pFontSystem->SetColor(color_rgba(157, 140, 120, 255));
	owner.pFontSystem->SetAligment(CGameFont::alCenter);
	owner.pFontSystem->OutI(0.f, 0.815f, owner.app_title);
	owner.pFontSystem->OnRender();

	//draw level-specific screenshot
	if (hLevelLogo)
	{
		Frect r;
		r.lt.set(257, 369);
		r.lt.x += offs;
		r.lt.y += offs;
		r.rb.add(r.lt, Fvector2().set(512, 256));
		r.lt.mul(k);
		r.rb.mul(k);
		pv = (FVF::TL*)RCache.Vertex.Lock(4, ll_hGeom.stride(), Offset);
		pv->set(r.lt.x, r.rb.y, C, 0, 1);
		pv++;
		pv->set(r.lt.x, r.lt.y, C, 0, 0);
		pv++;
		pv->set(r.rb.x, r.rb.y, C, 1, 1);
		pv++;
		pv->set(r.rb.x, r.lt.y, C, 1, 0);
		pv++;
		RCache.Vertex.Unlock(4, ll_hGeom.stride());

		RCache.set_Shader(hLevelLogo);
		RCache.set_Geometry(ll_hGeom);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void dxApplicationRender::draw_face(ref_shader &sh, Frect &coords, Frect &tex_coords, const Fvector2 &tsz)
{
#pragma TODO("TSMP: port from cs with load_draw_internal")
}
