#ifndef DEBUG_LSC_LIGHTSENSOR
    #undef DEBUGINFOS
#endif
#include <Arduino.h>
#include <LightSensor.h>


/// @brief Class to handle the Light Sensonr

CLightSensor::CLightSensor(int nLsrPin) {
    setup(nLsrPin);
}

void CLightSensor::setup(int nLsrPin) {
    pinMode(m_nLsrPin, INPUT);
}

int CLightSensor::getLightValue() {
    m_nLsrValue = analogRead(m_nLsrPin);
    return m_nLsrValue;
}

void CLightSensor::runTests(CLightSwitch *pSwitch) {
      Serial.printf(" - testing pin : %d  - (Light Sensor)",m_nLsrPin);
      Serial.print(" -> ");
      for(int nLoops = 2; nLoops > 0; nLoops--) {
        if(pSwitch != NULL) pSwitch->switchOn();
        Serial.printf(" on=%d",getLightValue());
        delay(500);
        if(pSwitch != NULL) pSwitch->switchOff();
        Serial.printf(" off=%d",getLightValue());
        delay(500);
      }
      Serial.println(" ...done");
}
