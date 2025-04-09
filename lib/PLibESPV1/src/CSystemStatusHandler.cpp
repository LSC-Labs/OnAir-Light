
#ifndef DEBUG_LSC_APPL
    #undef DEBUGINFOS
#endif
#include <SystemStatusHandler.h>
#include <LittleFS.h>
#include <DevelopmentHelper.h>

void CSystemStatusHandler::writeStatusTo(JsonObject &oStatusObj) {
    DEBUG_FUNC_START_PARMS("%s",oStatusObj ? "OK" : "-null-");
    FSInfo oFsInfo;
    LittleFS.info(oFsInfo);
    uint32_t nFreeHeap;
    uint32_t nMaxHeap;
    uint8_t  nFragHeap;
    
    ESP.getHeapStats(&nFreeHeap,&nMaxHeap,&nFragHeap);

    oStatusObj["full_ver"]          = ESP.getFullVersion();
    oStatusObj["chip_id"]           = String(ESP.getChipId(), HEX);
    oStatusObj["cpu_clock"]         = ESP.getCpuFreqMHz();
    oStatusObj["core_ver"]          = ESP.getCoreVersion();
    oStatusObj["flash_size"]        = ESP.getFlashChipSize();
    oStatusObj["flash_real_size"]   = ESP.getFlashChipRealSize();    
    oStatusObj["heap_free"]         = ESP.getFreeHeap(); 
    oStatusObj["heap_max"]          = nMaxHeap;
    oStatusObj["sketch_size"]       = ESP.getSketchSize();
    oStatusObj["sketch_free_size"]  = ESP.getFreeSketchSpace(); // availsize - should be able to cover a new sketch
    oStatusObj["fs_total"]          = oFsInfo.totalBytes;
    oStatusObj["fs_used"]           = oFsInfo.usedBytes;

    DEBUG_FUNC_END();
}