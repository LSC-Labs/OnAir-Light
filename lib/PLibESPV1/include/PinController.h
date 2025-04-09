#pragma once

class CPinController {
    protected:
        int m_nPin      = -1;         // The pin to control the device
        int m_bLowLevelIsOff  = true;       // Defines if the device is inactive, when the pin level is low (true) or high (false)

    };
