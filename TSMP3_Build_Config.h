#pragma once

#define ALIFE_MP
//#define DEDICATED_SERVER // Выделенный сервер
#if defined(DEDICATED_BUILD) && !defined(DEDICATED_SERVER)
//Собираем выделенный в любом случае 
#define DEDICATED_SERVER
#endif
//#define PUBLIC_BUILD // Публичная сборка, запрещены читерские отладочные команды
//#define MP_LOGGING // Вывод в лог внутренних событий сервера
//#define FZ_MOD_CLIENT // Мод, скачиваемый игроку с помощью fz
//#define NO_SINGLE // Запретить запускать сингл
//#define NO_MULTI_INSTANCES // Запретить запускать несколько экземпляров игры
//#define EVERYBODY_IS_ENEMY // Все нпс считают всех врагами
#define TSMP_VERSION "R14_test"

#ifdef PUBLIC_BUILD
#define SEND_ERROR_REPORTS // Отправлять отчеты о вылетах
#endif
