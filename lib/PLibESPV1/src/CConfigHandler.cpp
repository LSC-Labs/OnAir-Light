
#include <Appl.h>
#include <ConfigHandler.h>
#include <ArduinoJson.h>

using namespace std;

/// @brief Joins a ConfigHandler into the ConfigHandler queue...
/// @param strName 
/// @param pHandler 
void CConfigHandler::addConfigHandler(const char *strName, IConfigHandler *pHandler){
    if(pHandler && strName) {
        tListOfEntries.push_back({strName,pHandler});
    }
}

/// @brief  Implement this handler and call back the base class to continue proceeding
/// @param oNode 
void CConfigHandler::writeConfigTo(JsonObject &oNode, bool bHideCritical){
    for (const auto& oEntry : tListOfEntries) { 
        JsonObject oSubNode = oNode[oEntry.strCfgName];
        if(!oSubNode) oSubNode = oNode.createNestedObject(oEntry.strCfgName);
        if(oEntry.pHandler) {
            oEntry.pHandler->writeConfigTo(oSubNode,bHideCritical);
        }
    }
}

void CConfigHandler::readConfigFrom(JsonObject &oNode) {
    DEBUG_FUNC_START();
    DEBUG_INFOS(" - registered handlers : %d",tListOfEntries.size());
    serializeJsonPretty(oNode,Serial);
    for (const auto& oEntry : tListOfEntries) { 
        JsonObject oSubNode = oNode[oEntry.strCfgName];
        if(oSubNode && oEntry.pHandler) {
            DEBUG_INFOS(" - calling IConfigHandler(%s).readConfigFrom()",oEntry.strCfgName);
            oEntry.pHandler->readConfigFrom(oSubNode);
        } else {
            ApplLogWarn(" - invalid Config handler found....");
            if(!oSubNode) ApplLogWarnWithParms("   -> missing entry [%s]in config node",oEntry.strCfgName);
            if(!(oEntry.pHandler)) ApplLogWarn("  -> missing handler pointer");
        }
    }
    DEBUG_FUNC_END();
}

/**
 * Insert your size and call back the base - function...
 */
/*
int CConfigHandler::getJsonConfigSize() {
    int nSize = 0;
    for (auto pEntry = begin (tListOfEntries); pEntry != end (tListOfEntries); ++pEntry) {
        if(pEntry->pHandler) nSize += pEntry->pHandler->getJsonConfigSize();
    }
    return(nSize);
}
*/

IConfigHandler *CConfigHandler::getConfigHandler(const char* strName) {
    IConfigHandler *pHandler = nullptr;
    for (const auto& oEntry : tListOfEntries) { 
        if(strcmp(oEntry.strCfgName,strName)) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}
