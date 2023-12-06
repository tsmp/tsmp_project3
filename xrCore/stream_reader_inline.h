#pragma once

IC const HANDLE &CStreamReader::file_mapping_handle() const
{
	return (m_file_mapping_handle);
}

IC void CStreamReader::unmap()
{
	UnmapViewOfFile(m_current_map_view_of_file);
}

IC void CStreamReader::remap(const u32 &new_offset)
{
	unmap();
	map(new_offset);
}

IC u32 CStreamReader::elapsed() const
{
	u32 offset_from_file_start = tell();
	VERIFY(m_file_size >= offset_from_file_start);
	return (m_file_size - offset_from_file_start);
}

IC const u32 &CStreamReader::length() const
{
	return (m_file_size);
}

IC void CStreamReader::seek(const int &offset)
{
	advance(offset - tell());
}

IC u32 CStreamReader::tell() const
{
	VERIFY(m_current_pointer >= m_start_pointer);
	VERIFY(u32(m_current_pointer - m_start_pointer) <= m_current_window_size);
	return xr_narrow_cast<u32>(m_current_offset_from_start + (m_current_pointer - m_start_pointer));
}

IC void CStreamReader::close()
{
	destroy();
	CStreamReader *self = this;
	xr_delete(self);
}
