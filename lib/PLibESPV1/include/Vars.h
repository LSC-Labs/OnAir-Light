#pragma once
#include <Arduino.h>
#include <map>
#include <ConfigHandler.h>
/**
 * Represents a single Var entry
 */
class CVar {
        String Name;
        String Value;
        bool ReadOnly = false;
    public:
        CVar(String strName, String strValue)                       { Name = strName, setValue(strValue);}
        CVar(String strName, int nValue)                            { Name = strName, setValue(nValue);}
        CVar(String strName, bool bValue)                           { Name = strName, setValue(bValue);}

        CVar(const __FlashStringHelper* strName, String strValue)   { Name = strName, setValue(strValue);}
        CVar(const __FlashStringHelper* strName, int nValue)        { Name = strName, setValue(nValue);}
        CVar(const __FlashStringHelper* strName, bool bValue)       { Name = strName, setValue(bValue);}

        CVar(const char *strName, const char *strValue)             { Name = strName; setValue(strValue);}
        CVar(const char *strName, const int   nValue)               { Name = strName; setValue(nValue); }
        CVar(const char *strName, const bool  bValue)               { Name = strName; setValue(bValue); }

        const char * getName()      { return(Name.c_str()); }
        const char * getValue()     { return(Value.c_str()); }
        const int    getIntValue()  { return(Value.toInt()); }
        const bool   getBoolValue() { return(!(Value == "0" || Value == "-" || Value == "false")); }

        void setValue(const char * strValue)                { Value = strValue; }
        void setValue(const int  nValue)                    { Value = nValue;   }
        void setValue(const bool bValue)                    { Value = bValue;   }
        void setValue(String strValue)                      { Value = strValue; }
        void setValue(const __FlashStringHelper* strValue)  { Value = strValue; }
};

/**
 * Represents a collection of vars in a table
 */
class CVarTable : public IConfigHandler {

    std::map<String,CVar> tVars;
    bool isCaseSensitive = false;

    protected:
        String& getKeyName(const char * pszVarName);
        CVar * find(const char *);
        CVar * find(String); 
        CVar * find(const __FlashStringHelper*);
        CVar * getOrCreateVarEntry(const char *strName);

    public:
        CVarTable(bool bCaseSensitive = false);

        void setVar(String strName, String strValue);
        void setVar(String strName, int    nValue);
        void setVar(String strName, bool   bValue);
        void setVar(const char * pszName, String strValue);
        void setVar(const char * pszName, int    nValue);
        void setVar(const char * pszName, bool   bValue);
        void setVar(const __FlashStringHelper*, String  );
        void setVar(const __FlashStringHelper*, int     );
        void setVar(const __FlashStringHelper*, bool    );

        const char * getVarValue(    String strName, const char * pszDefault);
        int          getVarIntValue( String strName, int nDefault);
        bool         getVarBoolValue(String strName, bool bDefault);

        const char * getValue(    const char *pszName, const char * pszDefault);
        int          getIntValue( const char *pszName, int nDefault);
        bool         getBoolValue(const char *pszName, bool bDefault);

        // Implement the IConfigHandler interface
        virtual void writeConfigTo(JsonObject &oCfgObj, bool bHideCritical) override;
        virtual void readConfigFrom(JsonObject &oCfgObj) override;
};



