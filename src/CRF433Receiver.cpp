#ifndef DEBUG_LSC_RF433RECEIVER
    #undef DEBUGINFOS
#endif
#ifdef RADIO_433_RECEIVER_PIN
#include <RF433Receiver.h>
#include <Appl.h>
#include <LSCUtils.h>
#include <AppMsgs.h>
#include <OnAirLight.h>


bool CRF433Receiver::hasKey(unsigned long ulKey) {
    return(!(this->tMessagesToSend.find(ulKey) == this->tMessagesToSend.end()));
}
bool CRF433Receiver::isEnabled() {
    return(this->Config.isEnabled);
}

RF433Message CRF433Receiver::getMessage(unsigned long ulKey) {
    return(this->tMessagesToSend[ulKey]);
}

void CRF433Receiver::setup(int nPin) {
    DEBUG_FUNC_START();
    if(nPin > -1) m_nPin = nPin;
    if(m_nPin > -1 && Config.isEnabled) enableReceive(m_nPin);
    if(!this->m_bConfigIsLoaded) {
        DEBUG_INFO(" - adding default codes for RF433");
        // kein object "msgs" -> initialization status...
        // insert defaults...
        addMessage(0xFFFF02,MSG_ONAIR_BASE + ONAIR_CAMERA,ONAIR_DEVICE_ON);
        addMessage(0xFFFF08,MSG_ONAIR_BASE + ONAIR_CAMERA,ONAIR_DEVICE_OFF);
    }
    DEBUG_FUNC_END();
}


void CRF433Receiver::writeConfigTo(JsonObject &oCfg, bool bHideCritical){
    DEBUG_FUNC_START();
    oCfg["enabled"] = Config.isEnabled;
    JsonArray tMessageList = oCfg.createNestedArray("msgs");
    for(auto oEntry : this->tMessagesToSend) {
        JsonObject oData = tMessageList.createNestedObject();

        int nMsg = oEntry.second.MsgId - MSG_ONAIR_BASE;
        if(nMsg < 0 || nMsg > ONAIR_DEVICE_UPPER_LIMIT) nMsg = ONAIR_CAMERA;

        oData["on"]      = oEntry.first;
        oData["msg"]     = nMsg;
        oData["type"]    = oEntry.second.MsgType;
    }
    DEBUG_JSON_OBJ(oCfg);
};

/// @brief read the configuration from the Json Object...
///        removes old entries, if "_oper = new"
/// @param oCfg 
void CRF433Receiver::readConfigFrom(JsonObject &oCfg){
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    storeValueIF(&Config.isEnabled,oCfg["enabled"]);
    String strOper = oCfg["_oper"].as<String>();
    bool bDeleteExisting = !strOper.equalsIgnoreCase("upd");
    if(bDeleteExisting) this->tMessagesToSend.clear();
    JsonArray tMessages = oCfg["msgs"];
    if(tMessages) {
        // ToDo: Clear old rf433 infos, if in place - 
        // this module is waiting for the GUI implementation
        for(JsonObject oMsg : tMessages) {
            int nConfigMsg = oMsg["msg"];
            if(nConfigMsg < 0  || nConfigMsg > ONAIR_DEVICE_UPPER_LIMIT) {
                nConfigMsg = ONAIR_CAMERA;
            }
            unsigned long ulOn = oMsg["on"];
            int nMsgId         = (nConfigMsg + MSG_ONAIR_BASE);
            int nMsgType       = oMsg["type"];
            this->addMessage(ulOn,nMsgId,nMsgType);
        }
    } 
    DEBUG_FUNC_END();
};


void CRF433Receiver::writeStatusTo(JsonObject &oCfg) {
    oCfg["enabled"] = Config.isEnabled;
    // oCfg["avty"]    = available();
};

/// @brief Get the received value once...
///        Receives a value one time, and if it stays in place for a Timeout,
///        Reset the value to 0. So you avoid duplicate receives of values...
///        If the received data changes - you will get them
///        If the value stays in place (duplicate send) you receive only the first, then 0
///        Do NOT use resetAvailable() by your module...
/// @param ulTimeout timeout in milliseconds to avoid duplicate values
/// @return 0 - no or no new value / otherwise the received value...
unsigned long CRF433Receiver::getReceivedValueOnce(unsigned long ulTimeout) {
    unsigned long ulData = RCSwitch::getReceivedValue();
    // Data available ?
    if(ulData != 0) {
        // same value as last time ?
        if(ulData == m_ulLastDataReceived) {
            // in Timeout range ... no value...
            if(m_ulLastDataReceivedTime + ulTimeout < millis()) {
                ulData = 0;
            }
        } else {
            // Reset received timestamp...
            this->m_ulLastDataReceived = ulData;
            this->m_ulLastDataReceivedTime = millis();
        }
        resetAvailable();
    }
    return(ulData);
}

/// @brief Dispatch the messages
///        If a message is in place, check against the registered messages
///        and send the message via the message bus to the registered handlers. 
/// @param ulTimeout is used to avoid duplicate received messages... 
/// @see getReceivedValueOnce()
void CRF433Receiver::dispatchMessages(unsigned long ulTimeout) {
    unsigned long ulData = getReceivedValueOnce(ulTimeout);
    // Data available ?
    if(ulData != 0) { 
        if(hasKey(ulData)) {
            RF433Message oMsg = this->tMessagesToSend[ulData];
            Appl.MsgBus.sendEvent(this,oMsg.MsgId,pszDeviceName,oMsg.MsgType);
        }
    }
} 

/// @brief adds a message to the registered (known) message types
/// @param ulOnData this is the code that is received via RF433 Remote control
/// @param nMsgId the message id that will be sent, if ulOnData matches
/// @param nMsgType the message type that will be sent, if ulOnData matches.
void CRF433Receiver::addMessage(unsigned long ulOnData,int nMsgId, int nMsgType) {
    DEBUG_FUNC_START_PARMS("%lu,%d,%d",ulOnData,nMsgId,nMsgType);
    // auto oEntry = tMessagesToSend.find(ulOnData);
    RF433Message oMessage;
    oMessage.MsgId      = nMsgId;
    oMessage.MsgType    = nMsgType;
    this->tMessagesToSend[ulOnData] = oMessage;
}

#endif