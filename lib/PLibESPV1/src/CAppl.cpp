#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Logging.h>
#include <ConfigHandler.h>
#include <FileSystem.h>
#include <LSCUtils.h>

CAppl::CAppl() {
    Log = CEventLogger(&MsgBus);
}  

void CAppl::init(const char *strAppName, const char *strAppVersion) {
    AppName 	= strAppName;
    AppVersion  = strAppVersion;
    if(m_oCfg.bLogToSerial) {
        MsgBus.registerEventReceiver(new CSerialLogWriter());
    }
	MsgBus.sendEvent(this,MSG_APPL_INITIALIZED,nullptr,0);
}

void CAppl::reboot(int nDelay, bool bForce) {
	Log.log("W",F("Reboot requested..."));
	int nResult = MsgBus.sendEvent(this,MSG_APPL_SHUTDOWN,nullptr,0);
	if(nResult == EVENT_MSG_RESULT_OK || bForce) {
		Log.log("W",F("Restarting system..."));
		delay(nDelay);
		ESP.restart();
	}
}

String CAppl::getUpTime() {

	unsigned long ulUpTime = millis() - this->StartTime;
	// 3600000 milliseconds in an hour
	long nHours   = ulUpTime / 3600000; 		// 1000 * 60 * 60
	ulUpTime = ulUpTime - 3600000 * nHours;		// Remove hours

	// 60000 milliseconds in a minute
	long nMinutes = ulUpTime / 60000;			// 1000 * 60
	ulUpTime = ulUpTime - 60000 * nMinutes;		// Remove minutes

	// 1000 milliseconds in a second		
	long nSeconds = ulUpTime / 1000;			// 1000 ms
	
	char tUpTimeBuffer[20];
	sprintf(tUpTimeBuffer, "%02lu:%02lu:%02lu", nHours, nMinutes, nSeconds);
	return(String(tUpTimeBuffer));
}

void CAppl::sayHello() {
	char strBuff[100];
	sprintf(strBuff, "%s v%s", AppName.c_str(), AppVersion.c_str());
	Serial.println("");
	Serial.println(strBuff);
	for(size_t i = 0; i < strlen(strBuff); i++) {
		Serial.print("=");
	}
	Serial.println("");
}   

void CAppl::writeSystemStatusTo(JsonObject &oStatusObj) {
    DEBUG_FUNC_START();
	if(!oStatusObj.isNull()) {
		FSInfo oFsInfo;
		LittleFS.info(oFsInfo);
		uint32_t nFreeHeap;
		uint32_t nMaxHeap;
		// static uint8_t  nFragHeap;
		ESP.getHeapStats(&nFreeHeap,&nMaxHeap,nullptr);

		oStatusObj["full_ver"]          = ESP.getFullVersion();
		oStatusObj["chip_id"]           = String(ESP.getChipId(), HEX);
		oStatusObj["cpu_clock"]         = ESP.getCpuFreqMHz();
		oStatusObj["core_ver"]          = ESP.getCoreVersion();
		oStatusObj["flash_size"]        = ESP.getFlashChipSize();
		oStatusObj["flash_real_size"]   = ESP.getFlashChipRealSize();    
		oStatusObj["heap_free"]         = nFreeHeap; // ESP.getFreeHeap(); 
		oStatusObj["heap_max"]          = nMaxHeap;
		oStatusObj["sketch_size"]       = ESP.getSketchSize();
		oStatusObj["sketch_free_size"]  = ESP.getFreeSketchSpace(); // availsize - should be able to cover a new sketch
		oStatusObj["fs_total"]          = oFsInfo.totalBytes;
		oStatusObj["fs_used"]           = oFsInfo.usedBytes;
		oStatusObj["fs_block_size"]     = oFsInfo.blockSize;
	}
    DEBUG_FUNC_END();
}
/// @brief write the current configuration into the file system.
///        a current config file will be loaded first to avoid loosing unknown config data...
///        To get a clean config file, delete it first.
/// @param pszConfigFileName The configuration FileName (Default is /config.json)
/// @param nJsonDocSize      The size of the expected total size, othterwies JSON_CONFIG_DEFAULT_SIZE is used.
/// @return true, if the file could be written...
bool CAppl::saveConfig(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	if(nJsonDocSize < 10)  nJsonDocSize 	 = JSON_CONFIG_DOC_DEFAULT_SIZE;
    DynamicJsonDocument oCfgDoc(nJsonDocSize);
	// load existing config file from the file system first, to keep unknown settings in place
	loadJsonContentFromFile(pszConfigFileName,oCfgDoc);
	JsonObject oCfgNode = oCfgDoc.to<JsonObject>();
	writeConfigTo(oCfgNode,false);
	bool bResult = saveJsonContentToFile(pszConfigFileName,oCfgDoc);
	DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "ERROR");
    return(bResult);
}

