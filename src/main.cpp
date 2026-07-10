#include <Arduino.h>
#include <AppConfig.h>
#include <FileSystem.h>
#include <Appl.h>
#include <Status.h>
#include <Button.h>
#include <RGBLed.h>
#include <WiFiController.h>
#include <WebServer.h>
#include <WebRoutes.h>
#include <WebAppApi.h>
#include <WebSocket.h>

#include <MQTTController.h>
#include <BatteryMeasure.h>
#include <RF433Receiver.h>


/**
 * Initialize and open the file system...
 */
CFS oFS;

/**
 *  Application Config and Status objects
 */ 
CStatus         AppStatus;
// CAppConfig      AppConfig;

/**
 * I/O objects
 */
CRGBLed         oRgbLed(D5,D6,D7,false);
CButton         oButton(D4);
COnAirLight     oOnAirLight(D1);
CBatteryMeasure oBatMeasure(A0,4.4);

/**
 * Network communication objects
 */
CRF433Receiver       oRF433Receiver(RADIO_433_RECEIVER_PIN);
CWiFiController      oWiFiController;
CWebServer           oWebServer(80);
AsyncCorsMiddleware  oCorsMiddleware;
CWebSocket           oWebSocket("/ws");
CMQTTController      oMQTTController;
 
// ToDo: Implement the https TLS
// WiFiClientSecure secureClient;


// New middleware classes can be created!
class CMyMiddleware : public AsyncMiddleware {
  public:
    void run(AsyncWebServerRequest *request, ArMiddlewareNext next) override {
      // Before (Request entry)
      next();  // continue middleware chain
      // After (Response exit)
      AsyncWebServerResponse *pResponse = request->getResponse();
      if(pResponse != nullptr) {
        pResponse->addHeader("Copyright",APP_COPYRIGHT " - " APP_AUTHOR);
        pResponse->addHeader("App-Name",APP_NAME);
        pResponse->addHeader("App-Version",APP_VERSION);
      }
    }
  };

CMyMiddleware oTestMW;

#ifdef DEBUGINFOS

void runDebugTests() {
  Serial.println(" ---------------- DEBUG TESTS -");
  Serial.printf("File /config.json exits  : %d\n",oFS.fileExists("/config.json"));
  Serial.printf("File /default.json exits : %d\n",oFS.fileExists("/default.json"));
  Serial.println("Current files:");
  Serial.println(oFS.getFileList());
  Serial.printf("Button status : isLogicalOn == %d\n",oButton.isPressed());
  Serial.println(" ---------------- DEBUG TESTS - END");
}
#endif

/// @brief Register all Appl modules (Status/Config) and initialize the application
/// and load the configuration file, if button is not pressed.
void registerModules() {
    DEBUG_FUNC_START();
    DEBUG_INFO(" - registering modules...");
    Appl.registerModule("web",&oWebServer);
    Appl.registerModule("wifi",&oWiFiController);
    Appl.registerModule("mqtt",&oMQTTController);

    DEBUG_INFO(" - on appl msg bus...");
    Appl.MsgBus.registerEventReceiver(&AppStatus);
    Appl.MsgBus.registerEventReceiver(&oOnAirLight);
    
    DEBUG_INFO(" - on config interface");
    // Register modules with configuration
    // Appl.addConfigHandler("web",   &oWebServer);
    // Appl.addConfigHandler("wifi",  &oWiFiController);
    // Appl.addConfigHandler("mqtt",  &oMQTTController);
    Appl.addConfigHandler("onair", &oOnAirLight);
    // Appl.addConfigHandler("app",   &AppConfig);
    
    DEBUG_INFO(" - on status interface");
    // Register modules for status infos
    // Appl.addStatusHandler("wifi",  &oWiFiController);
    // Appl.addStatusHandler("mqtt",  &oMQTTController);
    Appl.addStatusHandler("app",   &AppStatus);
    Appl.addStatusHandler("onair", &oOnAirLight);
    Appl.addStatusHandler("bat",   &oBatMeasure);
    
    DEBUG_INFO(" - 433");
    Appl.addConfigHandler("rf433", &oRF433Receiver);
    Appl.addStatusHandler("rf433", &oRF433Receiver);

    oMQTTController.registerHomeAssistantComponent("onair",&oOnAirLight);
    DEBUG_FUNC_END();
}

/// @brief Poll and dispatch received 433 MHz messages.
///
/// If the web UI is currently scanning for a remote code, the received value is
/// sent back to the requesting websocket client. Otherwise known codes are
/// translated into on-air events.
void dispatchRadio433() {
  // Only, if the receiver notifies, that a message is available
  if(oRF433Receiver.available()) {
    unsigned long ulData = oRF433Receiver.getReceivedValueOnce(300);
    if(ulData != 0) {
      if(AppStatus.pScanRF433Requestor) {
        // Scan Code request from frontend ?
        // JSON_DOC_STATIC(oMsg,512);
        JsonNode oMsg;
        // JsonObject oPayload = LSC::createPayloadStructure("update","rf433code",oMsg);
        JsonNode *pPayload = oMsg.createPayloadStructure("update","rf433code");
        (*pPayload)["on"] = ulData;
        DEBUG_INFO(" - sending received code on websocket...");
        oWebSocket.sendJsonDocMessage(oMsg,nullptr,AppStatus.pScanRF433Requestor);
        AppStatus.pScanRF433Requestor = nullptr;
      } else {
        if(oRF433Receiver.isEnabled() && oRF433Receiver.hasKey(ulData)) {
          RF433Message oMsg  = oRF433Receiver.getMessage(ulData);
          Appl.MsgBus.sendEvent(nullptr,oMsg.MsgId,"RF433", oMsg.MsgType);      
        }
      }
    }
  }
}

