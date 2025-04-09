#ifndef DEBUG_LSC_ONAIRLIGHT
    #undef DEBUGINFOS
#endif
#include <Status.h>
#include <AppMsgs.h>
#include <DevelopmentHelper.h>


int CStatus::receiveEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) {
    switch(nMsgType) {
        case MSG_REBOOT_REQUEST: isRebootPending = true; break;
        case MSG_BUTTON_ON     : isButtonPressed = true; DEBUG_INFO("--- button ON"); break;
        case MSG_BUTTON_OFF    : isButtonPressed = false; DEBUG_INFO("--- button OFF");break;
        // case MSG_SET_RESTART_WIFI  : restartWiFi     = nClass == WIFI_RESTART_OFF ? false : true; break;
        case MSG_SCAN_RF433    : {
            DEBUG_INFO("--> STATUS message for SCAN RF433 received...");
            pScanRF433Requestor = (AsyncWebSocketClient *) pMessage;
        }
    };
    return(EVENT_MSG_RESULT_OK);
}

void CStatus::writeStatusTo(JsonObject &oStatusNode) {
    oStatusNode["rebootPending"] = isRebootPending;
    oStatusNode["ButtonPressed"] = isButtonPressed   ? "1" : "0";
    // oStatusNode["RestartWiFi"]   = restartWiFi       ? "1" : "0";
    #ifdef DEBUGINFOS
        oStatusNode["DebugMode"]     = "1";
    #else
        oStatusNode["DebugMode"]     = "0";
    #endif
}
