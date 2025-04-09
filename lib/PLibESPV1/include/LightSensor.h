#pragma once

#include <LightSwitch.h>

class CLightSensor {
  private:
    int m_nLsrPin;
    int m_nLsrValue = 0;

  public:
    CLightSensor() {};
    CLightSensor(int nLsrPin);

    void setup(int nLsrPin);

    int getLightValue();

    void runTests(CLightSwitch *pSwitch);

};

