#include <AppConfig.h>
#include <JsonNode.h>



void CAppConfig::readConfigFrom(CJsonNode &oCfg) {
    AutoRestartTime = oCfg.getValueAsUnsignedLong("autorestart",AutoRestartTime);
    //storeSValueIF(&(AppConfig::AutoRestartTime),oCfg["autorestart"]);
}

void CAppConfig::writeConfigTo(CJsonNode &oCfg, bool bHideCritical) {
    oCfg["autorestart"] = AppConfig::AutoRestartTime;
}
