#pragma once
#ifdef RADIO_433_RECEIVER_PIN
#include <ArduinoJson.h>
#include <RF433Receiver.h>
#include <RCSwitch.h>
#include <ConfigHandler.h>
#include <StatusHandler.h>
#include <map>

struct RF433Config {
    bool   isEnabled = true;
    String OnMessage = "";
    String OffMessae = "";
};

struct RF433Message {
    int MsgId;     // Message ID to send,
    int MsgType;   // Message Type to send
};

class CRF433Receiver : public RCSwitch, public IConfigHandler, public IStatusHandler {
    private:
        bool         m_bConfigIsLoaded = false; // If Config is not loaded, setup will insert default Remotes
        const char * pszDeviceName;
        int m_nPin = -1;
        unsigned long m_ulLastDataReceived;
        unsigned long m_ulLastDataReceivedTime = 0L;
        RF433Config Config;
        std::map<unsigned long, RF433Message> tMessagesToSend;
       


    public:
        CRF433Receiver(int nPin, const char* pszName = "RF433") { m_nPin = nPin; pszDeviceName = pszName ? pszName : "RF433"; }
        void setup(int nPin = -1);
        bool isEnabled();
        bool hasKey(unsigned long);
        RF433Message getMessage(unsigned long);
        void writeConfigTo(JsonObject &oCfg, bool bHideCritical) override;
        void readConfigFrom(JsonObject &oCfg) override;
        void writeStatusTo(JsonObject &oCfg) override;
        // Use to ask for a new message
        unsigned long getReceivedValueOnce(unsigned long ulTimeOut = 20);
        // Or use addMessage and dispatchMessages() - in loop.
        void addMessage(unsigned long ulOnData, int nMsgID, int nMsgType);
        void dispatchMessages(unsigned long ulTimeOut = 20);

};

#endif