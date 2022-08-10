#pragma once

#include "windows.h"

struct SMyUserData
{
    xrServer *server;
    ClientID idOfPlayer;
};

#pragma pack(push, 1)

using FZSysmsgPayloadWriter = void *;
using FZSysMsgSender = void *;
using FZSysMsgsProcessClientModDll = void *;
using FZSysMsgsProcessClientMap = void *;

typedef bool(__stdcall *FZSysMsgsInit)();
typedef bool(__stdcall *FZSysMsgsFlags)();
typedef bool(__stdcall *FZSysMsgsFree)();

typedef void(__stdcall *FZSysMsgsSendSysMessage_SOC)(void *, void *, FZSysMsgSender, void *);

typedef int FZSysmsgsCommonFlags;
const FZSysmsgsCommonFlags FZ_SYSMSGS_ENABLE_LOGS = 1;
const FZSysmsgsCommonFlags FZ_SYSMSGS_PATCH_UI_PROGRESSBAR = 2;

typedef void(__stdcall *SetCommonSysmsgsFlags)(FZSysmsgsCommonFlags);

//Тип компрессии, использованный в находящемся на сервере файле
typedef DWORD FZArchiveCompressionType;
const FZArchiveCompressionType FZ_COMPRESSION_NO_COMPRESSION = 0;  //Компрессии нет, файл на сервере не сжат
const FZArchiveCompressionType FZ_COMPRESSION_LZO_COMPRESSION = 1; //Файл сжат внутриигровым LZO-компрессором
const FZArchiveCompressionType FZ_COMPRESSION_CAB_COMPRESSION = 2; //Файл сжат стандартной утилитой MAKECAB из состава Windows

//Определяет момент, в который должна вызываться функция мода
typedef DWORD FZModdingPolicy;

//Функция из DLL мода может вызываться в любое время. Автоматический реконнект не производится.
const FZModdingPolicy FZ_MODDING_ANYTIME = 0;

//Функция из DLL мода должна вызываться только в процессе коннекта к серверу (т.е. при необходимости автоматически инициируется реконнект после скачки)
//Удобно для модов, не требующих рестарта клиента
const FZModdingPolicy FZ_MODDING_WHEN_CONNECTING = 1;

//Функция из DLL мода должна вызываться только когда клиент не в состоянии коннекта (т.е. если файл на клиенте уже есть - все равно будет произведен дисконнект)
//Автоматического реконнекта не производится, мод сам отвечает за реконнект в случае необходимости такового
//Имейте в виду: в случае скачки файла игрок может уйти на другой сервер! В этом случае автоматического дисконнекта с другого сервера произведено не будет!
const FZModdingPolicy FZ_MODDING_WHEN_NOT_CONNECTING = 2;

//Параметры загружаемого файла
struct FZFileDownloadInfo
{
    //Имя файла (вместе с путем) на клиенте, по которому он должен быть сохранен
    char *filename;
    //URL, с которого будет производиться загрузка файла
    char *url;
    //Контрольная сумма CRC32 для файла (в распакованном виде)
    DWORD crc32;
    //Используемый тип компрессии
    FZArchiveCompressionType compression;
    //Сообщение, выводимое пользователю во время закачки
    const char *progress_msg;
    //Сообщение, выводимое пользователю при возникновении ошибки во время закачки
    char *error_already_has_dl_msg;
};

//Параметры реконнекта клиента к серверу
struct FZReconnectInetAddrData
{
    //IPv4-адрес сервера (например, 127.0.0.1)
    char *ip;
    //Порт сервера
    DWORD port;
};

typedef DWORD FZMapLoadingFlags;
const FZMapLoadingFlags FZ_MAPLOAD_MANDATORY_RECONNECT = 1; //Обязательный реконнект после успешной подгрузки скачанной карты

