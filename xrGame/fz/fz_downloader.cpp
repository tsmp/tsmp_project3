#include "stdAfx.h"
#include "..\xrserver.h"
#include "fz_sysmsgs_defines.h"
#include "..\Level.h"
#include "..\xrGameSpyServer.h"

int fz_downloader_new = 0;
int fz_downloader_enabled = 1;
int fz_downloader_skip_full_check = 0;
int fz_downloader_allow_x64 = 0;
int fz_downloader_previous_version = 0;

std::string fz_downloader_mod_name = "tsmp";
std::string fz_downloader_reconnect_ip;
std::string fz_downloader_message = "Preparing mod";

bool LoadedDLL = false;
HMODULE dll;

FZSysMsgsSendSysMessage_SOC ProcSendSysMessage;
FZSysMsgsProcessClientModDll ProcProcessClientMod;

static void __stdcall SendCallback(void *msg, unsigned int len, void *userdata)
{
	const SMyUserData *data = static_cast<SMyUserData*>(userdata);
	xrServer *srv = data->server;
	const ClientID &ID = data->idOfPlayer;

	srv->IPureServer::SendTo_LL(ID, msg, len, net_flags(TRUE, TRUE, TRUE, TRUE));
}

void LoadDLL()
{
	if (LoadedDLL)
		return;

	dll = LoadLibrary("sysmsgs.dll");

	if (!dll)
	{
		Msg("! error, cant load dll with sysmsgs api");
		return;
	}

	const auto SysInit = reinterpret_cast<FZSysMsgsInit>(GetProcAddress(dll, "FZSysMsgsInit"));
	SysInit();

	ProcSendSysMessage = reinterpret_cast<FZSysMsgsSendSysMessage_SOC>(GetProcAddress(dll, "FZSysMsgsSendSysMessage_SOC"));
	ProcProcessClientMod = reinterpret_cast<FZSysMsgsProcessClientModDll>(GetProcAddress(dll, "FZSysMsgsProcessClientModDll"));

	const auto SetFlags = reinterpret_cast<SetCommonSysmsgsFlags>(GetProcAddress(dll, "FZSysMsgsSetCommonSysmsgsFlags"));
	SetFlags(FZ_SYSMSGS_ENABLE_LOGS | FZ_SYSMSGS_PATCH_UI_PROGRESSBAR);

	LoadedDLL = true;
	Msg("- sysmsgs.dll loaded successfully");
}

void UnloadSysmsgsDLL()
{
#pragma TODO("TSMP: Разобраться что тут вылетает при выгрузке FZ загрузчика!")
	// Код вроде правильный но чет вылетает
	//if (LoadedDLL)
	//{
	//	FZSysMsgsFree FreeFZ = (FZSysMsgsFree)GetProcAddress(dll, "FZSysMsgsFree");
	//	FreeFZ();

	//	FreeLibrary(dll);
	//}

	LoadedDLL = false;
}

void DownloadingMod(xrServer *server, ClientID const &ID)
{
	SMyUserData userdata = {};
	userdata.idOfPlayer = ID;
	userdata.server = server;

	ip_address Address;
	DWORD dwPort = 0;
	Level().GetServerAddress(Address, &dwPort);
	std::string ModLoadArgs = "-srv " + fz_downloader_reconnect_ip + " -srvport " + std::to_string(dwPort)+" -gamespymode ";

	FZDllDownloadInfo moddllinfo = {};

	if (fz_downloader_new)
	{
		if (fz_downloader_previous_version)
		{
			moddllinfo.fileinfo.filename = "fz_mod_loader_tsmp_v2.mod";
			moddllinfo.fileinfo.url = "http://stalker-life.com/stalker_files/mods_shoc/tsmp3/loader/tsmp_mod_loader_v2.dll";
			moddllinfo.fileinfo.crc32 = 0xB2D51956; // crc дллки

			//Цифровая подпись для загруженной DLL - проверяется перед тем, как передать управление в функцию мода
			moddllinfo.dsign = "302D021450268FA62C6B30BCA1DE8E3586BA1ED6749CD1890215008B30D01EB47529A9E5F9D49CE3CA56E84F3AD09F";
		}
		else
		{
			moddllinfo.fileinfo.filename = "fz_mod_loader_tsmp_v3.mod";
			moddllinfo.fileinfo.url = "http://stalker-life.com/stalker_files/mods_shoc/tsmp3/loader/tsmp_mod_loader_v3.dll";
			moddllinfo.fileinfo.crc32 = 0xACB62B2F; // crc дллки

			//Цифровая подпись для загруженной DLL - проверяется перед тем, как передать управление в функцию мода
			moddllinfo.dsign = "302D0215008868F530DFBB61F92B1AA4FBED0C84019E04706302142E09A800FB3E225A5BAE7431788A5700CAC2F94D";
		}

		const shared_str srvPasword = static_cast<xrGameSpyServer*>(server)->Password;

		if (srvPasword.size())
			ModLoadArgs += " -psw " + std::string(srvPasword.c_str());

		if (fz_downloader_skip_full_check)
			ModLoadArgs += " -skipfullcheck ";

		if (fz_downloader_allow_x64)
			ModLoadArgs += " -allow64bitengine ";

		ModLoadArgs += " -includename -preservemessage ";
	}
	else
	{
		moddllinfo.fileinfo.filename = "";
		moddllinfo.fileinfo.url = "";
		moddllinfo.fileinfo.crc32 = 0x274A4EBD;
		moddllinfo.dsign = "";
	}

	moddllinfo.mod_is_applying_message = fz_downloader_message.c_str();
	moddllinfo.fileinfo.compression = FZ_COMPRESSION_NO_COMPRESSION;
	moddllinfo.procname = "ModLoad";
	moddllinfo.modding_policy = FZ_MODDING_WHEN_NOT_CONNECTING;
	moddllinfo.procarg1 = fz_downloader_mod_name.c_str();

	if (fz_downloader_reconnect_ip.empty())
		fz_downloader_reconnect_ip = Address.to_string();

	moddllinfo.procarg2 = ModLoadArgs.c_str(); //Аргументы для передачи в процедуру
	
	// not used
	moddllinfo.name_lock = "123";				
	moddllinfo.reconnect_addr.ip = "127.0.0.1";
	moddllinfo.reconnect_addr.port = 5445;
	moddllinfo.incompatible_mod_message = "Incompatible";
	moddllinfo.fileinfo.progress_msg = fz_downloader_message.c_str();
	moddllinfo.fileinfo.error_already_has_dl_msg = "Error happens";

	ProcSendSysMessage(ProcProcessClientMod, &moddllinfo, SendCallback, &userdata);
}

void fz_download_mod(xrServer* server, ClientID const& ID)
{
	if (!fz_downloader_enabled)
		return;

	LoadDLL();

	if (!LoadedDLL)
		return;

	Msg("- Sending magic packet to client");
	DownloadingMod(server, ID);
}
