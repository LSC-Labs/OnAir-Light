
#ifndef DEBUG_LSC_FILESYSTEM
    #undef DEBUGINFOS
#endif

#include <FileSystem.h>
#include <Appl.h>
#include <ArduinoJson.h>

bool isFSInitializedAndOpen = false;
 
void setupFS() {
    DEBUG_FUNC_START();
    isFSInitializedAndOpen = LittleFS.begin();
    if(!isFSInitializedAndOpen) {
        DEBUG_INFO("Formating Filesystem...");
        LittleFS.format();
        isFSInitializedAndOpen = LittleFS.begin();
    } 
    DEBUG_INFOS(" - Filesystem is %s", isFSInitializedAndOpen ? "initialized and open" : "NOT available");
}

bool fileExists(const char *pszFileName) {
    bool bResult = false;
    if(pszFileName) bResult = LittleFS.exists(pszFileName);
    return(bResult);
}

bool fileExists(String &strFileName) {
    bool bResult = false;
    if(strFileName) bResult = LittleFS.exists(strFileName);
    return(bResult);
}

void deleteFile(const char *pszFileName) {
    if(fileExists(pszFileName)) LittleFS.remove(pszFileName);
}

void deleteFile(String &strFileName) {
    if(fileExists(strFileName)) LittleFS.remove(strFileName);
}

void deleteAllFilesOnPath(const char *pszPath) {
    String strPath = pszPath ? pszPath : "/";
    Dir oDirEntry = LittleFS.openDir(strPath);
    while (oDirEntry.next()) {
        if (oDirEntry.isFile()) {
            DEBUG_INFOS(" - deleting file %s",oDirEntry.fileName().c_str());
            LittleFS.remove(oDirEntry.fileName());
        }
    }
}


/// @brief Get the file size, if the file exists
/// @param pszFileName the name of the file to be checked
/// @return -1 == file does not exist, otherwise the size of the file
size_t getFileSize(const char *pszFileName) {
    size_t nSize = -1;
    if(fileExists(pszFileName)) {
        File oFP = LittleFS.open(pszFileName,"r");
        if(oFP) {
            nSize = oFP.size();
            oFP.close();
        }
    }
    return(nSize);
}

size_t getFileSize(String &strFileName) {
    return(getFileSize(strFileName.c_str()));
}

bool getFileList(JsonDocument &oDirDoc, const char *pszPath) {
    return(oDirDoc.set(getFileList(pszPath)));
}

String getFileList(const char* pszPath) {
    String strResult = "[";
    const char *pszDir = pszPath ?  pszPath : "/";
    Dir oDirEntry = LittleFS.openDir(pszDir);
    while (oDirEntry.next()) {
        if (strResult.length() > 5) strResult += ",";
        strResult += "{ \"t\": ";
        if(oDirEntry.isFile()) strResult += "\"F\"";
        else if(oDirEntry.isDirectory()) strResult += "\"D\"";
        else strResult += "\"-\"";
        strResult += ", \"s\": ",
        strResult += oDirEntry.fileSize();
        strResult += ", \"n\": \"" + oDirEntry.fileName() + "\"";
        strResult += "}";
    }
    strResult += "]";
    return(strResult);
}

/// @brief Load a file into a buffer structure
///        Open the LittleFS with begin, before using this function (!)
///        Filename may NOT be stored in F("") - LittleFS will throw an exception (3).
/// @param strFileName The filename to be read.
/// @return nullptr - if it could not be loaded, otherwise a smart unique_ptr with the content of the requested file
size_t loadFileToBuffer(const char* strFileName, std::unique_ptr<char[]> &pData) {
    DEBUG_FUNC_START_PARMS("%s",strFileName);
    size_t nDataLen = 0;
    // std::unique_ptr<char[]> ptrBuffer = nullptr;
    File oFile = LittleFS.open(strFileName,"r");
    if(oFile) {
        pData = std::unique_ptr<char[]>(new char[oFile.size()]);
        // std::unique_ptr<char[]> ptrBuffer(new char [oFile.size()]);
        nDataLen = oFile.readBytes(pData.get(),oFile.size());
        oFile.close();
    }
    DEBUG_FUNC_END_PARMS("%d",nDataLen);
    return(nDataLen);
}

bool loadFileToString(const char* strFileName, String &strResult) {
    DEBUG_FUNC_START_PARMS("%s",strFileName);
    bool bResult = false;
    File oFile = LittleFS.open(strFileName,"r");
    if(oFile) {
        strResult = oFile.readString();
        oFile.close();
        bResult = true;
    }
    DEBUG_FUNC_END_PARMS("%d : %s",bResult, strResult.c_str());
    return(bResult);
}

bool saveJsonContentToFile(const char* strFileName, JsonDocument &oDoc) {
    bool bResult = false;
    File oFile = LittleFS.open(strFileName, "w");
    if (oFile)
    {
        serializeJson(oDoc,oFile);
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

bool saveJsonContentToFile(const char* pszFileName, JsonObject &oNode) {
    bool bResult = false;
    File oFile = LittleFS.open(pszFileName, "w");
    if (oFile)
    {
        serializeJson(oNode,oFile);
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

bool loadJsonContentFromFile(const char *strFileName,JsonDocument &oDoc) {
    DEBUG_FUNC_START_PARMS("%s,...",strFileName);
    int nSize = -1;
    std::unique_ptr<char[]> ptrBuffer;
    nSize = loadFileToBuffer(strFileName,ptrBuffer);
    // (void) nSize;
    DEBUG_INFOS(" --- loaded %d bytes %s",nSize,ptrBuffer ? ptrBuffer.get() : "-nullptr-");
    if(ptrBuffer) {
	    auto error = deserializeJson(oDoc, ptrBuffer.get());
        if (error)
        {
            ApplLogErrorWithParms(F("FS: failed to parse json file %s - %s"),strFileName,error.c_str());
        }
    }
    DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "NOT Loaded");
    return(nSize > 0? true: false);
}

/*
/// @brief load a file into a JsonObject
///        If the file is not found, or the content is not a valid Json, the function will return false.
/// @param strFileName Filename to be loaded
/// @param oNode Object to store the data found.
/// @return true - successfully loaded, false - failed to load
bool loadJsonContentFromFile(const char *strFileName,JsonObject &oNode) {
    DEBUG_FUNC_START_PARMS("%s,...",strFileName);
    bool bResult = false;
    if(fileExists(strFileName)) {
        DynamicJsonDocument oFileDoc(2048);
        String strData = "{}";
        if(loadFileToString(strFileName,strData)) {
            auto error = deserializeJson(oFileDoc, strData.c_str());
            if (error)
            {
                ApplLogErrorWithParms(F("FS: failed to parse json file %s - %s"),strFileName,error.c_str());
            }
            else {
                // doc says the other way round ... but this works...
                bResult  = oNode.set(oFileDoc.as<JsonObject>());
                if(!bResult) {
                    ApplLogError(F("FS: failed to set json object data - not enough memory..."));
                }              
            }
        }
    }
    DEBUG_FUNC_END();
    return(bResult);
}
    */