//Параметры загрузки клиентом db-архива с картой
struct FZMapInfo
{
    //Параметры файла
    FZFileDownloadInfo fileinfo;
    //IP-адрес и порт для реконнекта после завершения закачки. Если IP пустой, то параметры реконнекта автоматически берутся игрой из тех, во время которых произошел дисконнект.
    FZReconnectInetAddrData reconnect_addr;
    //Внутриигровое имя загружаемой карты (например, mp_pool)
    char *mapname;
    //Версия загружаемой карты (обычно 1.0)
    char *mapver;
    //Название xml-файла с локализованными названием и описанием карты (nil, если такое не требуется)
    char *xmlname;
    //флаги для настройки особенностей применения карты
    FZMapLoadingFlags flags;
};

typedef DWORD FZDllModFunResult;
const FZDllModFunResult FZ_DLL_MOD_FUN_SUCCESS_LOCK = 0;   //Мод успешно загрузился, требуется залочить клиента по name_lock
const FZDllModFunResult FZ_DLL_MOD_FUN_SUCCESS_NOLOCK = 1; //Успех, лочить клиента (с использованием name_lock) пока не надо
const FZDllModFunResult FZ_DLL_MOD_FUN_FAILURE = 2;        //Ошибка загрузки мода

typedef FZDllModFunResult(__stdcall *FZDllModFun)(char *procarg1, char *procarg2);

//Параметры загрузки клиентом DLL-мода посредством ProcessClientModDll
struct FZDllDownloadInfo
{
    //Параметры файла для dll мода
    FZFileDownloadInfo fileinfo;

    //Имя процедуры в dll мода, которая должна быть вызвана; должна иметь тип FZDllModFun
    char *procname;

    //Аргументы для передачи в процедуру
    const char *procarg1;
    const char *procarg2;

    //Цифровая подпись для загруженной DLL - проверяется перед тем, как передать управление в функцию мода
    char *dsign;

    //IP-адрес и порт для реконнекта. Если IP нулевой, то параметры реконнекта автоматически берутся игрой из тех, во время которых произошел дисконнект.
    FZReconnectInetAddrData reconnect_addr;

    //Параметры времени активации мода
    FZModdingPolicy modding_policy;

    //Значение для проверки в параметре -fzmod.
    //Если аргумент nil - ничего не проверяется.
    //Если параметр совпадает с таковым в командной строке - мод считается уже установленным, коннект продолжается
    //Если параметр не совпадает с указанным в командной строке - происходит дисконнект игрока
    //Если в командной строке параметра нет - происходит установка мода.
    char *name_lock;

    //Строка, выводимая при обнаружении несовместимого мода
    char *incompatible_mod_message;

    //Строка, выводимая при применении мода после дисконнекта (если активен FZ_MODDING_WHEN_NOT_CONNECTING)
    char *mod_is_applying_message;
};

//Параметры отдельной карты для добавления в список голосования
struct FZClientVotingElement
{
    //Внутриигровое имя карты (например, mp_pool). В случае если nil - производится очистка списка карт!
    //Целесообразно передавать nil первым элементом в пакете
    char *mapname;
    //Версия карты
    char *mapver;
    //Локализованное название карты; если nil, будет использован результат вызова стандартного внутриигрового транслятора на клиенте
    char *description;
};

//Параметры добавления карт в список, доступных для голосования, используется с ProcessClientVotingMaplist
struct FZClientVotingMapList
{
    //Указатель на массив из FZClientVotingElement. Каждый элемент массива содержит параметры одной карты,
    //которую требуется добавить в список карт, доступных для голосования
    FZClientVotingElement *maps;

    //Число элементов в массиве maps
    DWORD count;

    //Идентификатор типа игры, для которого требуется изменить список карт. В движке текущий тип игры содержится в в game_GameState.m_type
    DWORD gametype;

    //Выходной параметр, показывает, сколько карт из массива было отправлено клиенту, отсчет идет с начала массива.
    DWORD was_sent;
};

#pragma pack(pop)
