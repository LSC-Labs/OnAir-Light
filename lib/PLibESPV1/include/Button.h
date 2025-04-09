#pragma once

#include <InputPinController.h>
#include <Ticker.h>

#define BUTTON_STATUS_ON  1
#define BUTTON_STATUS_OFF 0

/// Debouncing time of button... 
/// Default is 100 ms
#ifndef BUTTON_DEBOUNCING_TIME
    #define BUTTON_DEBOUNCING_TIME 0
#endif

class CButton;

/// @brief Interface for a button handler to be registered in CButton
class IButtonHandler
{
    public:
        virtual void buttonChanged(CButton *) = 0;
};  

/// @brief Native Function Pointer for a Button pressed action
typedef std::function<void(int)> funcButtonPressed;

class CButton : CInputPinController {
    private:
        unsigned long m_ulDebouncingTime = BUTTON_DEBOUNCING_TIME;  // Default 30ms...
        unsigned long m_ulLastCheckTime= 0L;
        // unsigned long m_ulLastOffStatus = 0L;
        volatile int m_nCurStatus = BUTTON_STATUS_OFF;
        
        std::vector<IButtonHandler*>     tButtonHandler;
        std::vector<funcButtonPressed>   tPressedHandler;
        
        /// @brief Interrupt Handling...
        /*
        bindArgVoidFunc_t m_pInterruptGate = nullptr;       // Gate for void() function...
        int               m_nInterruptNum;                  // Interrupt Number
        */
        /*
        bool              m_bPollingIsActivated = false;    // Polling is activated
        int               m_nPollMillis = 200;              // Frequency of polling in millis
        std::function<void()> m_fnCallInterruptHandler;     // Mapping of member for Polling Timer
        */

        void IRAM_ATTR interruptHandler();
       
        
    public:
        CButton();   
        CButton(   int nPin, bool bLowLevelIsOff = false, bool bUsePullUpDown = true);
        ~CButton();
        void setup(int nPin, bool bLowLevelIsOff = false, bool bUsePullUpDown = true);

        void startMonitoring();
        void stopMonitoring();
        // void setPollingFrequence(int nMS = 200) { m_nPollMillis = nMS; };

        bool isPressed();
        
        void registerButtonPressedHandler(funcButtonPressed &);
        void registerButtonHandler(IButtonHandler*);
        
};
