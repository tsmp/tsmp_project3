// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable : 4995)
#include "..\..\xrEngine\stdafx.h"
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)
#pragma warning(disable : 4714)
#pragma warning(4 : 4018)
#pragma warning(4 : 4244)
#pragma warning(disable : 4237)

#define R_R1 1
#define R_R2 2
#define RENDER R_R2

#include "../xrEngine/HW.h"
#include "../xrRender/Shader.h"
#include "../xrRender/R_Backend.h"
#include "../xrRender/R_Backend_Runtime.h"

#include "../xrRender/ResourceManager.h"
#include "vis_common.h"
#include "render.h"
#include "igame_level.h"
#include "..\xrRender\blenders\blender.h"
#include "..\xrRender\blenders\blender_clsid.h"
#include "psystem.h"
#include "..\xrRender\xrRender_console.h"
#include "r2.h"

IC void jitter(CBlender_Compile &C)
{
	C.r_Sampler("jitter0", JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_Sampler("jitter1", JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_Sampler("jitter2", JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_Sampler("jitter3", JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}
