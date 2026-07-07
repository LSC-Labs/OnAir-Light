#ifndef DEBUG_LSC_RF433RECEIVER
    #undef DEBUGINFOS
#endif
#ifdef RADIO_433_RECEIVER_PIN
#include <RF433Receiver.h>
#include <Appl.h>
#include <JsonNode.h>
#include <AppMsgs.h>
#include <OnAirLight.h>


/// @brief Return true if the RF code is configured for dispatch.
bool CRF433Receiver::hasKey(unsigned long ulKey) {
    return(!(this->tMessagesToSend.find(ulKey) == this->tMessagesToSend.end()));
}
/// @brief Return whether RF reception is enabled in configuration.
bool CRF433Receiver::isEnabled() {
    return(this->Config.isEnabled);
}

/// @brief Look up the message bus event associated with a known RF code.
RF433Message CRF433Receiver::getMessage(unsigned long ulKey) {
    return(this->tMessagesToSend[ulKey]);
}

/// @brief Initialize the receiver pin and install default codes when no config was loaded.
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


void CRF433Receiver::writeConfigTo(CJsonNode &oCfg, bool bHideCritical){
    DEBUG_FUNC_START();
    oCfg["enabled"] = Config.isEnabled;
    // JsonArray tMessageList = CreateJsonArray(oCfg,"msgs");//  oCfg.createNestedArray("msgs");
    JsonNode * ptMessageList = oCfg.getArray("msgs",true);
    for(auto oEntry : this->tMessagesToSend) {
        // JsonObject oData = CreateEmptyJsonObject(tMessageList); //  tMessageList.createNestedObject();
        JsonNode *pData = ptMessageList->createObject();
        int nMsg = oEntry.second.MsgId - MSG_ONAIR_BASE;
        if(nMsg < 0 || nMsg > ONAIR_DEVICE_UPPER_LIMIT) nMsg = ONAIR_CAMERA;

        (*pData)["on"]      = oEntry.first;
        (*pData)["msg"]     = nMsg;
        (*pData)["type"]    = oEntry.second.MsgType;
    }
    DEBUG_JSON_OBJ(oCfg);
};

/// @brief read the configuration from the Json Object...
///        removes old entries, if "_oper = new"
/// @param oCfg 
void CRF433Receiver::readConfigFrom(CJsonNode &oCfg){
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    oCfg.storeValueIf("enabled",&Config.isEnabled);
    String strOper = oCfg.getValue("_oper");
    bool bDeleteExisting = !strOper.equalsIgnoreCase("upd");
    if(bDeleteExisting) this->tMessagesToSend.clear();
    // JsonArray tMessages = oCfg["msgs"];
    CJsonNode *ptMessages = oCfg.getArray("msgs");
    if(ptMessages) {
        // ToDo: Clear old rf433 infos, if in place - 
        // this module is waiting for the GUI implementation
        for(CJsonNode *pMsg : ptMessages->Elements) {
            int nConfigMsg = (*pMsg).getValueAsInt("msg");
            if(nConfigMsg < 0  || nConfigMsg > ONAIR_DEVICE_UPPER_LIMIT) {
                nConfigMsg = ONAIR_CAMERA;
            }
            unsigned long ulOn = pMsg->getValueAsUnsignedLong("on");
            int nMsgId         = (nConfigMsg + MSG_ONAIR_BASE);
            int nMsgType       = pMsg->getValueAsInt("type");
            this->addMessage(ulOn,nMsgId,nMsgType);
        }
    } 
    DEBUG_FUNC_END();
};


void CRF433Receiver::writeStatusTo(CJsonNode &oCfg, int nLevel) {
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
    unsigned long ulNow = millis();
    // Data available ?
    if(ulData != 0) {
        // same value as last time ?
        if(ulData == m_ulLastDataReceived) {
            // in timeout range ... no new value
            if((ulNow - m_ulLastDataReceivedTime) < ulTimeout) {
                ulData = 0;
            } else {
                m_ulLastDataReceivedTime = ulNow;
            }
        } else {
            // Reset received timestamp...
            this->m_ulLastDataReceived = ulData;
            this->m_ulLastDataReceivedTime = ulNow;
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
