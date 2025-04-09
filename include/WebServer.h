#pragma once
#include <Appl.h>
#include <LSCUtils.h>
#include <NetworkModules.h>
#include <WebSocket.h>
#include <ConfigHandler.h>




    struct WebServerConfig {
        String UserName = "admin";
        String Passwd   = "admin";
        bool authenticate(AsyncWebServerRequest *pRequest, const char* pszReason, bool bLogFailed) {
            bool bAuthenticated = pRequest->authenticate(UserName.c_str(),Passwd.c_str());
            if(!bAuthenticated && bLogFailed) {
                IPAddress oRemoteIP = pRequest->client()->remoteIP();
                ApplLogWarnWithParms(F("unauthorized web access (%s): %s"),
                                       oRemoteIP.toString().c_str(),
                                       pszReason ? pszReason : "-");
            }
            return(bAuthenticated);
        }
    };


    
//     extern WebServerConfig Config;

class CWebServer;

/// @brief Function pointer to register the routes
typedef void (funcRegisterRoutes)(AsyncWebServer &oWebServer);

/// @brief Class to handle the WebServer and to register the routes
class CWebServer : public AsyncWebServer, public IConfigHandler {
    public:
        int nVersion = 1;
        WebServerConfig Config;
        // AsyncWebServer      m_oWebServer;
        // CWebSocket          m_oWebSocket;
    public:
        CWebServer(int nPortNumber,funcRegisterRoutes registerUserRoutes = nullptr);
        void writeConfigTo(JsonObject &oNode, bool bHideCritical)  override; 
        void readConfigFrom(JsonObject &oNode) override;
    private:
        void registerRoutes();
};
