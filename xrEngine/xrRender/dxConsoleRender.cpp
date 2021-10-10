#include "stdafx.h"
#include "dxConsoleRender.h"

dxConsoleRender::dxConsoleRender() {}

void dxConsoleRender::Copy(IConsoleRender &_in)
{
	*this = *(dxConsoleRender *)&_in;
}

void dxConsoleRender::OnRender(bool bGame)
{
	VERIFY(HW.pDevice);

	D3DRECT R = {0, 0, Device.dwWidth, Device.dwHeight};
	if (bGame)
		R.y2 /= 2;

	CHK_DX(HW.pDevice->Clear(1, &R, D3DCLEAR_TARGET, D3DCOLOR_XRGB(32, 32, 32), 1, 0));
}
