#pragma once

//#define DEDICATED_SERVER // ���������� ������
#if defined(DEDICATED_BUILD) && !defined(DEDICATED_SERVER)
//�������� ���������� � ����� ������ 
#define DEDICATED_SERVER
#endif
//#define PUBLIC_BUILD // ��������� ������, ��������� ��������� ���������� �������
//#define MP_LOGGING // ����� � ��� ���������� ������� �������
//#define FZ_MOD_CLIENT // ���, ����������� ������ � ������� fz
//#define NO_SINGLE // ��������� ��������� �����
//#define NO_MULTI_INSTANCES // ��������� ��������� ��������� ����������� ����
//#define EVERYBODY_IS_ENEMY // ��� ��� ������� ���� �������
//#define HUGE_OGF_FIX // �� ������� ������� ��������, ������� ������ ������� skin.h
#define NEW_RANKS // ����� �����: 8 ������ 5
#define TSMP_VERSION "TSMP 03.11.2023"

#ifdef PUBLIC_BUILD
#define SEND_ERROR_REPORTS // ���������� ������ � �������
#define USE_PLAYERS_STATS_SERVICE // ������������������ � ����� �������, �������� ����������
#endif
