// #pragma GCC diagnostic ignored "-Woverflow"
#ifndef DEBUG_LSC_WIFI
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <ArduinoJson.h>
#include <WiFiController.h>
#include <LSCUtils.h>
#include <AppMsgs.h>

namespace LSC_WIFI {
    /// @brief Create the Default SSID for the Access Point
    /// @return 
    String getDefaultSSIDofAP()
    {
        uint8_t macAddr[6];
        WiFi.softAPmacAddress(macAddr);
        char ssid[strlen(WIFI_DEFAULT_AP_SSID_PREFIX) + 10];
        sprintf(ssid, WIFI_DEFAULT_AP_SSID_PREFIX "-%02x%02x%02x%02x",macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        return String(ssid);
    }
}

CWiFiController::CWiFiController(const WiFiConfig *pConfig) 
{
    if(pConfig != nullptr) Config = *pConfig;
}


/// @brief Create the Default SSID for the Access Point
/// @return 
String CWiFiController::getDefaultSSIDofAP()
{
    return(LSC_WIFI::getDefaultSSIDofAP());
}

String CWiFiController::getStatusText(int nWiFiStatus) {
    switch (nWiFiStatus)
    {
        case WL_IDLE_STATUS:         return "Idle";
        case WL_NO_SSID_AVAIL:       return "No SSID available";
        case WL_SCAN_COMPLETED:      return "Scan completed";
        case WL_CONNECTED:           return "Connected";
        case WL_CONNECT_FAILED:      return "Connect failed";
        case WL_CONNECTION_LOST:     return "Connection lost";
        case WL_WRONG_PASSWORD:      return "Wrong password";
        case WL_DISCONNECTED:        return "Disconnected";
        default:                     return "Unknown status";
    }
}

#pragma region "Interface to Config / Status handling"
/**
 * Write your configuration into the json object,
 */
void CWiFiController::writeConfigTo(JsonObject &oCfgNode,bool bHideCritical) {
    DEBUG_FUNC_START();
    // Hostname, Operation Mode and Fallback settings
    oCfgNode["hostname"]        = Config.wifi_Hostname;
    oCfgNode["ap_mode"]         = Config.accessPointMode;
    oCfgNode["fallback"]        = Config.autoFallbackMode;

    // Access Point specific settings
    oCfgNode["ap_ssid"]         = Config.ap_ssid;
    oCfgNode["ap_pwd"]          = bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.ap_Password;
    oCfgNode["ap_hide"]         = Config.ap_hidden;
    oCfgNode["ap_channel"]      = Config.ap_channel;
    if(Config.ap_ipAddress.isSet())   oCfgNode["ap_ipaddress"] = Config.ap_ipAddress.toString(); 
    if(Config.ap_ipSubnetMask.isSet())oCfgNode["ap_subnet"]    = Config.ap_ipSubnetMask.toString();
    // WiFi specific settings
    oCfgNode["ssid"]            = Config.wifi_ssid;
    oCfgNode["bssid"]           = Config.wifi_bssid;
    oCfgNode["wifi_pwd"]        = bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.wifi_Password;
    oCfgNode["dhcp"]            = Config.dhcpEnabled;
    if(Config.ipAddress.isSet())     oCfgNode["ipaddress"] = Config.ipAddress.toString();
    if(Config.ipSubnetMask.isSet())  oCfgNode["subnet"]    = Config.ipSubnetMask.toString();
    if(Config.ipDNS.isSet())         oCfgNode["dnsip"]     = Config.ipDNS.toString();
    if(Config.ipGateway.isSet())     oCfgNode["gwip"]      = Config.ipGateway.toString();
    DEBUG_FUNC_END();
}

void CWiFiController::readConfigFrom(JsonObject &oNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oNode);
    storeValueIF(Config.wifi_Hostname,    oNode["hostname"]);
    storeValueIF(&Config.accessPointMode, oNode["ap_mode"]);
    storeValueIF(&Config.autoFallbackMode,   oNode["fallback"]);

    storeValueIF(Config.ap_ssid ,            oNode["ap_ssid"]);
    String strPass = oNode["ap_pwd"];
    if(!(oNode["ap_pwd"] == WIFI_HIDDEN_PASSWORD)) {
        storeValueIF(Config.ap_Password,oNode["ap_pwd"]);
    }
    storeValueIF(&Config.ap_hidden,          oNode["ap_hide"]);
    storeValueIF(&Config.ap_channel,         oNode["ap_channel"]);
    setIPAddressIF(Config.ap_ipAddress,      oNode["ap_ipaddress"]);
    setIPAddressIF(Config.ap_ipSubnetMask,   oNode["ap_subnet"]);

