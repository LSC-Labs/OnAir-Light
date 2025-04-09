#pragma once


#include <Msgs.h>

#define WIFI_MODE_OFF     0
#define WIFI_MODE_ON      1


#define WIFI_STATION_MODE       1
#define WIFI_ACCESS_POINT_MODE  0

#define MSG_STARTING_WIFI           (MSG_USER_BASE + 10)
#define MSG_WIFI_CONNECTED          (MSG_USER_BASE + 11)
#define MSG_WIFI_ERROR              (MSG_USER_BASE + 12)

#define MSG_DISABLE_WIFI            (MSG_USER_BASE + 14)
#define MSG_SET_RESTART_WIFI        (MSG_USER_BASE + 15)

#define MSG_ONAIR_BASE              (MSG_USER_BASE + 100)

#define MSG_SCAN_RF433              (MSG_USER_BASE + 200)
