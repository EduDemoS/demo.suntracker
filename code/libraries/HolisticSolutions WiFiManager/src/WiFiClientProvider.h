#pragma once

#include "WiFiManager.h"

/**
 * @file
 * @brief       WiFi Manager
  * 
 *              Further simplification of the Arduino WiFi stack implementing
 *              a simple statemachine to deal with the WiFi status.
 * 
 * @author      Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright   Copyright (c) 2025 by Niederer Engineering GmbH
 */

namespace NEG{
namespace CommSubsystem {    
    class WiFiClientProvider : public WiFiManager {
    public:
        WiFiClientProvider(WiFiClass &wifi = ::WiFi, HardwareSerial &log = Serial) : WiFiManager(wifi, log) {
        }

        virtual ~WiFiClientProvider() {
        }

    public:
        /**
         * @copydoc WiFiManager::InsecureAccept()
         */
        virtual void InsecureAccept(void) { ; };

        virtual operator Client&() { return m_client; }

    private:
        WiFiClient m_client;
    };
}
}
