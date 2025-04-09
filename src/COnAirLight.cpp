#ifndef DEBUG_LSC_ONAIRLIGHT
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <OnAirLight.h>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>

#pragma region OnAirLightStatus helper

/// @brief Set the state of mic or cam, only if a valid device and mode is addressed.
/// @param pszDevice either "audio", "mic", "video" or "cam". "media" -> all devices
/// @param pszMode "on", "true", "+", "1" => switches on. All other switches off.
void OnAirLightStatus::setState(const char *pszDevice, const char *pszMode) {
    DEBUG_FUNC_START_PARMS("%s,%s",NULL_POINTER_STRING(pszDevice),NULL_POINTER_STRING(pszMode));
    if(pszDevice && pszMode) {
        String strName = pszDevice;
        String strValue = pszMode;
        int nDevState = ONAIR_DEVICE_OFF;
        if(strValue.equals("on") || isTrueValue(pszMode)) nDevState = ONAIR_DEVICE_ON;
        if(strName.equalsIgnoreCase("audio") || strName.equalsIgnoreCase("mic") )       { 
            DEBUG_INFOS(" - setting isMicOn to : %d",nDevState);
            isMicOn = nDevState;
            ulLastUpdate = millis(); 
        }     
        else if(strName.equalsIgnoreCase("video") || strName.equalsIgnoreCase("cam"))  { 
            DEBUG_INFOS(" - setting isCamOn to : %d",nDevState);
            isCamOn = nDevState; 
            ulLastUpdate = millis();
        } else if(strName.equalsIgnoreCase("media")) {
            DEBUG_INFOS(" - setting all media to : %d",nDevState);
            isCamOn = nDevState;
            isMicOn = nDevState;
            ulLastUpdate = millis();
        } else {
            DEBUG_INFOS(" - unknown device : %s",pszDevice);
        }
    }
}
bool OnAirLightStatus::isTimeOutReached(unsigned long ulTimeOutMillis) {
    bool bResult = false;
    if(ulTimeOutMillis > 0 && ulLastUpdate > 0) {
        bResult = (ulLastUpdate + ulTimeOutMillis) < millis();
    }
    return(bResult);
}
#pragma endregion

#pragma region Constructor - Config and Status Interface

COnAirLight::COnAirLight(int nPin) : CLightSwitch(nPin) {
    this->setBrightness(ONAIR_LIGHT_BRIGHTNESS_DEFAULT);
}

void COnAirLight::readConfigFrom(JsonObject &oCfg) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    Config.OnCamMode = getModeByName(oCfg["oncam"],Config.OnCamMode);
    Config.OnMicMode = getModeByName(oCfg["onmic"],Config.OnMicMode);
    String strPrio = oCfg["priority"];
    if(strPrio) {
        if(strPrio == "mic") Config.Priority = ONAIR_MICRO;
        if(strPrio == "cam") Config.Priority = ONAIR_CAMERA;
    }
    unsigned long ulBrightness = oCfg["brightness"];
    // If not available, use default.
    if(ulBrightness < 10 || ulBrightness > 100) ulBrightness = ONAIR_LIGHT_BRIGHTNESS_DEFAULT;
    this->setBrightness(ulBrightness);
    DEBUG_INFOS(" --- timeout act is: %lu",Config.TimeOutMillis);
    // unsigned long ulTimeOut = oCfg["timeout"];
    // Config.TimeOutMillis = storeValueIF(&Config.TimeOutMillis,oCfg["timeout"].as<unsigned long>());
    DEBUG_INFOS(" --- timeout set to: %lu",Config.TimeOutMillis);
    DEBUG_FUNC_END();
}

void COnAirLight::writeConfigTo(JsonObject &oCfg, bool bHideCritical) {
    DEBUG_FUNC_START();
    setNameOfMode(oCfg,"oncam",Config.OnCamMode);
    setNameOfMode(oCfg,"onmic",Config.OnMicMode);
    oCfg["priority"]    = Config.Priority == ONAIR_MICRO ? "mic" : "cam";
    oCfg["brightness"]   = this->getBrightness();
    oCfg["timeout"]     = Config.TimeOutMillis / 1000;
    DEBUG_FUNC_END();
}