    storeValueIF(Config.wifi_ssid ,          oNode["ssid"]);

    if(!(oNode["wifi_pwd"] == WIFI_HIDDEN_PASSWORD)) {
        storeValueIF(Config.wifi_Password , oNode["wifi_pwd"]);
    }
    if(oNode["bssid"]) parseBytesTo(Config.wifi_bssid, oNode["bssid"],':',sizeof(Config.wifi_bssid),16);
    storeValueIF(&Config.dhcpEnabled,        oNode["dhcp"]);
    setIPAddressIF(Config.ipAddress,         oNode["ipaddress"]);
    setIPAddressIF(Config.ipSubnetMask,      oNode["subnet"]);
    setIPAddressIF(Config.ipGateway,         oNode["gwip"]);
    setIPAddressIF(Config.ipDNS,             oNode["dnsip"]);
    DEBUG_FUNC_END();
}

void CWiFiController::writeStatusTo(JsonObject &oStatusNode) {
    DEBUG_FUNC_START();
    oStatusNode["accesspoint"] = Status.isInAccessPointMode;
    oStatusNode["stationmode"] = Status.isInStationMode,
    oStatusNode["isConnected"] = Status.isWiFiConnected;
    oStatusNode["startTime"]   = Status.startTimeInMillis;
    oStatusNode["stopTime"]    = Status.stopTimeInMillis;
    oStatusNode["restartTime"] = Status.restartTimeInMillis;
    oStatusNode["onlineTime"]  = Status.startTimeInMillis > 0 ? millis() - Status.startTimeInMillis : 0;
    struct ip_info oIPInfo;
    if ( Status.isInAccessPointMode)
	{
		wifi_get_ip_info(SOFTAP_IF, &oIPInfo);
		struct softap_config oConf;
		wifi_softap_get_config(&oConf);
		oStatusNode["ssid"]  = String(reinterpret_cast<char *>(oConf.ssid));
		oStatusNode["mac"]   = WiFi.softAPmacAddress();
	}
	else
	{
		wifi_get_ip_info(STATION_IF, &oIPInfo);
		struct station_config oConf;
		wifi_station_get_config(&oConf);
		oStatusNode["ssid"] = String(reinterpret_cast<char *>(oConf.ssid));
		oStatusNode["dns"] = WiFi.dnsIP().toString();
		oStatusNode["mac"] = WiFi.macAddress();
	}

    IPAddress ipaddr = IPAddress(oIPInfo.ip.addr);
	IPAddress gwaddr = IPAddress(oIPInfo.gw.addr);
	IPAddress nmaddr = IPAddress(oIPInfo.netmask.addr);

    oStatusNode["ip"]       = ipaddr.toString(); // getAddressAsString(ipaddr);
	oStatusNode["gateway"]  = gwaddr.toString(); // getAddressAsString(gwaddr);
	oStatusNode["netmask"]  = nmaddr.toString(); //getAddressAsString(nmaddr);
    oStatusNode["hostname"] = WiFi.getHostname();
    oStatusNode["rssi"] = WiFi.RSSI();
    DEBUG_FUNC_END();
}

void CWiFiController::writeStatusToLog() {
    DynamicJsonDocument oStatusDoc(1024);
    JsonObject oStatusObj = oStatusDoc.to<JsonObject>();
    writeStatusTo(oStatusObj);
    String strPretty;
    serializeJsonPretty(oStatusDoc,strPretty);
    Appl.Log.logVerbose(F("WiFi Status:%s\n"),strPretty.c_str());
}

#pragma endregion

#pragma region "Starter for Access Point and Station Mode"

/// @brief start the access point
///       - if it does not work by config start a default access point
/// @param bUseConfigData use config data
/// @return 
bool CWiFiController::startAccessPoint(bool bUseConfigData)
{   bool bIsConnected = false;
    if(bUseConfigData) {
        bIsConnected = startAccessPoint(Config.ap_ssid.c_str(),
                                        Config.ap_ipAddress,
                                        Config.ap_ipSubnetMask,
                                        Config.ap_hidden);
    }
    // Start the default access point, if config did not work...
    if(!bIsConnected) {    
        bIsConnected = startAccessPoint(getDefaultSSIDofAP().c_str(), 
                                        WIFI_MODULE_DEFAULT_AP_IP, 
                                        WIFI_MODULE_DEFAULT_AP_SUBNET, 
                                        false);
    }
    
    return bIsConnected;
}

