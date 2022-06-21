#pragma once

class includer : public ID3DXInclude
{
public:
	HRESULT __stdcall Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
	{
		string_path pname;
		strconcat(sizeof(pname), pname, ::Render->getShaderPath(), pFileName);
		IReader* R = FS.r_open("$game_shaders$", pname);
		if (0 == R)
		{
			R = FS.r_open("$game_shaders$", pFileName);
			if (0 == R)			return			E_FAIL;
		}

		// duplicate and zero-terminate
		size_t size = R->length();
		u8* data = xr_alloc<u8>(size + 1);
		CopyMemory(data, R->pointer(), size);
		data[size] = 0;
		FS.r_close(R);

		*ppData = data;
		*pBytes = static_cast<u32>(size);
		return D3D_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData)
	{
		xr_free(pData);
		return D3D_OK;
	}
};

extern bool match_shader_id(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);
extern bool match_shader(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length);

HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32	const buffer_size, LPCSTR const file_name, void*& result, bool const disasm);
