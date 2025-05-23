#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUGINFOS
#endif
#include <StatusHandler.h>
#include <DevelopmentHelper.h>

IStatusHandler * CStatusHandler::getStatusHandler(String strName) {
    
    IStatusHandler *pHandler = nullptr;
    for (const auto& oEntry : tListOfStatusHandler) { 
        if(oEntry.Name == strName) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}

void CStatusHandler::addStatusHandler(String strName, IStatusHandler *pHandler) {
    IStatusHandler *pOrgHandler = getStatusHandler(strName);
    if(!pOrgHandler) {
        StatusHandlerEntry oEntry { strName, pHandler };
        tListOfStatusHandler.push_back(oEntry);
    } 
}

void CStatusHandler::writeStatusTo(JsonObject &oStatusNode){
    for (const auto& oEntry : tListOfStatusHandler) { 
        JsonObject oSubNode = oStatusNode[oEntry.Name];
        if(!oSubNode) oSubNode = oStatusNode.createNestedObject(oEntry.Name);
        if(oEntry.pHandler) {
            DEBUG_INFOS(" - calling status handler : %s",oEntry.Name.c_str());
            oEntry.pHandler->writeStatusTo(oSubNode);
        }
    }
}