/// @brief Start the Access Point of WiFi so, the user can connect and configure this device
///        SSID will be the default SSID of this Node...
/// @param ipAP         IPAddress of this node
/// @param ipSubnetAP   Subnet mask of this node. 
/// @param bHidden      if true, the SSID will be hidden...
/// @param pszPassword  Password for this Accesspoint
/// @return connected or not
bool CWiFiController::startAccessPoint(const char * pszSSID, 
                                        IPAddress ipAP, 
                                        IPAddress ipSubnetAP,
                                         bool bHidden, 
                                         const char *pszPassword)
{
    DEBUG_FUNC_START();
    if(pszSSID != nullptr) {
        Appl.Log.logInfo(F("Starting WiFi Access Point: %s"),pszSSID);
        Appl.MsgBus.sendEvent(this,MSG_STARTING_WIFI,nullptr,WIFI_ACCESS_POINT_MODE);

        Status.isInStationMode      = false;
        Status.isInAccessPointMode  = true;
        Status.startTimeInMillis    = 0;
        Status.wifiStatus = WL_DISCONNECTED;

        WiFi.mode(WIFI_AP);
    
        // local-ip, gateway, subnet
        WiFi.softAPConfig(ipAP, ipAP, ipSubnetAP);
        Status.isWiFiConnected = WiFi.softAP(pszSSID, 
                                            pszPassword, 
                                            Config.ap_channel, 
                                            bHidden ? 1 : 0);
        if(Status.isWiFiConnected) {
            Status.wifiStatus = WL_IDLE_STATUS;
            Status.startTimeInMillis = millis();
            Appl.MsgBus.sendEvent(this,MSG_WIFI_CONNECTED,nullptr,WIFI_ACCESS_POINT_MODE);
        }
    }
    DEBUG_FUNC_END_PARMS("%d",Status.isWiFiConnected);
    return Status.isWiFiConnected;
}

// Try to connect to an existing Wi-Fi (Station Mode)
bool CWiFiController::joinNetwork(const char *pszSSID, const char *pszPassword, byte bSSID[6])
{
    DEBUG_FUNC_START();
    Appl.Log.logInfo(F("Joining WiFi network %s"),pszSSID);

    Status.isInStationMode = true;
    Status.isInAccessPointMode = false;
    Status.startTimeInMillis = 0;
    Status.restartTimeInMillis = -1;    

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);

    
    if (!Config.dhcpEnabled)
    {
        Appl.Log.logInfo(F(" - using static IP address %s"), Config.ipAddress.toString().c_str());   
        WiFi.config(Config.ipAddress, Config.ipGateway, Config.ipSubnetMask, Config.ipDNS);
    }
    bool useBSSID = false;
    for (int i = 0; i < 6; i++)
    {
        if (bSSID[i] != 0) useBSSID = true;
    }
    if (useBSSID) {
        Appl.Log.logInfo(F(" - using fix bssid"));
        Status.wifiStatus = WiFi.begin(Config.wifi_ssid, Config.wifi_Password, 0, Config.wifi_bssid);
    }
    else {
        DEBUG_INFOS(" - begin connection to %s/%s",Config.wifi_ssid.c_str(),Config.wifi_Password.c_str());
        Status.wifiStatus = WiFi.begin(Config.wifi_ssid, Config.wifi_Password);
    }         
    
    unsigned long now = millis();
    uint8_t nTimeout = 15; // define when to time out in seconds
    do {
        if (WiFi.isConnected()) break;
        delay(500);
    } while (millis() - now < nTimeout * 1000);

    if (WiFi.isConnected())
    {
        Appl.MsgBus.sendEvent(this,MSG_WIFI_CONNECTED,nullptr,WIFI_STATION_MODE);
        Status.isWiFiConnected  = true;
        Status.startTimeInMillis = millis();
    } else {
        Appl.MsgBus.sendEvent(this,MSG_WIFI_ERROR,nullptr,Status.wifiStatus);
        Appl.Log.logError(F(" - failed to connect to %s"),Config.wifi_ssid.c_str());
        Appl.Log.logError(F(" - status: %d (%s)"),Status.wifiStatus,getStatusText(Status.wifiStatus));
    }
    return(Status.isWiFiConnected );
}


#pragma endregion



