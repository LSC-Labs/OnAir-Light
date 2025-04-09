#include <AppConfig.h>
#include <LSCUtils.h>


void CAppConfig::readConfigFrom(JsonObject &oCfg) {
    storeValueIF(&(AppConfig::AutoRestartTime),oCfg["autorestart"]);
}

void CAppConfig::writeConfigTo(JsonObject &oCfg, bool bHideCritical) {
    oCfg["autorestart"] = AppConfig::AutoRestartTime;
}