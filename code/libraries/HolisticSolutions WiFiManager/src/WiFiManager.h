#pragma once

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

#if defined(ESP32)
    #include <WiFi.h>
#elif defined (ESP8266)
    #include <ESP8266WiFi.h>
    #define WiFiClass ESP8266WiFiClass
#else
#   error "Unsupported platform"
#endif

#include <HardwareSerial.h>
 
#include <SimpleStateProcessor.h>
#include <SimpleSoftTimer.h>

#define NEG_COMMSUBSYSTEM_WIFIMANAGER_MAC_LEN   6

namespace HolisticSolutions{
namespace WiFi {

    class WiFiManager {
        public: 
            WiFiManager(WiFiClass &wifi = ::WiFi, HardwareSerial &log = Serial);
            virtual ~WiFiManager();

            void reset();

            int run();

            bool connect(const char *ssid, const char *psk);
            bool connected();
            void disconnect(void);
            bool disconnected();

            IPAddress      IpGet();
            const uint8_t *MacGet();
            const String & MacGetString();

        public:
            /**
             * @brief   Sets the client to accept insecure connections
             * 
             *          By invoking this method, the client is set to a mode, where it 
             *          does not validate the server certificate.
             * 
             * @warning Only use this mode for experimental setups, refrain from any
             *          productive use!
             */
            virtual void InsecureAccept(void) = 0;

            virtual operator Client&() = 0;
    
        private:
            uint8_t         m_mac[NEG_COMMSUBSYSTEM_WIFIMANAGER_MAC_LEN];
            String          m_mac_string;

            const char *    m_ssid;
            const char *    m_psk;

            WiFiClass &         m_wifi;
            HardwareSerial &    m_log;

        private:
            HolisticSolutions::SimpleSoftTimer m_timeout;

            SimpleStateProcessor *  m_fsm;
            static const tSSP_State m_WiFiStates[];

            /* Connection States */
            static SSP_STATE_HANDLER(Unknown);
            static SSP_STATE_HANDLER(Disconnected);
            static SSP_STATE_HANDLER(Connecting);
            static SSP_STATE_HANDLER(Connected);
            static SSP_STATE_HANDLER(Disconnecting);
    };
};
};
