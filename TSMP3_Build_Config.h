#pragma once

//#define DEDICATED_SERVER // Выделенный сервер
#if defined(DEDICATED_BUILD) && !defined(DEDICATED_SERVER)
//Собираем выделенный в любом случае 
#define DEDICATED_SERVER
#endif
#define PUBLIC_BUILD // Публичная сборка, запрещены читерские отладочные команды
//#define MP_LOGGING // Вывод в лог внутренних событий сервера
#define FZ_MOD_CLIENT // Мод, скачиваемый игроку с помощью fz
//#define NO_SINGLE // Запретить запускать сингл
//#define NO_MULTI_INSTANCES // Запретить запускать несколько экземпляров игры
//#define EVERYBODY_IS_ENEMY // Все нпс считают всех врагами
//#define HUGE_OGF_FIX // Не корёжить большие модельки, требует правку шейдера skin.h
#define TSMP_VERSION "R17_test"

#ifdef PUBLIC_BUILD
//#define SEND_ERROR_REPORTS // Отправлять отчеты о вылетах
#define USE_PLAYERS_STATS_SERVICE // Синхронизироваться с базой игроков, собирать статистику
#endif
