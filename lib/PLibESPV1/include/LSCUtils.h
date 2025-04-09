#pragma once
#include <Arduino.h>
#include <IPAddress.h>

#ifndef STORE_DIGIT_VALUE_DEFAULT_NONE
#define STORE_DIGIT_VALUE_DEFAULT_NONE -999
#endif

void ICACHE_FLASH_ATTR copyTo(char *pszTarget, const char *pszSource, int nMaxLen);
bool ICACHE_FLASH_ATTR isTrueValue(const char * pszData, bool bExplicit = false);
bool ICACHE_FLASH_ATTR isFalseValue(const char * pszData);

// String getAddressAsString(IPAddress ip);
// String getAddressAsString(ip4_addr ip);

void ICACHE_FLASH_ATTR parseBytesTo(byte *pBytes, const char *pszData, char cSep, int nMaxBytes, int nBase);

bool ICACHE_FLASH_ATTR storeValueIF(float         *pTarget, String strValue, const float         *pDefault   = nullptr);
bool ICACHE_FLASH_ATTR storeValueIF(int           *pTarget, String strValue, const int           *pDefault   = nullptr);
bool ICACHE_FLASH_ATTR storeValueIF(unsigned long *pTarget, String strValue, const unsigned long *pDefault   = nullptr);
bool ICACHE_FLASH_ATTR storeValueIF(bool          *pTarget, String strValue, const bool          *pDefault   = nullptr);

bool ICACHE_FLASH_ATTR storeValueIF(String &strTarget, const char* pszValue, const char          *pszDefault = nullptr);

/*
bool ICACHE_FLASH_ATTR storeValueIF(char  pTarget[], size_t nTargetSize, const char* pszValue,const char* pszDefault = nullptr);
bool ICACHE_FLASH_ATTR storeValue(  char  pTarget[], size_t nTargetSize, const char* pszValue,const char* pszDefault = nullptr);
*/

bool ICACHE_FLASH_ATTR setIPAddressIF(IPAddress pAddress, const char *pszAddress, const char *pszDefault = nullptr);
bool ICACHE_FLASH_ATTR setIPAddress(  IPAddress pAddress, const char *pszAddress, const char *pszDefault = nullptr);
