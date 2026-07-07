#pragma once
#ifdef RADIO_433_RECEIVER_PIN
#include <JsonNode.h>
#include <RF433Receiver.h>
#include <RCSwitch.h>
#include <ConfigHandler.h>
#include <StatusHandler.h>
#include <map>

/**
 * @brief Configuration for the 433 MHz receiver module.
 */
struct RF433Config {
    bool   isEnabled = true;
    String OnMessage = "";
    String OffMessae = "";
};

/**
 * @brief Message bus event emitted when a configured RF code is received.
 */
struct RF433Message {
    int MsgId;     // Message ID to send,
    int MsgType;   // Message Type to send
};

/**
 * @brief RCSwitch based 433 MHz receiver with configurable code-to-event mapping.
 *
 * The receiver stores known RF codes and forwards matching codes to the
 * application message bus. It also filters short duplicate repeats that are
 * common with inexpensive RF remotes.
 */
class CRF433Receiver : public RCSwitch, public IConfigHandler, public IStatusHandler {
    private:
        bool         m_bConfigIsLoaded = false; // If Config is not loaded, setup will insert default Remotes
        const char * pszDeviceName;
        int m_nPin = -1;
        unsigned long m_ulLastDataReceived = 0L;
        unsigned long m_ulLastDataReceivedTime = 0L;
        RF433Config Config;
        std::map<unsigned long, RF433Message> tMessagesToSend;
       


    public:
        CRF433Receiver(int nPin, const char* pszName = "RF433") { m_nPin = nPin; pszDeviceName = pszName ? pszName : "RF433"; }
        void setup(int nPin = -1);
        bool isEnabled();
        bool hasKey(unsigned long);
        RF433Message getMessage(unsigned long);
        void writeConfigTo(CJsonNode &oCfg, bool bHideCritical) override;
        void readConfigFrom(CJsonNode &oCfg) override;
        void writeStatusTo(CJsonNode &oCfg,int nStatus = STATUS_LEVEL_INFO) override;
        // Use to ask for a new message
        unsigned long getReceivedValueOnce(unsigned long ulTimeOut = 20);
        // Or use addMessage and dispatchMessages() - in loop.
        void addMessage(unsigned long ulOnData, int nMsgID, int nMsgType);
        void dispatchMessages(unsigned long ulTimeOut = 20);

};

#endif