void COnAirLight::writeStatusTo(JsonObject &oCfg) {
    oCfg["isMicOn"] = isMicOn();
    oCfg["isCamOn"] = isCamOn();
    oCfg["timeout"] = Config.TimeOutMillis / 1000;
    JsonArray oArray = oCfg.createNestedArray("clients");
    for(auto oClient : tClientStaties) {
        JsonObject oData = oArray.createNestedObject();
        oData["client"] = oClient.first;
        oData["isCamOn"] = oClient.second.isCamOn;
        oData["isMicOn"] = oClient.second.isMicOn;
        oData["lastUpd"] = oClient.second.ulLastUpdate;
    }
}

#pragma endregion

int COnAirLight::getModeByName(String strMode, int nDefault) {
    int nResult = nDefault;
    if(strMode) {
        if(strMode      == F("blink")) nResult = ONAIR_LIGHT_MODE_BLINK;
        else if(strMode == F("wave") ) nResult = ONAIR_LIGHT_MODE_WAVE;
        else if(strMode == F("on")   ) nResult = ONAIR_LIGHT_MODE_ON;
        else if(strMode == F("off")  ) nResult = ONAIR_LIGHT_MODE_OFF;
    }
    return(nResult);
}

String COnAirLight::setNameOfMode(JsonObject &oCfg, const char *strKey, int nMode) {
    String strMode;
    switch(nMode) {
        case ONAIR_LIGHT_MODE_BLINK : strMode = F("blink"); break;
        case ONAIR_LIGHT_MODE_WAVE  : strMode = F("wave");   break;
        case ONAIR_LIGHT_MODE_ON    : strMode = F("on");     break;
        default                     : strMode = F("off");    break;
    }
    oCfg[strKey] = strMode;
    return(strMode);
}

/**
 * check if a camera is on.
 * Iterates through all client infos
 */
bool COnAirLight::isCamOn() {
    bool bIsOn = false;
    for(auto oEntry : tClientStaties) { 
        if(oEntry.second.isCamOn) {
            bool bTimeoutReached = oEntry.second.isTimeOutReached(Config.TimeOutMillis);
            bIsOn = bTimeoutReached ? false : true;
        } 
    }
    return(bIsOn);
}

/**
 * check if a microphone is on.
 * Iterates through all client infos
 */
bool COnAirLight::isMicOn() {
    bool bIsOn = false;
    for(auto oEntry : tClientStaties) { 
        if(oEntry.second.isMicOn) {
            bool bTimeoutReached = oEntry.second.isTimeOutReached(Config.TimeOutMillis);
            bIsOn = bTimeoutReached ? false : true;
        }
    }
    return(bIsOn);
}


void COnAirLight::updateLightStatus() {
    // DEBUG_FUNC_START();
    int nMode = ONAIR_LIGHT_MODE_OFF;
    bool bIsCamOn = this->isCamOn();
    bool bIsMicOn = this->isMicOn();
    if(bIsCamOn && bIsMicOn) {
        nMode = Config.Priority == ONAIR_CAMERA ? Config.OnCamMode : Config.OnMicMode;
    } else {
        if(bIsCamOn) {
            nMode = Config.OnCamMode;
        }
        if(bIsMicOn) {
            nMode = Config.OnMicMode;
        }
    } 
    switch(nMode) {
        case ONAIR_LIGHT_MODE_ON    : switchOn();  break;
        case ONAIR_LIGHT_MODE_BLINK : blink();     break;
        case ONAIR_LIGHT_MODE_WAVE  : 
            wave(_ulWaveFadeIn,_ulWaveFadeOut,_ulWaveOnTime,_ulWaveOffTime);      
            break;
        default: switchOff();
    }
    // DEBUG_INFOS(" - isMicOn(%d) isCamOn(%d) -> Mode %d (CamMode %d) (MicMode %d)",bIsMicOn, bIsCamOn, nMode,Config.OnCamMode,Config.OnMicMode);
    // DEBUG_DELAY(1000);
}
 


