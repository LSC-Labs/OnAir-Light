#pragma once

/**
 * Include in this section all network components
 * Cause WiFiManager and AsyncWebServer may conflict in definitions !
 * So don't include them explicit - use this module to avoid conflicts.
 */

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
//  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
