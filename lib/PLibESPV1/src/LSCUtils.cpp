#include <string>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>



String getAddressAsString(IPAddress ip) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}
String getAddressAsString(ip4_addr ip) {
    DEBUG_FUNC_START();
    char tBuffer[20];
    sprintf(tBuffer,IPSTR,IP2STR(&(ip)));
    return(String(tBuffer));
}

void ICACHE_FLASH_ATTR copyTo(char *pszTarget, const char *pszSource, int nMaxLen) {
    if(pszTarget && pszSource) {
        if(nMaxLen > 0) memset(pszTarget,0,nMaxLen);
        strncpy(pszTarget,pszSource,nMaxLen);
    }
}

/// @brief check if it is a "false" value string
/// @param strData either nullptr, "0", "false" or "-" will result in a false result
/// @return 
bool ICACHE_FLASH_ATTR isFalseValue(const char* strData) {
    bool bResult = false;
    if(!strData) bResult = true;
    else if(0 == strcmp(strData,"0")) bResult = true;
    else if( 0 == strcmp(strData,"false")) bResult = true;
    else if( 0 == strcmp(strData,"-")) bResult = true;
    else if( 0 == strcmp(strData,"off")) bResult = true;
    return(bResult);
}

/// @brief Check if the value is a true value.
///        checks if a string is NOT false, by using the isFalseValue() function.
///        To check explicit if the values represents a true state, use bExplicit=true
/// @param strData either "1", "true" or "+" will result in a true value (if bExplicit == true)
/// @param bExplicit false == will use !isFalseValue(), otherwise a check of specific values
/// @return 
bool ICACHE_FLASH_ATTR isTrueValue(const char * strData, bool bExplicit) {
    bool bResult = false;
    if(!bExplicit) bResult = !isFalseValue(strData);
    else {
        if(strData) {
            if(0 == strcmp(strData,"1")) bResult = true;
            else if( 0 == strcmp(strData,"true")) bResult = true;
            else if( 0 == strcmp(strData,"+")) bResult = true;
            else if( 0 == strcmp(strData,"on")) bResult = true;
        }
    }
    return(bResult);
}

void ICACHE_FLASH_ATTR parseBytesTo(byte *pBytes, const char *strData, char cSep, int nMaxBytes, int nBase)
{
	for (int i = 0; i < nMaxBytes; i++)
	{
        if(strData && strlen(strData) > 0 ) {
            pBytes[i] = strtoul(strData, NULL, nBase);  // Convert byte
            strData = strchr(strData, cSep);		    // Find next separator
            if (strData == NULL || *strData == '\0')
            {
                break; // No more separators, exit
            }
            strData++; // Point to next character after separator
        } else {
            pBytes[i] = 0;
        }
	}
}

/// @brief Store a int value to the target (by pointer)
/// @param pTarget pointer to store the value
/// @param strValue the value to be stored. if null or empty, try to set the default
/// @param pDefault pointer to a default, if strValue can not be used.
/// @return  true, if value or default could be set
bool ICACHE_FLASH_ATTR storeValueIF(float *pTarget, String strValue, const float *pDefault) {
    bool bResult = true;
    if(pTarget) {
        if(strValue && strValue.length() > 0) {     
            *pTarget = strValue.toFloat();
        } else if(pDefault && pTarget != pDefault) {
            *pTarget = *pDefault;
        } else {
            bResult = false;
        }
    } else bResult = false;
    return(bResult);
}


/// @brief Store a int value to the target (by pointer)
/// @param pTarget pointer to store the value
/// @param strValue the value to be stored. if null or empty, try to set the default
/// @param pDefault pointer to a default, if strValue can not be used.
/// @return  true, if value or default could be set
bool ICACHE_FLASH_ATTR storeValueIF(int *pTarget, String strValue, const int *pDefault) {
    bool bResult = true;
    if(pTarget) {
        if(strValue && strValue.length() > 0) { 
            DEBUG_INFOS("storeValueIF(int) -> %s (%lu)",strValue.c_str(),strValue.toInt());
            *pTarget = strValue.toInt();
        } else if(pDefault && pTarget != pDefault) {
            *pTarget = *pDefault;
        } else {
            bResult = false;
        }
    } else bResult = false;
    return(bResult);
}

