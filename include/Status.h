#pragma once
#include <WiFiController.h>
#include <OnAirLight.h>

class  CStatus : public IMsgEventReceiver, public IStatusHandler {
    public:
        volatile bool isRebootPending = false;
        volatile bool isButtonPressed = false;
        volatile bool configLoaded    = false;
        AsyncWebSocketClient *pScanRF433Requestor;


    public:
        int receiveEvent(void *pSender, int nMsgType, const void *pMessage, int nClass);
        void writeStatusTo(JsonObject &oStatusNode);
};

