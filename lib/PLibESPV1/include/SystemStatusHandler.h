#pragma once

#include <StatusHandler.h>

class CSystemStatusHandler : public IStatusHandler {
    public:
        void writeStatusTo(JsonObject &oStatusObj) override;
};

