#pragma once

#include <WiFiManager.h>
#include <WiFiClientSecure.h>

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

namespace HolisticSolutions{
namespace WiFi {    
    class WiFiSecureClientProvider : public WiFiManager {
    public:
        WiFiSecureClientProvider(WiFiClass &wifi = ::WiFi, HardwareSerial &log = Serial) : WiFiManager(wifi, log) {
        }

        virtual ~WiFiSecureClientProvider() {
        }

        /**
         * @copydoc WiFiManager::InsecureAccept()
         */
        virtual void InsecureAccept(void) { m_client.setInsecure(); };

    public: 
        virtual operator Client&() { return m_client; }

        virtual operator WiFiClientSecure&() { return m_client; }

    private:
        WiFiClientSecure m_client;
    };
}
}
