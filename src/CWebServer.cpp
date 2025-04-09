#ifndef DEBUG_LSC_WEBSERVER
    #undef DEBUGINFOS
#endif

#include <WebServer.h>
#include <Appl.h>
// #include <Config.h>
// #include <JsonHandler.h>
#include <WebSocket.h>
#include <FileSystem.h>
#include <Security.h>
#include <LSCUtils.h>


// these are from vendors
#include "webh/glyphicons-halflings-regular.woff.gz.h"
#include "webh/required.css.gz.h"
#include "webh/required.js.gz.h"
#include "webh/index.js.gz.h"

// these are from us which can be updated and changed
#include "webh/main.js.gz.h"
#include "webh/main.htm.gz.h"
#include "webh/index.html.gz.h"


#define WEBSERVER_HIDDEN_PASSWORD       "******"
#define WEBSERVER_CONFIG_PASSWORD_KEY   "httpPasswd"

namespace WebServer {
    WebServerConfig Config;
}
// extern AppConfig oConfig;

// WsMessage   *_wsMessageQueue; 

CWebServer::CWebServer(int nPortNumber, funcRegisterRoutes pRegisterUserRoutes) 
	: AsyncWebServer(nPortNumber) {
    if (WebServer::Config.Passwd == "") {
		WebServer::Config.Passwd = "admin";
	}

    registerRoutes();

    if(pRegisterUserRoutes) {
        pRegisterUserRoutes(*this);
    }
}

void CWebServer::readConfigFrom(JsonObject &oNode) {
    String strPasswd = oNode[WEBSERVER_CONFIG_PASSWORD_KEY];
    if(!strPasswd.equals(WEBSERVER_HIDDEN_PASSWORD)) {
        WebServer::Config.Passwd = strPasswd;
    }
}

void CWebServer::writeConfigTo(JsonObject &oCfgNode,bool bHideCritical) {
    oCfgNode[WEBSERVER_CONFIG_PASSWORD_KEY] = bHideCritical ? WEBSERVER_HIDDEN_PASSWORD : WebServer::Config.Passwd;
}

void CWebServer::registerRoutes() {

    onNotFound([](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "Not found");
		request->send(response);
	});

#pragma region Default pages and includes

    on("/fonts/glyphicons-halflings-regular.woff", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "font/woff", glyphicons_halflings_regular_woff_gz, glyphicons_halflings_regular_woff_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
	on("/css/required.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", required_css_gz, required_css_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
	on("/js/required.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", required_js_gz, required_js_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
	on("/js/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", main_js_gz, main_js_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

    on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", index_js_gz, index_js_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
    on("/main.htm", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", main_htm_gz, main_htm_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
    /*
    WebServerConfig *pConfig = &this->Config;
*/
	on("/login", HTTP_GET, [](AsyncWebServerRequest *pRequest) {    
        if (!WebServer::Config.authenticate(pRequest,"login",true)) {
            return pRequest->requestAuthentication();
		}
        AsyncWebServerResponse *response = pRequest->beginResponse(200, "text/plain", "Success");
        setNewAuthHeader(pRequest,response);
        ApplLogInfoWithParms(F("Login from address %s"),pRequest->client()->remoteIP().toString().c_str());
		pRequest->send(response);
	});

    rewrite("/", "/index.html");

#pragma endregion


    on("/status", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        DEBUG_INFO("/status route called...");
        DynamicJsonDocument oStatusDoc(2048);
        JsonObject oPayload = oStatusDoc.createNestedObject("payload");
        Appl.writeStatusTo(oPayload);
        // Do it the old school way - do not use String here... Webserver destroys string info...
        int nSize = measureJson(oStatusDoc);
        char tBuffer[nSize*2];
        memset(tBuffer,'\0',sizeof(tBuffer));
        serializeJson(oPayload,&tBuffer,nSize*2);
        DEBUG_INFOS("WEB:/status -> %s",tBuffer);
        AsyncWebServerResponse *response = pRequest->beginResponse_P(200, "application/json",  tBuffer);
        pRequest->send(response);
    });

#pragma region Firmware Update
    on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        ApplLogInfo(F("WEB:/update"));
		AsyncWebServerResponse * response = request->beginResponse(200, "text/plain", "OK");
		response->addHeader("Connection", "close");
		request->send(response);
	}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
		/*
        if (!WebServer::Config.authenticate(request,"update (firmware)", true)) {
            ApplLogError(F("... not authenticated !!! - update rejected"));
            return request->requestAuthentication();
		}
            */
		if (!index) {
            ApplLogInfoWithParms(F("Firmware update started : %s"),filename.c_str());
			Update.runAsync(true);
			if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
				ApplLogError(F("Not enough space..."));
			}
		}
		if (!Update.hasError()) {
			if (Update.write(data, len) != len) {
                ApplLogErrorWithParms(F("Writing to flash failed..."), filename.c_str());
                ApplLogErrorWithParms(F("%s"),Update.getErrorString().c_str());
			}
		}
		if (final) {
			if (Update.end(true)) {
                ApplLogInfoWithParms(F("Firmware update finished (%uB)"),index + len);
                if(!Update.hasError()) Appl.MsgBus.sendEvent(nullptr,MSG_REBOOT_REQUEST,F("Firmware update"),0);
			} else {
                ApplLogError(F("Firmware update failed."));
                ApplLogErrorWithParms(F("%s"),Update.getErrorString().c_str());
			}
		}
	});
#pragma endregion

#pragma region Upload and list files pages...

	// Webserver - Startseite
    on("/files/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!WebServer::Config.authenticate(request,"files/upload",true)) {
            return request->requestAuthentication();
		}
        request->send(200, "text/html", "<form method='POST' action='/files/upload' enctype='multipart/form-data'>"
                                        "<input type='file' name='file'>"
                                        "<input type='submit' value='Upload'>"
                                        "</form>");
    });

	 // Upload-Handler
    on("/files/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            if (!WebServer::Config.authenticate(request,"files/upload",true)) {
                return request->requestAuthentication();
		    }
            request->send(200);
        }, 
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            static File file;
            if (index == 0) {  // Neue Datei öffnen
                Serial.printf("Speichere Datei: %s\n", filename.c_str());
                file = LittleFS.open("/" + filename, "w");
            }
            if (file) {
                file.write(data, len);
            }
            if (final) {  // Datei schließen
                file.close();
                Serial.printf("Upload abgeschlossen: %s\n", filename.c_str());
            }
        }
    );
	
	// Liste der Dateien anzeigen
    on("/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!WebServer::Config.authenticate(request,"files/list",true)) {
            return request->requestAuthentication();
		}
        String strFileList = getFileList("/");
        request->send(200, "application/json", strFileList);
    });

#pragma endregion END File Pages
}


