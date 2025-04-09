#pragma once

#define MSG_LOG_ENTRY          1001
#define MSG_LOG_ENTRY_JSON     1002

#define MSG_BUTTON_CHANGED     1020
#define MSG_BUTTON_ON          1021
#define MSG_BUTTON_OFF         1022

#define MSG_MQTT_MSG_RECEIVED  7001

#define MSG_REBOOT_REQUEST     8000
#define MSG_APPL_INITIALIZED   9000
#define MSG_APPL_SHUTDOWN      9001




const int MSG_USER_BASE = 10000;

#define LOG_CLASS_INFO       1   // [I]
#define LOG_CLASS_VERBOSE    2
#define LOG_CLASS_WARN       3
#define LOG_CLASS_ERROR      6
#define LOG_CLASS_TRACE      7
#define LOG_CLASS_DEBUG      8
#define LOG_CLASS_EXCEPTION  9

// Definition of the Keys in a string for quick access...
// Index is defined Level
#define LOG_CLASS_LEVEL_MASK ".IVW..ETDX"       
