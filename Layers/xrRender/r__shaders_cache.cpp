#include "stdafx.h"
#include "r__shaders_cache.h"

bool match_shader_id(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result)
{
#ifdef DEBUG
	LPCSTR temp = "";
	bool found = false;
	for (auto i = file_set.begin(); i != file_set.end(); ++i)
	{
		if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
		{
			VERIFY(!found);
			found = true;
			temp = (*i).name.c_str();
		}
	}

	strcpy(result, temp);
	return found;

#else // #ifdef DEBUG
	for (auto i = file_set.begin(); i != file_set.end(); ++i)
	{
		if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
		{
			strcpy(result, (*i).name.c_str());
			return true;
		}
	}

	return false;
#endif // #ifdef DEBUG
}

HRESULT create_shader(
	LPCSTR const pTarget,
	DWORD const* buffer,
	u32	const buffer_size,
	LPCSTR const file_name,
	void*& result,
	bool const disasm)
{
	HRESULT _result = E_FAIL;
	if (pTarget[0] == 'p')
	{
		SPS* sps_result = (SPS*)result;
		_result = HW.pDevice->CreatePixelShader(buffer, &sps_result->ps);
		if (!SUCCEEDED(_result))
		{
			Log("! PS: ", file_name);
			Msg("! CreatePixelShader hr == 0x%08x", _result);
			return E_FAIL;
		}

		LPCVOID data = nullptr;
		_result = D3DXFindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, NULL);
		if (SUCCEEDED(_result) && data)
		{
			LPD3DXSHADER_CONSTANTTABLE	pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
			sps_result->constants.parse(pConstants, 0x1);
		}
		else
		{
			Log("! PS: ", file_name);
			Msg("! D3DXFindShaderComment hr == 0x%08x", _result);
		}
	}
	else
	{
		SVS* svs_result = (SVS*)result;
		_result = HW.pDevice->CreateVertexShader(buffer, &svs_result->vs);
		if (!SUCCEEDED(_result))
		{
			Log("! VS: ", file_name);
			Msg("! CreatePixelShader hr == 0x%08x", _result);
			return		E_FAIL;
		}

		LPCVOID data = nullptr;
		_result = D3DXFindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, NULL);
		if (SUCCEEDED(_result) && data)
		{
			LPD3DXSHADER_CONSTANTTABLE	pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
			svs_result->constants.parse(pConstants, 0x2);
		}
		else
		{
			Log("! VS: ", file_name);
			Msg("! D3DXFindShaderComment hr == 0x%08x", _result);
		}
	}

	if (disasm)
	{
		ID3DXBuffer* _disasm = 0;
		D3DXDisassembleShader(LPDWORD(buffer), FALSE, 0, &_disasm);
		string_path dname;
		strconcat(sizeof(dname), dname, "disasm\\", file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
		IWriter* W = FS.w_open("$logs$", dname);
		W->w(_disasm->GetBufferPointer(), _disasm->GetBufferSize());
		FS.w_close(W);
		_RELEASE(_disasm);
	}

	return _result;
}

bool match_shader(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length)
{
	const size_t full_shader_id_length = xr_strlen(full_shader_id);
	R_ASSERT2(
		full_shader_id_length == mask_length,
		make_string(
			"bad cache for shader %s, [%s], [%s]",
			debug_shader_id,
			mask,
			full_shader_id
		)
	);

	const char* i;
	const char* j;
	const char* const e = full_shader_id + full_shader_id_length;
	for (i = full_shader_id, j = mask; i != e; ++i, ++j)
	{
		if (*i == *j)
			continue;

		if (*j == '_')
			continue;

		return false;
	}

	return true;
}
