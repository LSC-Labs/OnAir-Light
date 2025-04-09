#include <Arduino.h>
#include <AppConfig.h>
#include <FileSystem.h>
#include <Appl.h>
#include <Status.h>
#include <Button.h>
#include <RGBLed.h>
#include <WiFiController.h>
#include <WebServer.h>
#include <WebSocket.h>
#include <AppWebApi.h>
#include <MQTTController.h>
#include <BatteryMeasure.h>
#include <RF433Receiver.h>


/**
 *  Application Config and Status objects
 */ 
CStatus         AppStatus;
CAppConfig      AppConfig;

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
CRF433Receiver  oRF433Receiver(RADIO_433_RECEIVER_PIN);
CWiFiController      oWiFiController;
CWebServer           oWebServer(80,registerWebRoutes);
AsyncCorsMiddleware  oCorsMiddleware;
CWebSocket           oWebSocket("/ws");
CMQTTController             oMQTTController;
 
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
  Serial.printf("File /config.json exits  : %d\n",fileExists("/config.json"));
  Serial.printf("File /default.json exits : %d\n",fileExists("/default.json"));
  Serial.println(getFileList());
  /*
  Serial.println("checking encryption...");
  String strIP = "1.2.3.4";
  String strToken = getNewAuthToken(strIP);
  bool bResult = isAuthTokenValid(strToken,strIP);
  Serial.printf("--> result : %d\n",bResult);
  */
  Serial.println(" ---------------- DEBUG TESTS - END");
}
#endif

/// @brief Register all Appl modules (Status/Config) and initialize the application
/// and load the configuration file, if button is not pressed.
void setupAppl(bool bLoadConfig = true) {
  // Register modules to listen on the Appl-Message bus
  Appl.MsgBus.registerEventReceiver(&AppStatus);
  Appl.MsgBus.registerEventReceiver(&oOnAirLight);
  
  // Register modules with configuration
  Appl.addConfigHandler("web",   &oWebServer);
  Appl.addConfigHandler("wifi",  &oWiFiController);
  Appl.addConfigHandler("onair", &oOnAirLight);
  Appl.addConfigHandler("app",   &AppConfig);
  Appl.addConfigHandler("mqtt",  &oMQTTController);
  
  // Register modules for status infos
  Appl.addStatusHandler("wifi",  &oWiFiController);
  Appl.addStatusHandler("app",   &AppStatus);
  Appl.addStatusHandler("onair", &oOnAirLight);
  Appl.addStatusHandler("mqtt",  &oMQTTController);
  Appl.addStatusHandler("bat",   &oBatMeasure);
  
  Appl.addConfigHandler("rf433", &oRF433Receiver);
  Appl.addStatusHandler("rf433", &oRF433Receiver);

  // Now init the application and load the configuration.
  Appl.init(APP_NAME, APP_VERSION);
  
  if(bLoadConfig) {
    AppStatus.configLoaded = Appl.readConfigFrom(fileExists(CONFIG_FILE) ? CONFIG_FILE : "/default.json");
  } else {
    AppStatus.configLoaded = false;
  }
}

/// @brief Dispatch the received 433 MHz messages.
void dispatchRadio433() {
  // Only, if the receiver notifies, that a message is available
  if(oRF433Receiver.available()) {
    unsigned long ulData = oRF433Receiver.getReceivedValueOnce(300);
    if(ulData != 0) {
      if(AppStatus.pScanRF433Requestor) {
        // Scan Code request from frontend ?
        StaticJsonDocument<512> oMsg;
        JsonObject oPayload = oWebSocket.createPayloadStructure("update","rf433code",oMsg);
        oPayload["on"] = ulData;
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
 * Actions requested by user / GUI ?
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
    if(_buttonPressedTime + 500 > millis()) {
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
 * Set the status LED to the desired state
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
  Serial.begin(115200);
  DEBUG_INFOS("\nInitializing application: \"%s\" Version: %s\n",APP_NAME,APP_VERSION);
  setupFS();
  bool bButtonIsPressed = oButton.isPressed();
  setupAppl(!bButtonIsPressed);

  oRgbLed.setColor(RGB_COLOR::BLUE);

  Appl.sayHello();
  Appl.printDiag();
  Appl.Log.logInfo(F("Initializing services..."));
 
  // Start the WiFi - with config, if it could be loaded and no button is pressed !
  oWiFiController.startWiFi(AppStatus.configLoaded && !oButton.isPressed());

  
  oWebServer.addMiddleware(&oCorsMiddleware);
  oWebServer.addMiddleware(&oTestMW);
  oWebServer.addHandler(&oWebSocket);
  oWebServer.begin();
  
  oMQTTController.setup();
  
  oButton.startMonitoring();
  oRF433Receiver.setup();

  /// Signal the start of the application
  Appl.Log.logInfo(F("Hello world... - let's start the show!"));
  oRgbLed.showStartupFlashLight(250);

  oOnAirLight.switchOn();
  delay(200);
  oOnAirLight.switchOff();

  #ifdef DEBUGINFOS
    runDebugTests();
  #endif
}

void loop() {

  // The WebServer is handling already the GET/POST/WebSocket requests 
  // so look for new messages to be processed on the websocket
  oWebSocket.dispatchMessageQueue();

  oMQTTController.publishHeartBeat();

  dispatchRadio433();
 
  // Set the status lights / messages
  updateStatusLED();

  // dispatch the requested actions...
  dispatchActions();

  // Set the OnAir Light to desired status
  oOnAirLight.updateLightStatus();

}


