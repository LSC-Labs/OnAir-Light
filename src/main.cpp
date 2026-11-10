#include <Arduino.h>
#include <AppConfig.h>
#include <FileSystem.h>
#include <Appl.h>
#include <Status.h>
#include <Button.h>
#include <SysStatusLed.h>
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
CSysStatusLed   oRgbLed(D5,D6,D7,false);   // status LED - driven by the message bus (MSG_APPL_LOOP)
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
  JsonNode oConfig;
  Appl.writeConfigTo(oConfig,false);
  Serial.printf(" - Config: \n%s\n",oConfig.getAsJsonTextPretty());
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
    Appl.MsgBus.registerEventReceiver(&oRgbLed);
    // WebSocket: dispatches its message queue on MSG_APPL_LOOP and sends
    // bus messages (i.e. WiFi scan results) to the clients
    Appl.MsgBus.registerEventReceiver(&oWebSocket,"WebSocket");
    
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

    oMQTTController.registerHomeAssistantComponent("onair", &oOnAirLight);
    oMQTTController.registerHomeAssistantComponent("bat",   &oBatMeasure); 
    oMQTTController.registerHomeAssistantComponent("wifi",  &oWiFiController);   
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
      if(AppStatus.ScanRF433RequestorId != 0) {
        // Scan Code request from frontend ?
        // JSON_DOC_STATIC(oMsg,512);
        JsonNode oMsg;
        // JsonObject oPayload = LSC::createPayloadStructure("update","rf433code",oMsg);
        JsonNode *pPayload = oMsg.createPayloadStructure("update","rf433code");
        (*pPayload)["on"] = ulData;
        DEBUG_INFO(" - sending received code on websocket...");
        // Send by client id - a disconnected client is skipped by the library
        oWebSocket.text(AppStatus.ScanRF433RequestorId, oMsg.getAsJsonText());
        AppStatus.ScanRF433RequestorId = 0;
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
  if(AppStatus.isRebootPending) {
    Appl.reboot(2000,true);
  } else if(AppStatus.isButtonPressed) {
    if(_buttonPressedTime == 0) _buttonPressedTime = millis();
    // Longer than 5 millis pressed ? => reboot...
    // Send the reboot request as event, so all modules (i.e. the status LED) are informed
    else if((millis() - _buttonPressedTime) > 5 * 1000) Appl.MsgBus.sendEvent(nullptr,MSG_REBOOT_REQUEST,"Button",0);
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
 
    // Register the web routes BEFORE the WiFi is started: the server is started on
    // MSG_WIFI_CONNECTED - requests (i.e. network scanners) must not hit a server,
    // whose routes are still being registered.
    ApplLogInfo(F("..initializing web"));
    registerWebRoutes(oWebServer);  
    registerWebApis(oWebServer);
    oWebServer.addMiddleware(&oCorsMiddleware);
    oWebServer.addMiddleware(&oTestMW);
    oWebServer.addHandler(&oWebSocket);

    // Start the WiFi - with config, if it could be loaded and no button is pressed !
    oWiFiController.startWiFi(AppStatus.configLoaded && !oButton.isPressed());
    oWebServer.begin();         // ignored, if already started by MSG_WIFI_CONNECTED

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

  Appl.dispatch();

    // Appl.dispatch() sends MSG_APPL_LOOP on the message bus - the library modules
    // dispatch themselves there: WebSocket (message queue), MQTT, WiFi (scan, reconnect),
    // WebServer (reboot after OTA), Button, status LED (oRgbLed).

    dispatchRadio433();
 
    // dispatch the requested actions...
    dispatchActions();

    // Set the OnAir Light to desired status
    oOnAirLight.updateLightStatus();

}

