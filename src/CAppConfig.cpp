#include <AppConfig.h>
#include <JsonHelper.h>



void CAppConfig::readConfigFrom(JsonObject &oCfg) {
    LSC::setJsonValue(oCfg,"autorestart",&AutoRestartTime);
    //storeSValueIF(&(AppConfig::AutoRestartTime),oCfg["autorestart"]);
}

void CAppConfig::writeConfigTo(JsonObject &oCfg, bool bHideCritical) {
    oCfg["autorestart"] = AppConfig::AutoRestartTime;
}