/// @brief Store a unsigned long value to the target (by pointer)
/// @param pTarget pointer to store the value
/// @param strValue the value to be stored. if null or empty, try to set the default
/// @param pDefault pointer to a default, if strValue can not be used.
/// @return  true, if value or default could be set
bool ICACHE_FLASH_ATTR storeValueIF(unsigned long *pTarget, String strValue, const unsigned long *pDefault) {
    bool bResult = true;
    if(pTarget) {
        if(strValue && strValue.length() > 0) { 
            unsigned long ulValue = strValue.toInt();
            *pTarget = ulValue;
        } else if(pDefault && pTarget != pDefault) {
            *pTarget = *pDefault;
        } else {
            bResult = false;
        }
    } else bResult = false;
    return(bResult);
}


/// @brief Store a bool value to the target (by pointer)
/// @param pTarget pointer to store the value
/// @param strValue the value to be stored. if null or empty, try to set the default
/// @param bDefault The default, if strValue can not be used. 
/// @return  true, if value or default could be set
bool ICACHE_FLASH_ATTR storeValueIF(bool *pTarget, String strValue, const bool *pDefault) {
    bool bResult = true;
    if(pTarget) {
        if(strValue && strValue.length() > 0) { 
            *pTarget = isTrueValue(strValue.c_str());
        } else if(pDefault && pTarget != pDefault) {
            *pTarget = *pDefault;
        } else {
            bResult = false;
        }
    } else bResult = false;
    return(bResult);
}


/// @brief Store a psz value to the target 
/// @param strTarget Target string object
/// @param pszValue the value to be stored. if null or empty, try to set the default
/// @param pszDefault The default, if pszValue can not be used. Ignore if == STORE_DIGIT_VALUE_DEFAULT_NONE
/// @return  true, if value or default could be set
bool ICACHE_FLASH_ATTR storeValueIF(String &strTarget, const char* pszValue, const char* pszDefault) {
    bool bResult = true;
    if(strTarget) {
        if(pszValue && *pszValue && *pszValue != '\0') {
            strTarget = pszValue;
        } else if(pszDefault && *pszDefault != '\0') {
            strTarget = pszValue;
        } else {
            bResult = false;
        }
    } else bResult = false;
    return(bResult);
}
/*

bool ICACHE_FLASH_ATTR storeValue(char pTarget[], size_t nTargetSize, const char* strValue,const char* strDefault) {
    bool bResult = storeValueIF(pTarget,nTargetSize,strValue,strDefault);
    if(!bResult && pTarget) memset(pTarget,0,nTargetSize);
    return(bResult);
}
*/

bool ICACHE_FLASH_ATTR setIPAddressIF(IPAddress & pAddress, const char *strAddress, const char *strDefault) {
    DEBUG_FUNC_START_PARMS("..,%s,%s",NULL_POINTER_STRING(strAddress),NULL_POINTER_STRING(strDefault));
    bool bResult = true;
    if(strAddress) {
        DEBUG_INFOS("setIPAddressIF: setting strAddress=%s",strAddress);
        pAddress.fromString(strAddress);
    } else if(strDefault) {
        pAddress.fromString(strDefault);
    } else {
        bResult = false;
    }
    return(bResult);
}

bool ICACHE_FLASH_ATTR setIPAddress(IPAddress & pAddress, const char *strAddress, const char *strDefault) {
    bool bResult = setIPAddressIF(pAddress,strAddress,strDefault);
    if(!bResult) pAddress.clear();
    return(bResult);
}