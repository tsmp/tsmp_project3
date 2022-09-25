#include "pch_xrengine.h"
#include "resource.h"
#include "..\xrCustomRes\resource.h"

HMODULE hCustomRes = nullptr;
const u32 bufferSize = 128;
char textBuf[bufferSize];

void TryLoadXrCustomResDll()
{
	hCustomRes = LoadLibraryA("xrCustomRes.dll");
}

// меняем логотип на картинку, загруженную из xrCustomRes.dll
void TryToChangeLogoImageToCustom(HWND logoWindow)
{
	if (hCustomRes)
	{		
		HWND hStatic = GetDlgItem(logoWindow, IDC_STATIC_PICTURE);
		HBITMAP hBmp = LoadBitmap(hCustomRes, MAKEINTRESOURCE(IDB_LOGO_BITMAP));
		SendMessage(hStatic, STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), (LPARAM)hBmp);
	}
}

const char *TryToGetNewWindowText()
{
	if (!hCustomRes)
		return nullptr;

	LoadString(hCustomRes, IDS_STRING_APPNAME, textBuf, bufferSize);
	return textBuf;
}