/// @brief start the setup of wifi (first call in usage)
/// If the instance has already a configuration, we can start the setup with enableWiFi).
/// Otherwise go into Default Acces Point mode
/// @param bUseConfigData if false - start default AP, if true, use the loaded config data.
void CWiFiController::startWiFi(bool bUseConfigData)
{
    DEBUG_FUNC_START_PARMS("%d",bUseConfigData);
    Status.restartTimeInMillis = -1;
    // Appl.MsgBus.sendEvent(this,MSG_RESTART_WIFI,nullptr,WIFI_RESTART_OFF);
    if (!bUseConfigData)
    {
        WiFi.hostname( WIFI_DEFAULT_AP_SSID_PREFIX );
        startAccessPoint(false);
    } else
    {
         //  wifiConnectHandler = WiFi.onStationModeConnected(onWifiConnect);
        // wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
        // wifiOnStationModeGotIPHandler = WiFi.onStationModeGotIP(onWifiGotIP);
        WiFi.hostname(Config.wifi_Hostname);
        bool bIsConnected = false;
        if(Config.accessPointMode) {
            bIsConnected = startAccessPoint(true);
        } else {
            bIsConnected = joinNetwork(Config.wifi_ssid.c_str(), 
                                        Config.wifi_Password.c_str(), 
                                        Config.wifi_bssid);
            if(!bIsConnected) {
                if(Config.autoFallbackMode || Status.wifiStatus == WL_WRONG_PASSWORD ) {
                    Appl.Log.logInfo(F("... auto fallback to AP Mode"));
                    bIsConnected = startAccessPoint(true);
                }
                if(!bIsConnected) {
                    // If the connection has the wrong password... do not try to restart.
                    // Reconnect retry...
                    Appl.Log.logInfo(F("... trying reconnect to WiFi network in %d seconds"),Config.retryTimeoutSeconds);
                    Status.restartTimeInMillis = millis() + (Config.retryTimeoutSeconds * 1000); // Retry in 10 seconds
                }
                if(bIsConnected) Status.nRetryConnectCounter = 0;
            }
        } 
    }
    writeStatusToLog();
    DEBUG_FUNC_END();
};

bool CWiFiController::restartIfNeeded() {
    if(!Status.isWiFiConnected) {
        if(Status.restartTimeInMillis > millis()) {
            if(Config.retryCount < Status.nRetryConnectCounter) {
                Status.nRetryConnectCounter++;
                startWiFi(true);
            } else {
                disableWiFi();
            }
        }
    }
    return Status.isWiFiConnected;
}

/*
/// @brief Enable the WiFi, depending on Config information loaded...
void CWiFiController::startWiFiByConfig()
{
    DEBUG_FUNC_START();
    Status.restartTimeInMillis = -1; // Do not restart while starting...
    
    DEBUG_INFOS(" - Current mode: %d",Config.accessPointMode);

    // Is the access point mode requested?
    if (Config.accessPointMode)
    {
        Appl.Log.logInfo(F("Starting Access Point: %s"),Config.ap_ssid.c_str());
        // Start the access point - restart not needed, cause if it does not start,
        // There is a config error - this would not be fixed if we try again...
        if(!startAccessPoint(Config.ap_ssid.c_str(), Config.ap_ipAddress, Config.ap_ipSubnetMask, Config.ap_hidden)) {
            Appl.Log.logError(F(" - failed to start %s"),Config.ap_ssid.c_str());
            startAccessPoint(WIFI_DEFAULT_AP_SSID.c_str(), WIFI_MODULE_DEFAULT_AP_IP, WIFI_MODULE_DEFAULT_AP_SUBNET, false);
        }
    }
    else
    {
        Status.isWiFiConnected = joinNetwork(Config.wifi_ssid.c_str(), 
                                            Config.wifi_Password.c_str(), 
                                            Config.wifi_bssid);
        if(!Status.isWiFiConnected) {
            Appl.MsgBus.sendEvent(this,MSG_SET_RESTART_WIFI,nullptr,WIFI_ON);
            if(Config.autoFallbackMode) {
                Appl.Log.logInfo(F("... fallback to AP Mode"));
                Config.accessPointMode = true;
            } else {
                ApplLogInfo(F("... trying to reconnect to WiFi network"));
            }
        } else {
            Appl.MsgBus.sendEvent(this,MSG_SET_RESTART_WIFI,nullptr,WIFI_OFF);
        }
    }
    writeStatusToLog();
    DEBUG_FUNC_END();
}
*/
void CWiFiController::disableWiFi()
{
    DEBUG_FUNC_START();
    Appl.Log.logInfo(F("Turn wifi off."));
    Appl.MsgBus.sendEvent(this,MSG_DISABLE_WIFI,nullptr,0);
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    Status.stopTimeInMillis = millis();
    Status.isInStationMode = false;
    Status.isInAccessPointMode = false;
    Status.isWiFiConnected = false;
    Status.restartTimeInMillis = -1;
    DEBUG_FUNC_END();
}

