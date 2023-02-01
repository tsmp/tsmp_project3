#include "stdafx.h"
#include "../../xrEngine/render.h"
#include "fbasicvisual.h"
#include "../../xrEngine/fmesh.h"

IRender_Mesh::~IRender_Mesh()
{
	_RELEASE(p_rm_Vertices);
	_RELEASE(p_rm_Indices);
}

dxRender_Visual::dxRender_Visual()
{
	Type = 0;
	shader = 0;
	vis.clear();
}

dxRender_Visual::~dxRender_Visual()
{
}

void dxRender_Visual::Release()
{
}

void dxRender_Visual::Load(const char *N, IReader *data, u32)
{
#ifdef DEBUG
	dbg_name = N;
#endif

	// header
	VERIFY(data);
	ogf_header hdr;
	if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
	{
		R_ASSERT2(hdr.format_version == xrOGF_FormatVersion, "Invalid visual version");
		Type = hdr.type;
		if (hdr.shader_id)
			shader = ::Render->getShader(hdr.shader_id);
		vis.box.set(hdr.bb.min, hdr.bb.max);
		vis.sphere.set(hdr.bs.c, hdr.bs.r);
	}
	else
	{
		FATAL("Invalid visual");
	}

	// Shader
	if (data->find_chunk(OGF_TEXTURE))
	{
		string256 fnT, fnS;
		data->r_stringZ(fnT, sizeof(fnT));
		data->r_stringZ(fnS, sizeof(fnS));
		shader.create(fnS, fnT);
	}
}

#define PCOPY(a) a = pFrom->a
void dxRender_Visual::Copy(dxRender_Visual *pFrom)
{
	PCOPY(Type);
	PCOPY(shader);
	PCOPY(vis);

#ifdef DEBUG
	PCOPY(dbg_name);
#endif
}
