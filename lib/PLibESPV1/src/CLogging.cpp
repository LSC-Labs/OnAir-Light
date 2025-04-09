#ifndef DEBUG_LSC_LOGGING
    #undef DEBUGINFOS
#endif
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Logging.h>
#include <Msgs.h>
#include <DevelopmentHelper.h>

#define LOG_ENTRY_BUFFER_SIZE  512

#pragma region Implementation of CEventLogger

CEventLogger::CEventLogger() {}
CEventLogger::CEventLogger(CEventHandler *oEventHandler) {
    pEventHandler = oEventHandler;
}

int CEventLogger::getLogClassNumberFrom(const char *strClass) {
    int nResult = 0;
    const char *strLevelMask = LOG_CLASS_LEVEL_MASK; 
    if(strClass) {
        char cClass = strClass[0];
        for(size_t nIdx = 0; nIdx < strlen(strLevelMask); nIdx++) {
            if(strLevelMask[nIdx] == cClass) {
                nResult = nIdx; break;
            }
        }
    }
    return(nResult);
}

/// @brief Calculates the character, representing the nClass number.
/// @param nClass 
/// @return The character or '\0' if not found...
char CEventLogger::getClassCharFromLogClass(int nClass) {
    char cResult = '\0';
    const char *strLevelMask = LOG_CLASS_LEVEL_MASK;
    if(nClass > -1 && nClass < (int) strlen(strLevelMask)) {
        char cData = strLevelMask[nClass];
        if(cData != '.') cResult = cData;
    }
    return(cResult);
}

void CEventLogger::logInfo(const __FlashStringHelper *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char*)strMessage, args);
    va_end(args);
    if(pEventHandler) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_INFO);
    }
}
void CEventLogger::logVerbose(const __FlashStringHelper *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char*)strMessage, args);
    va_end(args);
    if(pEventHandler) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_VERBOSE);
    }
}
void CEventLogger::logInfo(const char *strMessage, ...) {
    
        va_list args;
        va_start(args, strMessage);
        char strBuffer[LOG_ENTRY_BUFFER_SIZE];
        vsnprintf(strBuffer, sizeof(strBuffer), strMessage, args);
        va_end(args);
        if(pEventHandler) {
            pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_INFO);
        }
}
void CEventLogger::logVerbose(const char *strMessage, ...) {
    
        va_list args;
        va_start(args, strMessage);
        char strBuffer[LOG_ENTRY_BUFFER_SIZE];
        vsnprintf(strBuffer, sizeof(strBuffer), strMessage, args);
        va_end(args);
        if(pEventHandler) {
            pEventHandler->sendEvent(this,MSG_LOG_ENTRY, strBuffer, LOG_CLASS_VERBOSE);
        }
}

void CEventLogger::logError(const __FlashStringHelper *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char*)strMessage, args);
    va_end(args);
    if(pEventHandler) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_ERROR);
    }
}
void CEventLogger::logError(const char *strMessage, ...) {
        va_list args;
        va_start(args, strMessage);
        char strBuffer[LOG_ENTRY_BUFFER_SIZE];
        vsnprintf(strBuffer, sizeof(strBuffer), strMessage, args);
        va_end(args);
    if(pEventHandler) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_ERROR);
    }
}
void CEventLogger::logWarning(const __FlashStringHelper *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char*)strMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_WARN);
}
void CEventLogger::logWarning(const char *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf(strBuffer, sizeof(strBuffer), strMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_WARN);
}
void CEventLogger::logTrace(const char *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf(strBuffer, sizeof(strBuffer), strMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_TRACE);
}
void CEventLogger::logTrace(const __FlashStringHelper *strMessage, ...) {
    va_list args;
    va_start(args, strMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char*)strMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer, LOG_CLASS_TRACE);
}

void CEventLogger::log(const char * strType, const __FlashStringHelper* pszMessage,...) {
    va_list args;
    va_start(args, (const char*)pszMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char *) pszMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer,CEventLogger::getLogClassNumberFrom(strType) );
}

void CEventLogger::log(const char * pszType, const char *pszMessage, ...) {
    va_list args;
    va_start(args, pszMessage);
    char tBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(tBuffer, sizeof(tBuffer), pszMessage, args);
    va_end(args);
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, tBuffer, CEventLogger::getLogClassNumberFrom(pszType) );
}

void CEventLogger::log(const char * pszType, JsonDocument *pDoc) {
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY_JSON,pDoc, CEventLogger::getLogClassNumberFrom(pszType) );
}
#pragma endregion

#pragma region CLogWriter, Base Class for all Log Writers
void CLogWriter::writeLogEntry(const char *pszType, JsonDocument *pDoc) {
}

/// @brief Class to handle the logging for a module
///      This class is used to write log entries for a specific module
void CLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
}

/// @brief Receive an event, and prepare the LOG entries.
///        
/// @param pSender  Sender of the message 
/// @param nMsgType MSG_LOG_ENTRY or MSG_LOG_ENTRY_JSON are interesting
/// @param pMessage Either a string of a JsonObject/JsonDocument
/// @param nClass   Class Level -> representing LogLevel like 'I' or 'W' as number
int CLogWriter::receiveEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) {
    if(nMsgType == MSG_LOG_ENTRY || nMsgType == MSG_LOG_ENTRY_JSON) {
        String strType = "";
        char cLogClass = CEventLogger::getClassCharFromLogClass(nClass);
        if( cLogClass != '\0') {
            strType = "[";
            strType += cLogClass;
            strType += "] ";
        }
        
        if(nMsgType == MSG_LOG_ENTRY) {
            writeLogEntry(strType.c_str(),(const char *) pMessage);
        } 
        else if (nMsgType == MSG_LOG_ENTRY_JSON) {
            writeLogEntry(strType.c_str(),(JsonDocument *)(pMessage));
        }
    }
    return(EVENT_MSG_RESULT_OK);
}

#pragma endregion

#pragma region Serial Log Writer, Implementation of CLogWriter
void CSerialLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
    Serial.printf("%s%s\n", pszType, pszMessage);
}

void CSerialLogWriter::writeLogEntry(const char *strType, JsonDocument *pDoc) {
    DEBUG_FUNC_START();
    serializeJsonPretty(*pDoc,Serial);
}
#pragma endregion