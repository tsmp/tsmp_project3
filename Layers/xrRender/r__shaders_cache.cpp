#include "stdafx.h"
#include "r__shaders_cache.h"
#include "boost/crc.hpp"

static FS_FileSet s_FileSet;

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

void FillFileName(string_path &fileName, const char* shName, const char* cachedShName, const char* render /* r1/r2 */, const char* target)
{
	string_path folder_name, folder;
	strcpy_s(folder, render);
	strcat_s(folder,"\\objects\\");
	strcat_s(folder, render);
	strcat_s(folder, "\\");

	strcat_s(folder, shName);
	strcat_s(folder, ".");

	char extension[3];
	strncpy_s(extension, target, 2);
	strcat_s(folder, extension);

	FS.update_path(folder_name, "$game_shaders$", folder);
	strcat_s(folder_name, "\\");

	s_FileSet.clear();
	FS.file_list(s_FileSet, folder_name, FS_ListFiles | FS_RootOnly, "*");

	string_path temp_file_name;
	if (!match_shader_id(shName, cachedShName, s_FileSet, temp_file_name))
	{
		string_path file;
		strcpy_s(file, "shaders_cache\\");
		strcat_s(file, render);
		strcat_s(file, "\\");
		strcat_s(file, shName);
		strcat_s(file, ".");
		strcat_s(file, extension);
		strcat_s(file, "\\");
		strcat_s(file, cachedShName);
		FS.update_path(fileName, "$app_data_root$", file);
	}
	else
	{
		strcpy_s(fileName, folder_name);
		strcat_s(fileName, temp_file_name);
	}
}

bool LoadShaderFromCache(const string_path& fileName, const char* target, bool disasm, void*& result)
{
	if (!FS.exist(fileName))
		return false;

	HRESULT hres = E_FAIL;
	IReader* file = FS.r_open(fileName);

	if (file->length() > 4)
	{
		u32 crc = 0;
		crc = file->r_u32();

		boost::crc_32_type processor;
		processor.process_block(file->pointer(), ((char*)file->pointer()) + file->elapsed());
		u32 const real_crc = processor.checksum();

		if (real_crc == crc)
		{
#ifdef DEBUG
			Msg("shader loading from cache: %s", fileName);
#endif

			hres = create_shader(target, (DWORD*)file->pointer(), static_cast<u32>(file->elapsed()), fileName, result, disasm);
		}
	}

	file->close();
	return SUCCEEDED(hres);
}

HRESULT CompileShader(const char* pSrcData, const char* pFunctionName, const char* pTarget, int Flags, u32 SrcDataLen, const string_path& fileName, D3DXMACRO* defines, void*& result, bool disasm)
{
	includer Includer;
	LPD3DXBUFFER pShaderBuf = nullptr;
	LPD3DXBUFFER pErrorBuf = nullptr;
	LPD3DXCONSTANTTABLE pConstants = nullptr;
	LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)&Includer;
	Msg("- compiling shader: %s", fileName);

#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL // December 2006 and later
	HRESULT _result = D3DXCompileShader((LPCSTR)pSrcData, SrcDataLen, defines, pInclude, pFunctionName, pTarget, Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &pShaderBuf, &pErrorBuf, &pConstants);
#else
	HRESULT _result = D3DXCompileShader((LPCSTR)pSrcData, SrcDataLen, defines, pInclude, pFunctionName, pTarget, Flags, &pShaderBuf, &pErrorBuf, &pConstants);
#endif

	if (SUCCEEDED(_result))
	{
		IWriter* shdr_file = FS.w_open(fileName);

		boost::crc_32_type processor;
		processor.process_block(pShaderBuf->GetBufferPointer(), ((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
		u32 const crc = processor.checksum();

		shdr_file->w_u32(crc);
		shdr_file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
		FS.w_close(shdr_file);

		_result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), fileName, result, disasm);
	}
	else
	{
		Log("! ", fileName);
		if (pErrorBuf)
			Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
		else
			Msg("Can't compile shader hr=0x%08x", _result);
	}

	return _result;
}
