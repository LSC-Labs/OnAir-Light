#pragma once
#include <Network.h>
#include <OnAirLight.h>

/**
 * @brief Runtime status shared between hardware input, RF scanning and the web UI.
 *
 * The object is registered on the application message bus and mirrors transient
 * application state that should be visible to status APIs or other modules.
 */
class  CStatus : public IMsgEventReceiver, public IStatusHandler {
    public:
        volatile bool isRebootPending = false;
        volatile bool isButtonPressed = false;
        volatile bool configLoaded    = false;
        /// @brief WebSocket client id that requested a RF433 scan (0 = no request).
        /// Only the id is kept - the client object may be deleted at any time (disconnect).
        uint32_t ScanRF433RequestorId = 0;


    public:
        virtual int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nMsgInfo);
        virtual void writeStatusTo(JsonNode & oStatusNode, int nLevel = STATUS_LEVEL_INFO);
};
