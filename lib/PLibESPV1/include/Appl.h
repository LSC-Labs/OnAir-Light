#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EventHandler.h>
#include <Msgs.h>
#include <Logging.h>
#include <Vars.h>
#include <SystemStatusHandler.h>
#include <DevelopmentHelper.h>

#define GetJsonDocumentAsObject(oDoc)         oDoc.as<JsonObject>()

#define ApplLogInfo(oData)          Appl.Log.log("I",oData)
#define ApplLogWarn(oData)          Appl.Log.log("W",oData)
#define ApplLogVerbose(oData)       Appl.Log.log("V",oData)
#define ApplLogError(oData)         Appl.Log.log("E",oData)

#define ApplLogInfoWithParms(str,...)     Appl.Log.log("I",str,__VA_ARGS__)
#define ApplLogVerboseWithParms(str,...)  Appl.Log.log("V",str,__VA_ARGS__)
#define ApplLogWarnWithParms(str,...)     Appl.Log.log("W",str,__VA_ARGS__)
#define ApplLogErrorWithParms(str,...)    Appl.Log.log("E",str,__VA_ARGS__)


#ifdef TRACE
    #define ApplLogTrace(oData)                 Appl.Log.log("T",oData)
    #define ApplLogTraceWithParms(str,...)      Appl.Log.log("T",str,__VA_ARGS__)
#else
    #define ApplLogTraceWithParms(str,...)      ((void*)0)
    #define ApplLogTrace(oData)                 ((void*)0)
#endif

#define JSON_CONFIG_DOC_DEFAULT_SIZE  2048      // Until the new runtime comes in place, this is necessary !
#define JSON_CONFIG_DEFAULT_NAME      "/config.json"

/// @brief Configuration structure of CAppl object
struct ApplConfig {
    bool bLogToSerial       = true;     // Log to serial port
    bool bTraceMode         = false;    // Trace Mode on / off
};

class CAppl : public CConfigHandler, public CStatusHandler {
    ApplConfig m_oCfg;   
    public:
        String AppName;
        String AppVersion;
        const unsigned long StartTime = millis();

    public:

        CSystemStatusHandler SystemStatus;
        CVarTable Config;
        CEventHandler  MsgBus;
        CEventLogger   Log;
        void writeConfigTo(JsonObject &oNode,bool bHideCritical) override;
        // Flashstring is not supported by LittleFS !
        bool saveConfig(const char *pszFileName = JSON_CONFIG_DEFAULT_NAME,  int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files

        void readConfigFrom(JsonDocument &oDoc);
        void readConfigFrom(JsonObject &oNode) override;
        // Flashstring is not supported by LittleFS !
        bool readConfigFrom(const char *pszFileName,                int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files
 
        
        void writeStatusTo(JsonObject &oNode) override;
        void writeSystemStatusTo(JsonObject &oNode);
        String getUpTime();
        

    public:
        CAppl();
        void init(const char *strAppName, const char *strAppVersion);
        void sayHello();
        void printDiag();
        void reboot(int nDelayMillis = 2000, bool bForce = false);
        
};

extern CAppl Appl;
