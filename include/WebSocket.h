#pragma once
#include <NetworkModules.h>
#include <ArduinoJson.h>

// class CWebSocket;

/// @brief Web Socket Message for message queue...
struct WsMessage
{
	char *serializedMessage;
	// CWebSocket           *pWebSocket;
	AsyncWebSocket       *pSocket;
	AsyncWebSocketClient *pClient;
	WsMessage *nextMessage = NULL;
};

/// @brief Web Socket status info
struct WebSocketStatus {
	long uptime = millis();
};


class CWebSocket : public AsyncWebSocket {
	private:
		WebSocketStatus Status;
		// WebSocket Message Queue
		WsMessage *pMsgQueue;
		String strNeedsAuth =  "getconfig,saveconfig,getbackup,restorebackup,restart,factoryreset";
		bool inline needsAuth(String &strCommand);
		bool checkAuth(JsonDocument &oRequestDoc, AsyncWebSocketClient *pClient);
	public:
		CWebSocket(const char* strSocketName);
		void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);
		void dispatchMessageQueue();
		void addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize);
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient);
		
	private:
		void dispatchMessage(WsMessage *pMessage);
		void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonDocument &oDoc,AsyncWebSocketClient *pClient);
		void ICACHE_FLASH_ATTR sendStatus(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient);
		void onWiFiScanResult(int nCount);

		public:
		JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszData = nullptr);
		JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc,const char *pszData = nullptr);
		
};
