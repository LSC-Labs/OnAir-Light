#pragma once

#include <stdarg.h>
#include <EventHandler.h>
#include <Msgs.h>

/*
typedef void (*funcLogWriter)(const char* strType, const char* strModuleName, const char* strMessage);

struct LogMessage {
    String strType;
    JsonDocument oData;
    String strMessage;
};
*/
class CEventLogger {
    private:
        CEventHandler *pEventHandler;
    public:
        CEventLogger();
        CEventLogger(CEventHandler *oEventHandler);

        static int  getLogClassNumberFrom(const char * pszClassString);
        static char getClassCharFromLogClass(int nClassNo);

        void logInfo(const char* strMessage,...);
        void logVerbose(const char* strMessage,...);
        void logWarning(const char* strMessage,...);
        void logError(const char* strMessage,...);
        void logTrace(const char* strMessage,...);

        void logInfo(const __FlashStringHelper*,...);
        void logVerbose(const __FlashStringHelper*,...);
        void logWarning(const __FlashStringHelper*,...);
        void logError(const __FlashStringHelper*,...);
        void logTrace(const __FlashStringHelper*,...);

        void log(const char* pszType, const __FlashStringHelper* pszMessage, ...);
        void log(const char* pszType, const char *pszMessage, ...);
        void log(const char* strType, JsonDocument *pDoc);
};


/// @brief Class to handle the logging for a module
///      This class is used to write log entries for a specific module
class CLogWriter : public IMsgEventReceiver {
    
    private:
        void *pSender;
        CEventHandler *pEventHandler;
        const char* m_strModuleName;
        
    public:
        CLogWriter(){};
        virtual int  receiveEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) override;   
        virtual void writeLogEntry(const char *strType, const char *strMessage);
        virtual void writeLogEntry(const char *strType, JsonDocument *oDoc);
};

class CSerialLogWriter : public CLogWriter {
    public:
        void writeLogEntry(const char *strType, const char *strMessage) override;
        void writeLogEntry(const char *strType, JsonDocument *pDoc) override;
};
