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
    bool bHasChanged = false;
    if(pszDevice && pszMode) {
        String strName = pszDevice;
        String strValue = pszMode;
        int nDevState = ONAIR_DEVICE_OFF;
        if(strValue.equals("on") || LSC::isTrueValue(pszMode)) nDevState = ONAIR_DEVICE_ON;
        if(strName.equalsIgnoreCase("audio") || strName.equalsIgnoreCase("mic") )       { 
            if(isMicOn != nDevState) bHasChanged = true;
            isMicOn = nDevState;
            ulLastUpdate = millis(); 
            DEBUG_INFOS(" - setting isMicOn to : %d TS(%lu)",isCamOn,ulLastUpdate);
        }     
        else if(strName.equalsIgnoreCase("video") || strName.equalsIgnoreCase("cam"))  { 
            if(isCamOn != nDevState) bHasChanged = true;
            isCamOn = nDevState; 
            ulLastUpdate = millis();
            DEBUG_INFOS(" - setting isCamOn to : %d TS(%lu)",isCamOn,ulLastUpdate);
        } else if(strName.equalsIgnoreCase("media")) {
            if(isCamOn != nDevState) bHasChanged = true;
            if(isMicOn != nDevState) bHasChanged = true;
            isCamOn = nDevState;
            isMicOn = nDevState;
            ulLastUpdate = millis();
            DEBUG_INFOS(" - setting all media to : %d TS(%lu)",isCamOn,ulLastUpdate);
        } else {
            DEBUG_INFOS(" - unknown device : %s",pszDevice);
        }
        if(bHasChanged) {
            Appl.MsgBus.sendEvent(this,MSG_MQTT_SEND_JSONSTATE,Appl.getState(),0);
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

void COnAirLight::readConfigFrom(CJsonNode &oCfg) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    Config.OnCamMode = getModeByName(oCfg.getValue("oncam"),Config.OnCamMode);
    Config.OnMicMode = getModeByName(oCfg.getValue("onmic"),Config.OnMicMode);
    String strPrio = oCfg.getValue("priority");
    if(strPrio) {
        if(strPrio == "mic") Config.Priority = ONAIR_MICRO;
        if(strPrio == "cam") Config.Priority = ONAIR_CAMERA;
    }
    unsigned long ulBrightness = 0;
    ulBrightness = oCfg.getValueAsUnsignedLong("brightness",ulBrightness);
    // If not available, use default.
    if(ulBrightness < 10 || ulBrightness > 100) ulBrightness = ONAIR_LIGHT_BRIGHTNESS_DEFAULT;
    this->setBrightness(ulBrightness);
    // Timeout is in seconds, so convert to millis ! - currently fix
    /*
    String strTimeOut = oCfg["timeout"];
    if(strTimeOut && strTimeOut.length() > 0) {
        Config.TimeOutMillis = strTimeOut.toInt() * 1000; // Convert to millis
    }   
    DEBUG_INFOS(" --- timeout set to: %lu",Config.TimeOutMillis);
    */
    DEBUG_FUNC_END();
}

void COnAirLight::writeConfigTo(CJsonNode &oCfg, bool bHideCritical) {
    DEBUG_FUNC_START();
    setNameOfMode(oCfg,"oncam",Config.OnCamMode);
    setNameOfMode(oCfg,"onmic",Config.OnMicMode);
    oCfg["priority"]    = Config.Priority == ONAIR_MICRO ? "mic" : "cam";
    oCfg["brightness"]   = this->getBrightness();
    oCfg["timeout"]     = Config.TimeOutMillis / 1000;
    DEBUG_FUNC_END();
}

void COnAirLight::writeStatusTo(CJsonNode &oCfg,int nLevel) {
    oCfg["isMicOn"] = isMicOn();
    oCfg["isCamOn"] = isCamOn();
    oCfg["isLightOn"] = isOn();
    oCfg["timeout"] = Config.TimeOutMillis / 1000;
    // JsonArray oArray = CreateJsonArray(oCfg,"clients");
    JsonNode * pArray = oCfg.getArray("clients",true);
    for(auto oClient : tClientStaties) {
        // JsonObject oData = CreateEmptyJsonObject(oArray);//  oArray.createNestedObject();
        JsonNode *pData = pArray->createObject(); 
        (*pData)["client"] = oClient.first.c_str();
        (*pData)["isCamOn"] = oClient.second.isCamOn;
        (*pData)["isMicOn"] = oClient.second.isMicOn;
        (*pData)["lastUpd"] = oClient.second.ulLastUpdate;
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

String COnAirLight::setNameOfMode(CJsonNode &oCfg, const char *strKey, int nMode) {
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
    // DEBUG_DELAY(100);
}
 

void COnAirLight::dispatchBrokerMessage(const char *pszMessage, int nLen) {
    DEBUG_FUNC_START_PARMS("\"%s\"",pszMessage);
    if(pszMessage && nLen > 10) {
        CJsonNode oMsg;
        // JSON_DOC(oMsgDoc,nLen * 4);
        // DynamicJsonDocument oMsgDoc(nLen * 4);
        oMsg.parse(pszMessage);
        // DeserializationError rc = deserializeJson(oMsgDoc,pszMessage);
        // if(rc == DeserializationError::Ok) {
            // CJsonNode oMsg; // = GetJsonDocumentAsObject(oMsgDoc);
            // String strClient = JsonKeyExists(oMsg,"client",String) ? (const char*) oMsg["client"] : "-anonymous-";
            String strClient = oMsg.getValueAsCharPointer("client","-ananymous-");

            if(oMsg.exists("mic"))      setClientStatus(strClient, "audio", oMsg.getValue("mic"));
            if(oMsg.exists("audio"))    setClientStatus(strClient, "audio", oMsg.getValue("audio"));
            if(oMsg.exists("cam"))      setClientStatus(strClient, "video", oMsg.getValue("cam"));
            if(oMsg.exists("video"))    setClientStatus(strClient, "video", oMsg.getValue("video"));
            if(oMsg.exists("media"))  {
                setClientStatus(strClient, "audio",oMsg.getValue("media"));
                setClientStatus(strClient, "video",oMsg.getValue("media"));
            }
                /*
        } else {
            DEBUG_INFOS("Json deserialization error %s",rc.c_str());
        }
            */
    }
}

inline void COnAirLight::setClientStatus(String strClientAddress, const char *pszName, const char *pszValue) {
    DEBUG_FUNC_START_PARMS("'%s',%s,%s",strClientAddress.c_str(),NULL_POINTER_STRING(pszName),NULL_POINTER_STRING(pszValue));
    OnAirLightStatus *pStatus = &tClientStaties[strClientAddress]; //  getClientStatus(strClientAddress);
    pStatus->setState(pszName,pszValue);
   
    
    DEBUG_FUNC_END();
}


/// @brief Set the mode to on / off 
/// @param pSender sender of the message
/// @param nMsgId message, based on MSG_ONAIR_BASE
/// @param pMessage - currently not respected - future will be the hostname with open/closed devices
/// @param nType  - ONAIR_DEVICE_ON or ONAIR_DEVICE_OFF
/// @return 
int COnAirLight::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
    DEBUG_FUNC_START_PARMS("%d,%d",nMsgId,nType);
    switch(nMsgId) {

        case MSG_MQTT_MSG_RECEIVED : { // Message Broker Message in pMessage
            MQTTMessage *pMsg = (MQTTMessage *) pMessage;
            if(pMsg && pMsg->isDeviceCommandTopic() && pMsg->Message) {
                dispatchBrokerMessage(pMsg->Message, strlen(pMsg->Message));
            }
            break;
        }
        case MSG_ONAIR_BASE + ONAIR_CAMERA : 
            setClientStatus((const char *) pMessage,"video",nType == ONAIR_DEVICE_ON ? "on" : "off");
            break;
        case MSG_ONAIR_BASE + ONAIR_MICRO : 
            setClientStatus((const char *) pMessage,"audio",nType == ONAIR_DEVICE_ON ? "on" : "off");
            break;
            
    }
    return(EVENT_MSG_RESULT_OK);
}

/**
 * @brief insert the Home Assistant component informations.
 * Do not use msg queue to avoid duplicate insertion
 * see messages for Home Assistance
 * --> https://github.com/home-assistant/core/blob/dev/homeassistant/components/mqtt/abbreviations.py
 * --> https://www.home-assistant.io/integrations/mqtt/#supported-abbreviations-in-mqtt-discovery-messages
 * --> Icons : https://pictogrammers.com/library/mdi/
 */
void COnAirLight::insertComponentDiscovery(const char *pszCompName,JsonNode & oComponentArea, CMQTTController * pController) {

    String strCommandTopic = pController ? pController->getDeviceCommandBaseTopicPath() : "";
    String strStateTopic = pController ? pController->Config.PublishTopicPrefix + "/state" : "";
    String strCompID;
    strCompID = Appl.getDeviceID();
    strCompID += "-cam";
    JsonNode * pCameraSensor = oComponentArea.getObject("cam",true);
    pCameraSensor->setValue("p",                   "binary_sensor");
    pCameraSensor->setValue("icon",                "mdi:cctv");
    pCameraSensor->setValue("value_template" ,     "{{ 'ON' if value_json.onair.isCamOn else 'OFF' }}");
    pCameraSensor->setValue("payload_on",          "ON");
    pCameraSensor->setValue("payload_off",         "OFF");
    pCameraSensor->setValue("unique_id",           strCompID.c_str());
    pCameraSensor->setValue("name",                "Camera status");

    strCompID = Appl.getDeviceID();
    strCompID += "-mic";
    JsonNode * pMicrophoneSensor = oComponentArea.getObject("mic",true);
    pMicrophoneSensor->setValue("p",                  "binary_sensor");
    pMicrophoneSensor->setValue("icon",               "mdi:microphone");
    pMicrophoneSensor->setValue("value_template",     "{{ 'ON' if value_json.onair.isMicOn else 'OFF' }}");
    pMicrophoneSensor->setValue("payload_on",         "ON");
    pMicrophoneSensor->setValue("payload_off",        "OFF");
    pMicrophoneSensor->setValue("unique_id",          strCompID.c_str());
    pMicrophoneSensor->setValue("name",               "Microphone status");

    strCompID = Appl.getDeviceID();
    strCompID += "-light-switch";
    JsonNode * pLightSwitch = oComponentArea.getObject("light",true);
    pLightSwitch->setValue("p",                       "switch");
    pLightSwitch->setValue("icon",                    "mdi:alarm-light");
    pLightSwitch->setValue("value_template",          "{{ 'ON' if value_json.onair.isCamOn or value_json.onair.isMicOn else 'OFF' }}");
    pLightSwitch->setValue("state_on",                "ON");
    pLightSwitch->setValue("state_off",               "OFF");
    pLightSwitch->setValue("command_topic",           (strCommandTopic + "/onair/light").c_str());
    pLightSwitch->setValue("payload_on",              "{\"client\":\"HomeAssistant\",\"media\":\"on\"}");
    pLightSwitch->setValue("payload_off",             "{\"client\":\"HomeAssistant\",\"media\":\"off\"}");
    pLightSwitch->setValue("unique_id",               strCompID.c_str());
    pLightSwitch->setValue("name",                    "On Air light");

    strCompID = Appl.getDeviceID();
    strCompID += "-cam-switch";
    JsonNode * pCameraSwitch = oComponentArea.getObject("cam_switch",true);
    pCameraSwitch->setValue("p",                      "switch");
    pCameraSwitch->setValue("icon",                   "mdi:cctv");
    pCameraSwitch->setValue("value_template",         "{{ 'ON' if value_json.onair.isCamOn else 'OFF' }}");
    pCameraSwitch->setValue("state_on",               "ON");
    pCameraSwitch->setValue("state_off",              "OFF");
    pCameraSwitch->setValue("command_topic",          (strCommandTopic + "/onair/cam").c_str());
    pCameraSwitch->setValue("payload_on",             "{\"client\":\"HomeAssistant\",\"cam\":\"on\"}");
    pCameraSwitch->setValue("payload_off",            "{\"client\":\"HomeAssistant\",\"cam\":\"off\"}");
    pCameraSwitch->setValue("unique_id",              strCompID.c_str());
    pCameraSwitch->setValue("name",                   "Camera switch");

    strCompID = Appl.getDeviceID();
    strCompID += "-mic-switch";
    JsonNode * pMicrophoneSwitch = oComponentArea.getObject("mic_switch",true);
    pMicrophoneSwitch->setValue("p",                  "switch");
    pMicrophoneSwitch->setValue("icon",               "mdi:microphone");
    pMicrophoneSwitch->setValue("value_template",     "{{ 'ON' if value_json.onair.isMicOn else 'OFF' }}");
    pMicrophoneSwitch->setValue("state_on",           "ON");
    pMicrophoneSwitch->setValue("state_off",          "OFF");
    pMicrophoneSwitch->setValue("command_topic",      (strCommandTopic + "/onair/mic").c_str());
    pMicrophoneSwitch->setValue("payload_on",         "{\"client\":\"HomeAssistant\",\"mic\":\"on\"}");
    pMicrophoneSwitch->setValue("payload_off",        "{\"client\":\"HomeAssistant\",\"mic\":\"off\"}");
    pMicrophoneSwitch->setValue("unique_id",          strCompID.c_str());
    pMicrophoneSwitch->setValue("name",               "Microphone switch");

    strCompID = Appl.getDeviceID();
    strCompID += "-clients";
    JsonNode * pClientSensor = oComponentArea.getObject("clients",true);
    pClientSensor->setValue("p",                      "sensor");
    pClientSensor->setValue("icon",                   "mdi:account-multiple");
    pClientSensor->setValue("unit_of_measurement",    "clients");
    pClientSensor->setValue("value_template",         "{{ value_json.onair.clients | count if value_json.onair.clients is defined else 0 }}");
    pClientSensor->setValue("json_attributes_topic",  strStateTopic.c_str());
    pClientSensor->setValue("json_attributes_template","{{ {'clients': value_json.onair.clients | default([])} | tojson }}");
    pClientSensor->setValue("unique_id",              strCompID.c_str());
    pClientSensor->setValue("name",                   "Clients");

}