/**
 * @brief Handle deferred actions requested by hardware input or the GUI.
 *
 * Long button presses request a reboot. Short presses cycle through a small set
 * of demo/manual media states. WiFi restarts are delegated to the WiFi module.
 */
void dispatchActions() {
  static unsigned long _buttonPressedTime = 0;
  static int _buttonPressedCount = 0;
  oWiFiController.restartIfNeeded();
  if(AppStatus.isRebootPending) {
    Appl.reboot(2000,true);
  } else if(AppStatus.isButtonPressed) {
    if(_buttonPressedTime == 0) _buttonPressedTime = millis();
    // Longer than 5 millis pressed ? => reboot...
    else if((millis() - _buttonPressedTime) > 5 * 1000) AppStatus.isRebootPending = true;
  } else {
    if(_buttonPressedTime != 0 && (millis() - _buttonPressedTime) < 500) {
      const char *pszID = "Light - Button";
      switch(_buttonPressedCount++) {
        case 0: 
          Appl.MsgBus.sendEvent(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,pszID,ONAIR_DEVICE_ON); 
          break;
        case 1: 
          Appl.MsgBus.sendEvent(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,pszID,ONAIR_DEVICE_OFF); 
          Appl.MsgBus.sendEvent(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO, pszID,ONAIR_DEVICE_ON);
          break;
        default: 
          Appl.MsgBus.sendEvent(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO, pszID,ONAIR_DEVICE_OFF);
          _buttonPressedCount = 0;
          break;
      }
    }
    // ToDo: Switch to next state of Light...
    _buttonPressedTime = 0;
  } 
}

/**
 * @brief Reflect reboot, button and WiFi state on the RGB status LED.
 */
void updateStatusLED() {
  // Reboot and Button Pressed == Prio 1
  if(AppStatus.isRebootPending) {
    oRgbLed.setColor(RGB_COLOR::RED);
  } else if(AppStatus.isButtonPressed) {
    oRgbLed.blink(RGB_COLOR::GREEN,100,100);
  }  else {
    // Normal operation...
    int nBlinkOn = 50;
    int nBlinkOff = 10000;
    int nColor = RGB_COLOR::YELLOW;
    if(oWiFiController.Status.isInAccessPointMode) {
      nColor = RGB_COLOR::BLUE;
    } else if(oWiFiController.Status.isWiFiConnected) {
      nColor = RGB_COLOR::GREEN;
      // Strength of signal to low or not longer available...
      if(!WiFi.isConnected())     { nColor = RGB_COLOR::RED; }
      else if (WiFi.RSSI() < -82) { nColor = RGB_COLOR::YELLOW;}
    }
    oRgbLed.blink(nColor,nBlinkOn,nBlinkOff); 
  }
} 

/// @brief Setup the system
/// - Prepare the filesystem
/// - load configuration (from filesystem)
/// - initialize Appl Framework
/// - initialize the Services:
///   - RGB Light
///   - OnAir Light
///   - WiFi Controller
///   - WebServer and WebSockets
void setup() {
    // Serial Port Setup and Application init...
    // Serial.begin(115200);
    DEBUG_INFOS("\nInitializing application: \"%s\" Version: %s\n",APP_NAME,APP_VERSION);
    // register the modules...
    registerModules();

    // Now init the application and load the configuration.
    Appl.init(APP_NAME, APP_VERSION);
    
    if(!oButton.isPressed()) {
        AppStatus.configLoaded = Appl.readConfigFrom(JSON_APPL_CONFIG_FILE);
    } else {
        AppStatus.configLoaded = false;
    } 

    oRgbLed.setColor(RGB_COLOR::BLUE);

    Appl.sayHello();
    Appl.printDiag();
    ApplLogInfo(F("Initializing services..."));
 
    // Start the WiFi - with config, if it could be loaded and no button is pressed !
    oWiFiController.startWiFi(AppStatus.configLoaded && !oButton.isPressed());

    ApplLogInfo(F("..initializing web"));
    registerWebRoutes(oWebServer);  
    registerWebApis(oWebServer);
    oWebServer.addMiddleware(&oCorsMiddleware);
    oWebServer.addMiddleware(&oTestMW);
    oWebServer.addHandler(&oWebSocket);
    oWebServer.begin();

    ApplLogInfo(F("..initializing mqtt"));
    oMQTTController.setup();
  
    oButton.startMonitoring();
    ApplLogInfo(F("..initializing rf433"));
    oRF433Receiver.setup();

    /// Signal the start of the application
    ApplLogInfo(F("Hello world... - let's start the show!"));
    oRgbLed.showStartupFlashLight(250);

    oOnAirLight.switchOn();
    delay(200);
    oOnAirLight.switchOff();

    #ifdef DEBUGINFOS
        runDebugTests();
    #endif
    Appl.start();
}

void loop() {

    // The WebServer is handling already the GET/POST/WebSocket requests 
    // so look for new messages to be processed on the websocket
    oWebSocket.dispatchMessageQueue();

    oMQTTController.dispatch();
  
    dispatchRadio433();
 
    // Set the status lights / messages
    updateStatusLED();

    // dispatch the requested actions...
    dispatchActions();

    // Set the OnAir Light to desired status
    oOnAirLight.updateLightStatus();

}