void COnAirLight::dispatchBrokerMessage(const char *pszMessage, int nLen) {
    DEBUG_FUNC_START_PARMS("\"%s\"",pszMessage);
    if(pszMessage && nLen > 10) {
        DynamicJsonDocument oMsgDoc(nLen * 4);
        DeserializationError rc = deserializeJson(oMsgDoc,pszMessage);
        if(rc == DeserializationError::Ok) {
            JsonObject oMsg = GetJsonDocumentAsObject(oMsgDoc);
            String strClient = oMsg.containsKey("client") ? (const char*) oMsg["client"] : "-anonymous-";
            if(oMsg.containsKey("mic"))     setClientStatus(strClient, "audio",  oMsg["mic"]);
            if(oMsg.containsKey("audio"))   setClientStatus(strClient, "audio",  oMsg["audio"]);
            if(oMsg.containsKey("cam"))     setClientStatus(strClient, "video",  oMsg["cam"]);
            if(oMsg.containsKey("video"))   setClientStatus(strClient, "video",  oMsg["video"]);
            if(oMsg.containsKey("media"))   {
                setClientStatus(strClient, "audio",oMsg["media"]);
                setClientStatus(strClient, "video",oMsg["media"]);
            }
        } else {
            DEBUG_INFOS("Json deserialization error %s",rc.c_str());
        }
    }
}

inline void COnAirLight::setClientStatus(String strClientAddress, const char *pszName, const char *pszValue) {
    DEBUG_FUNC_START_PARMS("'%s',%s,%s",strClientAddress.c_str(),NULL_POINTER_STRING(pszName),NULL_POINTER_STRING(pszValue));
    OnAirLightStatus *pStatus = &tClientStaties[strClientAddress]; //  getClientStatus(strClientAddress);
    pStatus->setState(pszName,pszValue);
    /*
    String strData1 = "H";
    oStatus = getClientStatus(strData1);
    oStatus.isCamOn = true;
    DEBUG_INFOS("S1 : %lu %d",&oStatus,oStatus.isCamOn );

    String strData2 = "H";
    oStatus = getClientStatus(strData2);
    DEBUG_INFOS("S2 : %lu %d",&oStatus,oStatus.isCamOn );

    String strData3 = "H";
    DEBUG_INFOS("S3 : %lu %d",&tClientStaties[strData3],tClientStaties[strData3].isCamOn );
    */
    

}

/// @brief Set the mode to on / off 
/// @param pSender sender of the message
/// @param nMsgId message, based on MSG_ONAIR_BASE
/// @param pMessage - currently not respected - future will be the hostname with open/closed devices
/// @param nType  - ONAIR_DEVICE_ON or ONAIR_DEVICE_OFF
/// @return 
int COnAirLight::receiveEvent(void *pSender, int nMsgId, const void *pMessage, int nType) {
    DEBUG_FUNC_START_PARMS("%d,%d",nMsgId,nType);
    switch(nMsgId) {
        case MSG_MQTT_MSG_RECEIVED : // Message Broker Message in pMessage, size in nType
            dispatchBrokerMessage((const char*) pMessage, nType);
            break;
        case MSG_ONAIR_BASE + ONAIR_CAMERA : 
            setClientStatus((const char *) pMessage,"video",nType == ONAIR_DEVICE_ON ? "on" : "off");
            break;
        case MSG_ONAIR_BASE + ONAIR_MICRO : 
            setClientStatus((const char *) pMessage,"audio",nType == ONAIR_DEVICE_ON ? "on" : "off");
            break;
            
    }
    return(EVENT_MSG_RESULT_OK);
}


