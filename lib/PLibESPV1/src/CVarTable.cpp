
#ifndef DEBUG_LSC_VARS
    #undef DEBUGINFOS
#endif
#include <Vars.h>
#include <DevelopmentHelper.h>

#define LSC_VARS_CRITICAL_ENTRY_MASK "******"

CVarTable::CVarTable(bool isCaseSensitive) {
    this->isCaseSensitive = isCaseSensitive;
}

String& CVarTable::getKeyName(const char *pszVarName) {
    String *pKeyName = new String(pszVarName);
    if(isCaseSensitive) pKeyName->toUpperCase();
    return(*pKeyName);
}

#pragma region Find Var by name...


CVar * CVarTable::find(const __FlashStringHelper* strName) {
    return(find((const char*) strName));
}

CVar * CVarTable::find(String strName) {
    return(find(strName.c_str()));
}

CVar * CVarTable::find(const char * strName) {
    CVar *pResult = nullptr;
    if(strName) {
        String strSearchName = getKeyName(strName);
        auto oElement = tVars.find(strSearchName.c_str());
        if(oElement != tVars.end()) pResult = &oElement->second;
    }
    return(pResult);
}

#pragma endregion


#pragma region Get Var Values

const char * CVarTable::getVarValue(String strName, const char *pszDefault) {
    CVar *pVar = find(strName);
    return(pVar ? pVar->getValue() : pszDefault );
}

const char * CVarTable::getValue(const char *pszName, const char *pszDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getValue() : pszDefault );
}

int CVarTable::getVarIntValue(String strName, int nDefault){
    CVar *pVar = find(strName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

int CVarTable::getIntValue(const char *pszName, int nDefault){
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

bool CVarTable::getVarBoolValue(String strName, bool bDefault) {
    CVar *pVar = find(strName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}

bool CVarTable::getBoolValue(const char * pszName, bool bDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}

#pragma endregion


#pragma region setVar overlays


CVar * CVarTable::getOrCreateVarEntry(const char *pszName) {
    CVar *pVar = find(pszName);
    if(pVar == nullptr) {
        String strKeyName = getKeyName(pszName);
        pVar = new CVar(strKeyName,"");
        tVars.insert(std::pair<String,CVar>( strKeyName,*pVar));
    }
    return(pVar);
}

void CVarTable::setVar(const char * pszName, String strValue) {
    CVar * pVar =  getOrCreateVarEntry(pszName);
    pVar->setValue(strValue);
}

void CVarTable::setVar(const char * pszName, int nValue) {
    CVar * pVar =  getOrCreateVarEntry(pszName);
    pVar->setValue(nValue);
}

void CVarTable::setVar(const char * pszName, bool bValue) {
    CVar * pVar =  getOrCreateVarEntry(pszName);
    pVar->setValue(bValue);
}

void CVarTable::setVar(String strName, String strValue) {
    setVar(strName.c_str(),strValue);
}

void CVarTable::setVar(String strName, int nValue) {
    setVar(strName.c_str(),nValue);
}

void CVarTable::setVar(String strName, bool bValue) {
    setVar(strName.c_str(),bValue);
}

void CVarTable::setVar(const __FlashStringHelper* strName, String strValue) {
    setVar((const char*) strName,strValue);
}

void CVarTable::setVar(const __FlashStringHelper* strName, int nValue) {
    setVar((const char*) strName,nValue);
}

void CVarTable::setVar(const __FlashStringHelper* strName, bool bValue) {
    setVar((const char* ) strName,bValue);
}

#pragma endregion

void CVarTable::writeConfigTo(JsonObject &oCfgObj, bool bHideCritical) {
    for(auto pVarEntry : this->tVars) {
        bool isCritical = false;
        if(bHideCritical) {
            String strLower = pVarEntry.first.c_str();
            strLower.toLowerCase();
            if(strLower.indexOf("pwd")     > -1 ||
               strLower.indexOf("passwd")  > -1 ||
               strLower.indexOf("password") > -1) isCritical = false;
        }
        oCfgObj[pVarEntry.second.getName()] = isCritical ? 
                                              LSC_VARS_CRITICAL_ENTRY_MASK : 
                                              pVarEntry.second.getValue();
    }
}

void CVarTable::readConfigFrom(JsonObject &oCfgObj) {
    DEBUG_FUNC_START();
    for(const auto oElement : oCfgObj) {
        String strValue = oElement.value();
        // Set only if it is NOT a CRITICAL VALUE that is HIDDEN, to avoid loosing the original data.
        // You have to use setVar() explicit for this value...
        if(!strValue.equals(LSC_VARS_CRITICAL_ENTRY_MASK)) {
            setVar(oElement.key().c_str(),strValue.c_str());
        }
    }   
    DEBUG_FUNC_END();
}