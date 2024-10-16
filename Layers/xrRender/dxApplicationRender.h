#pragma once

#include "..\..\Include\xrRender\ApplicationRender.h"

class dxApplicationRender : public IApplicationRender
{
public:
	virtual void Copy(IApplicationRender &_in);

	virtual void LoadBegin();
	virtual void destroy_loading_shaders();
	virtual void setLevelLogo(LPCSTR pszLogoName);
	virtual void load_draw_internal(CApplication &owner);

private:
	ref_shader hLevelLogo;
	ref_geom ll_hGeom;
	ref_geom ll_hGeom2;

	ref_shader sh_progress;
	void draw_face(ref_shader &sh, Frect &coords, Frect &tex_coords, const Fvector2 &tex_size);
};
