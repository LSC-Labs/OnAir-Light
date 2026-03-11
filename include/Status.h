#pragma once
#include <Network.h>
#include <OnAirLight.h>

class  CStatus : public IMsgEventReceiver, public IStatusHandler {
    public:
        volatile bool isRebootPending = false;
        volatile bool isButtonPressed = false;
        volatile bool configLoaded    = false;
        AsyncWebSocketClient * pScanRF433Requestor;


    public:
        virtual int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nMsgInfo);
    
        virtual void writeStatusTo(JsonObject & oStatusNode);
};

