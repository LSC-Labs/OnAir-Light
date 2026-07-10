#pragma once
#include <LightSwitch.h>
#include <ModuleInterface.h>
#include <MQTTController.h>
#include <AppMsgs.h>
#include <map>

#define ONAIR_CAMERA 0
#define ONAIR_MICRO  1
#define ONAIR_DEVICE_UPPER_LIMIT  2    // Set to the next free device enum 


#define ONAIR_DEVICE_ON  1
#define ONAIR_DEVICE_OFF 0

#define ONAIR_LIGHT_MODE_OFF     0
#define ONAIR_LIGHT_MODE_ON      1
#define ONAIR_LIGHT_MODE_BLINK   2
#define ONAIR_LIGHT_MODE_WAVE    3
#define ONAIR_LIGHT_BRIGHTNESS_DEFAULT 30
// #define MSG_SELECT_NEXT_LIGHT_STATE (MSG_USER_BASE + 300)

/**
 * @brief User configurable behavior for the on-air light.
 *
 * Camera and microphone activity can map to different light modes. If both are
 * active at the same time, Priority decides which mode wins.
 */
struct OnAirLightConfig {
    int Priority   = ONAIR_CAMERA;
    int OnMicMode  = ONAIR_LIGHT_MODE_BLINK;
    int OnCamMode  = ONAIR_LIGHT_MODE_WAVE;
    unsigned long TimeOutMillis = 1000 * 60 * 60 * 12; // 12h
    // Brightness is inside the CLightSwitch
};

/**
 * @brief Last known media state for one reporting client.
 *
 * Each client can independently report microphone and camera state. The
 * timestamp allows stale clients to be ignored after the configured timeout.
 */
struct OnAirLightStatus {
    bool isMicOn = false;
    bool isCamOn = false;
    unsigned long ulLastUpdate = 0;
    // Helper fucntions..
    bool setState(const char *pszDevice,const char *pszMode);
    bool isTimeOutReached(unsigned long ulTimeOutMillis);
};
/*
struct OnAirLightClient {
   
    bool isMicOn = false;
    bool isCamOn = false;
    String strAddress;      // Client address...
};
*/

/**
 * @brief Controls the physical on-air light from web, MQTT, button and RF events.
 *
 * The class aggregates per-client camera/microphone state, exposes the state for
 * status and Home Assistant discovery, and translates the effective state into
 * the configured light mode.
 */
class COnAirLight : public CLightSwitch, public IModule, public IHomeAssistantComponent{
    private:
        bool m_bNeedsHAUpdate = true;       // Initial state needs to be published to HA
        unsigned long _ulWaveFadeIn  = 2000;
        unsigned long _ulWaveFadeOut = 2000;
        unsigned long _ulWaveOnTime  =  100;
        unsigned long _ulWaveOffTime = 1000;
        std::map<String,OnAirLightStatus> tClientStaties;
        const char* m_szHA_DeviceName = "HomeAssistant";
        int writeHAValueSelectTo(char *pszBuffer, size_t nBufferSize, const char *pszMediaType) {
            return(snprintf(pszBuffer,nBufferSize,
                "{{ 'ON' if value_json.get('onair', {}).get('clients', {}).get('%s', {}).get('is%sOn') else 'OFF' }}",
                m_szHA_DeviceName,pszMediaType));
        }
    public:
        // OnAirLightStatus Status;
        OnAirLightConfig Config;
        int ButtonLightStatus = ONAIR_LIGHT_MODE_OFF;
    public:
        COnAirLight(int nPin);
        int receiveEvent(const void *pSender, int nMsgId, const void*pMsg, int nMsgInfo) override;
        void readConfigFrom(CJsonNode &oCfg) override;
        void writeConfigTo(CJsonNode &oCfg, bool bHideCritical) override;
        void writeStatusTo(CJsonNode &oStatus,int nLevel = STATUS_LEVEL_INFO) override;
        void insertComponentDiscovery(const char *pszCompName,JsonNode & oComponentArea, CMQTTController * pController);
        void updateLightStatus();
        void dispatchBrokerMessage(MQTTMessage *pMsg);  
        bool isCamOn();
        bool isMicOn();

    private:
        int getModeByName(String strMode,int nDefault);
        String setNameOfMode(CJsonNode &oCfg, const char *pszKey, int nMode);
        bool setClientStatus(String strClientAddress, const char *pszMode, const char *pszCommand);
};
