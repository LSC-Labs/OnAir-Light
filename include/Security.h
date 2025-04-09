#pragma once

#include <Arduino.h>
#include <NetworkModules.h>

void setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse);
String getNewAuthToken(String &strIPAddress);
bool isAuthTokenValid(String &strEncBase64Token, String &strRemoteIPAddress);

