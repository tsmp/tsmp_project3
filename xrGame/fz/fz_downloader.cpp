#include "stdAfx.h"
#include "..\xrserver.h"
#include "fz_sysmsgs_defines.h"
#include "..\Level.h"

int fz_downloader_enabled = 1;
std::string fz_downloader_mod_name = "tsmp";
std::string fz_downloader_reconnect_ip;

bool LoadedDLL = false;
HMODULE dll;

FZSysMsgsSendSysMessage_SOC ProcSendSysMessage;
FZSysMsgsProcessClientModDll ProcProcessClientMod;

static void __stdcall SendCallback(void *msg, unsigned int len, void *userdata)
{
	SMyUserData *data = (SMyUserData *)userdata;
	xrServer *srv = data->server;
	ClientID ID = data->idOfPlayer;

	srv->IPureServer::SendTo_LL(ID, msg, len, net_flags(TRUE, TRUE, TRUE, TRUE));
}

void LoadDLL()
{
	if (!LoadedDLL)
	{
		dll = LoadLibrary("sysmsgs.dll");

		if (dll)
		{
			FZSysMsgsInit SysInit;
			SysInit = (FZSysMsgsInit)GetProcAddress(dll, "FZSysMsgsInit");
			(*SysInit)();

			ProcSendSysMessage = (FZSysMsgsSendSysMessage_SOC)GetProcAddress(dll, "FZSysMsgsSendSysMessage_SOC");
			ProcProcessClientMod = (FZSysMsgsProcessClientModDll)GetProcAddress(dll, "FZSysMsgsProcessClientModDll");

			SetCommonSysmsgsFlags SetFlags;
			SetFlags = (SetCommonSysmsgsFlags)GetProcAddress(dll, "FZSysMsgsSetCommonSysmsgsFlags");
			SetFlags(FZ_SYSMSGS_ENABLE_LOGS | FZ_SYSMSGS_PATCH_UI_PROGRESSBAR);

			LoadedDLL = true;
			Msg("- sysmsgs.dll loaded successfully");
		}
		else
			Msg("! error, cant load dll with sysmsgs api");
	}
}

void UnloadSysmsgsDLL()
{
	if (LoadedDLL)
	{
		FZSysMsgsFree FreeFZ = (FZSysMsgsFree)GetProcAddress(dll, "FZSysMsgsFree");
		FreeFZ();

		FreeLibrary(dll);
	}

	LoadedDLL = false;
}

void DownloadingMod(xrServer *server, ClientID ID);

void fz_download_mod(xrServer *server, ClientID ID)
{
	if (fz_downloader_enabled)
	{
		LoadDLL();

		if (!LoadedDLL)
			return;

		Msg("- Sending magic packet to client");
		DownloadingMod(server, ID);
	}
}

void DownloadingMod(xrServer *server, ClientID ID)
{
	SMyUserData userdata = {};
	userdata.idOfPlayer = ID;
	userdata.server = server;

	FZDllDownloadInfo moddllinfo = {};

	moddllinfo.fileinfo.filename = "";
	moddllinfo.fileinfo.url = "";
	moddllinfo.fileinfo.crc32 = 0x274A4EBD;
	moddllinfo.fileinfo.progress_msg = "In Progress";
	moddllinfo.fileinfo.error_already_has_dl_msg = "Error happens";
	moddllinfo.fileinfo.compression = FZ_COMPRESSION_NO_COMPRESSION;
	moddllinfo.procname = "ModLoad";

	moddllinfo.procarg1 = fz_downloader_mod_name.c_str();

	ip_address Address;
	DWORD dwPort = 0;

	Level().GetServerAddress(Address, &dwPort);

	if (fz_downloader_reconnect_ip.empty())
		fz_downloader_reconnect_ip = Address.to_string().c_str();

	std::string procargs2 = "-srv " + fz_downloader_reconnect_ip + " -srvport " + std::to_string(dwPort);

	moddllinfo.procarg2 = procargs2.c_str(); //Аргументы для передачи в процедуру
	moddllinfo.dsign = "";
	moddllinfo.name_lock = "123";				//Цифровая подпись для загруженной DLL - проверяется перед тем, как передать управление в функцию мода
	moddllinfo.reconnect_addr.ip = "127.0.0.1"; //IP-адрес и порт для реконнекта. Если IP нулевой, то параметры реконнекта автоматически берутся игрой из тех, во время которых произошел дисконнект.
	moddllinfo.reconnect_addr.port = 5445;		// Порт

	ProcSendSysMessage(ProcProcessClientMod, &moddllinfo, SendCallback, &userdata);
}
