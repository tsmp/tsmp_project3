#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Target.h"

void CSoundRender_Emitter::fill_data(u8 *_dest, u32 offset, u32 size)
{
	u32 line_size = SoundRender->cache.get_linesize();
	u32 line = offset / line_size;

	// prepare for first line (it can be unaligned)
	u32 line_offs = offset - line * line_size;
	u32 line_amount = line_size - line_offs;

	while (size)
	{
		// cache access
		if (SoundRender->cache.request(source->CAT, line))		
			source->decompress(line, target->get_data());		

		// fill block
		u32 blk_size = _min(size, line_amount);
		u8 *ptr = (u8 *)SoundRender->cache.get_dataptr(source->CAT, line);
		CopyMemory(_dest, ptr + line_offs, blk_size);

		// advance
		line++;
		size -= blk_size;
		_dest += blk_size;
		offset += blk_size;
		line_offs = 0;
		line_amount = line_size;
	}
}

void CSoundRender_Emitter::fill_block(void *ptr, u32 size)
{
	//Msg			("stream: %10s - [%X]:%d, p=%d, t=%d",*source->fname,ptr,size,position,source->dwBytesTotal);
	LPBYTE dest = LPBYTE(ptr);

	if ((position + size) > source->dwBytesTotal)
	{
		// We are reaching the end of data, what to do?
		switch (state)
		{
		case stPlaying:
		{
			// Fill as much data as we can, zeroing remainder
			if (position >= source->dwBytesTotal)
			{
				// ??? We requested the block after remainder - just zero
				Memory.mem_fill(dest, 0, size);
				//					Msg				("        playing: zero");
			}
			else
			{
				// Calculate remainder
				u32 sz_data = source->dwBytesTotal - position;
				u32 sz_zero = (position + size) - source->dwBytesTotal;
				VERIFY(size == (sz_data + sz_zero));
				fill_data(dest, position, sz_data);
				Memory.mem_fill(dest + sz_data, 0, sz_zero);
				//					Msg				("        playing: [%d]-normal,[%d]-zero",sz_data,sz_zero);
			}
			position += size;
		}
		break;

		case stPlayingLooped:
		{
			u32 hw_position = 0;
			do
			{
				u32 sz_data = source->dwBytesTotal - position;
				u32 sz_write = _min(size - hw_position, sz_data);
				fill_data(dest + hw_position, position, sz_write);
				hw_position += sz_write;
				position += sz_write;
				position %= source->dwBytesTotal;
			} while (0 != (size - hw_position));
		}
		break;

		default:
			FATAL("SOUND: Invalid emitter state");
			break;
		}
	}
	else
	{
		// Everything OK, just stream
		//		Msg				("        normal");
		fill_data(dest, position, size);
		position += size;
	}
}