void CAppl::writeConfigTo(JsonObject &oJsonObj, bool bHideCritical) {
	Config.writeConfigTo(oJsonObj,bHideCritical);
	oJsonObj["logToSerial"] = m_oCfg.bLogToSerial;
	oJsonObj["traceMode"] = m_oCfg.bTraceMode;

	// Iterate through registered Config Handler
	CConfigHandler::writeConfigTo(oJsonObj,bHideCritical);
}

void CAppl::writeStatusTo(JsonObject &oStatusObj) {
	DEBUG_FUNC_START();
	oStatusObj["prog_name"] 	= AppName;
	oStatusObj["prog_version"] 	= AppVersion;
	oStatusObj["uptime"] 		= getUpTime();
	oStatusObj["starttime"]     = StartTime;
	oStatusObj["now"] 			= millis();	
	#ifdef DEBUG_LSC_HTLM_PAGES
		oStatusObj["DebugMode"]     = "1";
	#endif
	writeSystemStatusTo(oStatusObj);
	// Iterate through additional registered Status Handler
	CStatusHandler::writeStatusTo(oStatusObj);
	// DEBUG_INFO(" - status collected...");
	// DEBUG_JSON_OBJ(oStatusObj);
	DEBUG_FUNC_END();
}

void CAppl::readConfigFrom(JsonObject &oJsonObj) {
	DEBUG_FUNC_START();
	// Config.readConfigFrom(oJsonObj);
	// Speed oper
	storeValueIF(&m_oCfg.bLogToSerial, oJsonObj["logToSerial"]);
	storeValueIF(&m_oCfg.bTraceMode,   oJsonObj["traceMode"]);
	// Iterate through registerd Config Handler
	CConfigHandler::readConfigFrom(oJsonObj);
	DEBUG_FUNC_END();
}

void CAppl::readConfigFrom(JsonDocument &oJsonDoc) {
	JsonObject oCfgData = oJsonDoc.as<JsonObject>();
	readConfigFrom(oCfgData);
}

bool CAppl::readConfigFrom(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	if(nJsonDocSize < 10)  nJsonDocSize      = JSON_CONFIG_DOC_DEFAULT_SIZE;
    DynamicJsonDocument oCfgDoc(nJsonDocSize);
    bool bResult = false;
    if(loadJsonContentFromFile(pszConfigFileName,oCfgDoc)) {
		serializeJsonPretty(oCfgDoc,Serial);
        readConfigFrom(oCfgDoc);
		bResult = true;
    }
	DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}

/// @brief print some diagnostic information
/// https://42project.net/groesse-des-esp8266-flash-speicher-sowie-chip-id-ausgeben-und-mit-der-konfiguration-ueberpruefen/
void CAppl::printDiag() {
    uint32_t    realSize = ESP.getFlashChipRealSize();
	uint32_t    ideSize  = ESP.getFlashChipSize();
	FlashMode_t ideMode  = ESP.getFlashChipMode();
	Serial.printf("Flash real id     : %08X\n", ESP.getFlashChipId());
	Serial.printf("Flash real size   : %u\n\n", realSize);
	Serial.printf("Flash ide  size   : %u\n", ideSize);
	Serial.printf("Flash ide speed   : %u\n", ESP.getFlashChipSpeed());
	Serial.printf("Flash ide mode    : %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT"
																	     : ideMode == FM_DIO  ? "DIO"
																	     : ideMode == FM_DOUT ? "DOUT"
																		 : "UNKNOWN"));
	Serial.printf("Free heap size    : %u\n", ESP.getFreeHeap());
	Serial.printf("Free stack content: %u\n", ESP.getFreeContStack());

	if (ideSize != realSize)
	{
		Serial.println("Flash Chip configuration wrong!\n");
	}
	else
	{
		Serial.println("Flash Chip configuration ok.\n");
	}
}

/// @brief The global application object
CAppl Appl;

