#include <Appl.h>
#include <OnAirLight.h>
#include <WebAppApi.h>
#include <LSCUtils.h>

// Note: the handlers run in the async context (ESP8266: SYS context, ESP32: AsyncTCP task).
// postEventText() copies the client address and delivers the event in the loop -
// sendEvent() with a payload would call all receivers directly in the async context
// with a pointer to a temporary String.


void registerWebApis(AsyncWebServer &oWebServer) {

    /// Video ON
    oWebServer.on("/video/on", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,strIP.c_str(),ONAIR_DEVICE_ON);
        pRequest->send(200, "text/plain", "OK");
	});
    /// Video OFF
    oWebServer.on("/video/off", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,strIP.c_str(),ONAIR_DEVICE_OFF);
        pRequest->send(200, "text/plain", "OK");
	});
    /// Microphone ON
    oWebServer.on("/audio/on", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO,strIP.c_str(),ONAIR_DEVICE_ON);
        pRequest->send(200, "text/plain", "OK");
	});
    /// Microphone OFF
    oWebServer.on("/audio/off", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO,strIP.c_str(),ONAIR_DEVICE_OFF);
        pRequest->send(200, "text/plain", "OK");
	});
    /// All Media devices ON
    oWebServer.on("/media/on", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,strIP.c_str(),ONAIR_DEVICE_ON);
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO,strIP.c_str(),ONAIR_DEVICE_ON);
        pRequest->send(200, "text/plain", "OK");
	});
    /// All Media devices OFF
    oWebServer.on("/media/off", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        String strIP = pRequest->client()->remoteIP().toString();
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_CAMERA,strIP.c_str(),ONAIR_DEVICE_OFF);
        Appl.MsgBus.postEventText(nullptr,MSG_ONAIR_BASE + ONAIR_MICRO,strIP.c_str(),ONAIR_DEVICE_OFF);
        pRequest->send(200, "text/plain", "OK");
	});

}