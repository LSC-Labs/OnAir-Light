#pragma once
#include <LightSwitch.h>
#include <ConfigHandler.h>
#include <StatusHandler.h>
#include <EventHandler.h>
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

struct OnAirLightConfig {
    int Priority   = ONAIR_CAMERA;
    int OnMicMode  = ONAIR_LIGHT_MODE_BLINK;
    int OnCamMode  = ONAIR_LIGHT_MODE_WAVE;
    unsigned long TimeOutMillis = 3600 * 60 * 12; // 12h
    // Brightness is inside the CLightSwitch
};

struct OnAirLightStatus {
    bool isMicOn = false;
    bool isCamOn = false;
    unsigned long ulLastUpdate = 0;
    // Helper fucntions..
    void setState(const char *pszDevice,const char *pszMode);
    bool isTimeOutReached(unsigned long ulTimeOutMillis);
};
/*
struct OnAirLightClient {
   
    bool isMicOn = false;
    bool isCamOn = false;
    String strAddress;      // Client address...
};
*/

class COnAirLight : public CLightSwitch, public IConfigHandler, public IStatusHandler, public IMsgEventReceiver{
    private:
        unsigned long _ulWaveFadeIn  = 400;
        unsigned long _ulWaveFadeOut = 400;
        unsigned long _ulWaveOnTime  = 1000;
        unsigned long _ulWaveOffTime = 1000;
        std::map<String,OnAirLightStatus> tClientStaties;

    public:
        // OnAirLightStatus Status;
        OnAirLightConfig Config;
        int ButtonLightStatus = ONAIR_LIGHT_MODE_OFF;
    public:
        COnAirLight(int nPin);
        int receiveEvent(void *pSender, int nMsgId, const void*pMsg, int nType) override;
        void readConfigFrom(JsonObject &oCfg) override;
        void writeConfigTo(JsonObject &oCfg, bool bHideCritical) override;
        void writeStatusTo(JsonObject &oStatus) override;
        void updateLightStatus();
        void dispatchBrokerMessage(const char *pszMessage, int nLen);  
        bool isCamOn();
        bool isMicOn();

    private:
        int getModeByName(String strMode,int nDefault);
        String setNameOfMode(JsonObject &oCfg, const char *pszKey, int nMode);
        void setClientStatus(String strClientAddress, const char *pszMode, const char *pszCommand);
};
