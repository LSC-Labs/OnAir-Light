#pragma once
#include <Arduino.h>
#include <PinController.h>

class CInputPinController : public CPinController {

    protected:
        bool m_bWithPullUpDown = false;

    public:
        CInputPinController() {};
        CInputPinController(int nPin, bool bLowLevelIsOff = false, bool bWithPullUpOrDown = false);

        virtual void setup(int SwitchPin, bool bLowLevelIsOff = true, bool bWithPullUpOrDown = false);
        virtual bool isPinLogicalON();
        virtual bool isPinLogicalOFF();
        virtual bool canSendInterrupts();
};

