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
bool OnAirLightStatus::setState(const char *pszDevice, const char *pszMode) {
    DEBUG_FUNC_START_PARMS("%s,%s",NULL_POINTER_STRING(pszDevice),NULL_POINTER_STRING(pszMode));
    bool bHasChanged = false;
    if(pszDevice && pszMode) {
        String strName = pszDevice;
        int nDevState = ONAIR_DEVICE_OFF;
        // handle a true value - "On", "+"", "true" ... see LSC::isTrueValue()
        if(LSC::isTrueValue(pszMode)) nDevState = ONAIR_DEVICE_ON;
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
    return(bHasChanged);
}

/// @brief Return true once this client's last update is older than the configured timeout.
bool OnAirLightStatus::isTimeOutReached(unsigned long ulTimeOutMillis) {
    bool bResult = false;
   
    if(ulTimeOutMillis > 0 && ulLastUpdate > 0) {
        bResult = (ulLastUpdate + ulTimeOutMillis) < millis();
    }
    return(bResult);
}


/// @brief Convert a config mode name into the internal light mode constant.
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

/// @brief Store a readable mode name for an internal light mode constant.
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


#pragma endregion

#pragma region Constructor - Module Interface implementation

/// @brief Create an on-air light on the given GPIO pin and apply default brightness.
COnAirLight::COnAirLight(int nPin) : CLightSwitch(nPin) {
    this->setBrightness(ONAIR_LIGHT_BRIGHTNESS_DEFAULT);
}

/// @brief Load light modes, priority and brightness from the application config.
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

/// @brief Persist the current on-air light configuration into a JSON node.
void COnAirLight::writeConfigTo(CJsonNode &oCfg, bool bHideCritical) {
    DEBUG_FUNC_START();
    setNameOfMode(oCfg,"oncam",Config.OnCamMode);
    setNameOfMode(oCfg,"onmic",Config.OnMicMode);
    oCfg["priority"]    = Config.Priority == ONAIR_MICRO ? "mic" : "cam";
    oCfg["brightness"]   = this->getBrightness();
    oCfg["timeout"]     = Config.TimeOutMillis / 1000;
    DEBUG_FUNC_END();
}

/// @brief Write effective light state and per-client state to the status JSON.
void COnAirLight::writeStatusTo(CJsonNode &oCfg,int nLevel) {
    DEBUG_FUNC_START_PARMS("%p,%d",&oCfg,nLevel);
    oCfg["isMicOn"] = isMicOn();
    oCfg["isCamOn"] = isCamOn();
    oCfg["isLightOn"] = isOn();
    oCfg["timeout"] = Config.TimeOutMillis / 1000;
    // JsonArray oArray = CreateJsonArray(oCfg,"clients");

    JsonNode *pClients = oCfg.getObject("clients",true);
    for(auto oClient : tClientStaties) {
        // JsonObject oData = CreateEmptyJsonObject(oArray);//  oArray.createNestedObject();
        JsonNode *pData = pClients->createObject(oClient.first.c_str());
        (*pData)["isCamOn"] = oClient.second.isCamOn;
        (*pData)["isMicOn"] = oClient.second.isMicOn;
        (*pData)["lastUpd"] = oClient.second.ulLastUpdate;
    }
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
        case MSG_APPL_STARTED:  // Application started, all modules are initialized and available...
            m_bNeedsHAUpdate = true; // Force update to HA, as we have no idea if the light is on or off... 
            break;

        case MSG_MQTT_MSG_RECEIVED : { // Message Broker Message in pMessage
            MQTTMessage *pMsg = (MQTTMessage *) pMessage;
            if(pMsg && pMsg->isDeviceCommandTopic() && pMsg->Message) {
                dispatchBrokerMessage(pMsg);
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

#pragma endregion


/// @brief Recompute the effective media state and drive the physical light mode.
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
    if(m_bNeedsHAUpdate) {
        Appl.MsgBus.sendEvent(this,MSG_MQTT_SEND_JSONSTATE,Appl.getState(),0);
        m_bNeedsHAUpdate = false;
    }
    // DEBUG_INFOS(" - isMicOn(%d) isCamOn(%d) -> Mode %d (CamMode %d) (MicMode %d)",bIsMicOn, bIsCamOn, nMode,Config.OnCamMode,Config.OnMicMode);
    // DEBUG_DELAY(100);
}
 

/// @brief Update or create the media state entry for one client.
inline bool COnAirLight::setClientStatus(String strClientAddress, const char *pszName, const char *pszValue) {
    DEBUG_FUNC_START_PARMS("'%s',%s,%s",strClientAddress.c_str(),NULL_POINTER_STRING(pszName),NULL_POINTER_STRING(pszValue));
    OnAirLightStatus *pStatus = &tClientStaties[strClientAddress]; //  getClientStatus(strClientAddress);
    bool bHasChanged = pStatus->setState(pszName,pszValue);
    m_bNeedsHAUpdate = true;
    DEBUG_FUNC_END();
    return(bHasChanged);
}



#pragma region Home Assistant Auto Discovery and MQTT Messages
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
    char szValueTemplate[256];
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

    writeHAValueSelectTo(szValueTemplate,sizeof(szValueTemplate),"Cam");
    String strDevCmdTopic = strCommandTopic + "/" + m_szHA_DeviceName + "/cam";
    strCompID = Appl.getDeviceID();
    strCompID += "-cam-switch";
    JsonNode * pCameraSwitch = oComponentArea.getObject("cam_switch",true);
    pCameraSwitch->setValue("p",                      "switch");
    pCameraSwitch->setValue("icon",                   "mdi:cctv");
    pCameraSwitch->setValue("value_template",         szValueTemplate);
    pCameraSwitch->setValue("state_on",               "ON");
    pCameraSwitch->setValue("state_off",              "OFF");
    pCameraSwitch->setValue("command_topic",          strDevCmdTopic.c_str());
    pCameraSwitch->setValue("payload_on",             "ON");
    pCameraSwitch->setValue("payload_off",            "OFF");
    pCameraSwitch->setValue("unique_id",              strCompID.c_str());
    pCameraSwitch->setValue("name",                   "Camera switch");

    JsonNode * pMicrophoneSwitch = oComponentArea.getObject("mic_switch",true);
    writeHAValueSelectTo(szValueTemplate,sizeof(szValueTemplate),"Mic");
    strDevCmdTopic = strCommandTopic + "/" + m_szHA_DeviceName + "/mic";
    strCompID = Appl.getDeviceID();
    strCompID += "-mic-switch";
    String  strValueTemplate = "{{ 'ON' if value_json.onair.isMicOn else 'OFF' }}";
    pMicrophoneSwitch->setValue("p",                  "switch");
    pMicrophoneSwitch->setValue("icon",               "mdi:microphone");
    pMicrophoneSwitch->setValue("value_template",     szValueTemplate);
    pMicrophoneSwitch->setValue("state_on",           "ON");
    pMicrophoneSwitch->setValue("state_off",          "OFF");
    pMicrophoneSwitch->setValue("command_topic",      strDevCmdTopic.c_str());
    pMicrophoneSwitch->setValue("payload_on",         "ON");
    pMicrophoneSwitch->setValue("payload_off",        "OFF");
    pMicrophoneSwitch->setValue("unique_id",          strCompID.c_str());
    pMicrophoneSwitch->setValue("name",               "Microphone switch");

    JsonNode * pClientSensor = oComponentArea.getObject("clients",true);
    strCompID = Appl.getDeviceID();
    strCompID += "-clients";
    pClientSensor->setValue("p",                      "sensor");
    pClientSensor->setValue("icon",                   "mdi:account-multiple");
    pClientSensor->setValue("unit_of_measurement",    "clients");
    pClientSensor->setValue("value_template",         "{{ value_json.onair.clients | count if value_json.onair.clients is defined else 0 }}");
    pClientSensor->setValue("json_attributes_topic",  strStateTopic.c_str());
    pClientSensor->setValue("json_attributes_template","{{ {'clients': value_json.onair.clients | default([])} | tojson }}");
    pClientSensor->setValue("unique_id",              strCompID.c_str());
    pClientSensor->setValue("name",                   "Clients");

}



/// @brief Parse an MQTT JSON command and update the addressed client's media state.
void COnAirLight::dispatchBrokerMessage(MQTTMessage *pMsg) {
    DEBUG_FUNC_START_PARMS("\"Topic:%s - DevCmdTopic:%s - Cmd:%s\"",NULL_POINTER_STRING(pMsg->Topic),NULL_POINTER_STRING(pMsg->DeviceCmdTopic),NULL_POINTER_STRING(pMsg->Message));
    if(pMsg && pMsg->pController && pMsg->Message && pMsg->DeviceCmdTopic) {
        String strDevice = pMsg->DeviceCmdTopic;
        String strClient = strDevice.substring(0,strDevice.indexOf('/'));
        strDevice.remove(0,strClient.length() + 1); // Remove the client name
        setClientStatus(strClient,strDevice.c_str(),pMsg->Message);
    }
    DEBUG_FUNC_END();
}

#pragma endregion
