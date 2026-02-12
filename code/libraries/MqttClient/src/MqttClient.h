#pragma once

#include <SimpleStateProcessor.h>
#include <SimpleSoftTimer.h>
#include <WiFiManager.h>

#include <string.h>
#include <map>

#include <ArduinoJson.h>
#include <PubSubClient.h>

/**
 * @file
 * @brief       MQTT client
 * 
 *              Wrapper for the Arduino Pub-Sub-Client simplifying connection 
 *              handling as well as simplified topic/subscription handling.
 * 
 * @author      Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright   Copyright (c) 2025 by Niederer Engineering GmbH
 */

namespace HolisticSolutions{
namespace Mqtt {
    #define MQTT_MESSAGE_HANDLER_NAME(name)     name ## Handler
    typedef void (*tMqttMessageHandler)(void *context, const char *topic, const void *data, size_t len);
    #define MQTT_MESSAGE_HANDLER_DECLARE(name)  void MQTT_MESSAGE_HANDLER_NAME(name)(void *context, const char *topic, const void *data, size_t len)

    typedef struct sMqttMessageHandlerRegistration {
        tMqttMessageHandler handler;
        void *              context;
    }tMqttMessageHandlerRegistration;

    class MqttClient {
        public:
            MqttClient(WiFi::WiFiManager &manager, HardwareSerial &log = Serial);
            virtual ~MqttClient();

            void reset();

            int  run();

            void CredentialsSet(const String &user, const String &password);

            void InsecureAccept();
            
            void          TopicPrefixSet(const String &prefix);
            const String& TopicPrefixGet();

            const String& ClientIdGet();

            bool connect(const String &clientId, const String &broker, int port);
            bool connected();
            bool disconnect();
            bool disconnected();

            bool publish(const String &topic, const void *data, size_t len, bool retain = false);
            bool publish(const String &topic, const JsonDocument& data, bool retain = false);
            bool publish(const String &topic, const String& data, bool retain = false);

            bool subscribe(const String &topic, tMqttMessageHandler handler, void *context);
            bool unsubscribe(const String &topic, tMqttMessageHandler handler, void *context);

        protected:
            virtual void OnTopicReceived(const char *topic, const void* data, size_t length);

        private:
            HardwareSerial&  m_log;

            WiFi::WiFiManager&  m_wifi;
            PubSubClient        m_client;
            
            String              m_topic_prefix;
            String              m_clientId;
            String              m_user;
            String              m_password;
            String              m_brokerIp;

        private:
            std::map<const String, tMqttMessageHandlerRegistration>    m_topics;

        private:
            SimpleStateProcessor *  m_fsm;
            static const tSSP_State m_mqtt_states[];

            SimpleSoftTimer         m_timeout;

            String                  TopicFullyQualifyIfNeeded(const String &topic);

            /* Connection States */
            static SSP_STATE_HANDLER(Unknown);
            static SSP_STATE_HANDLER(Disconnected);
            static SSP_STATE_HANDLER(Connecting);
            static SSP_STATE_HANDLER(Connected);
            static SSP_STATE_HANDLER(Disconnecting);                
            static SSP_STATE_HANDLER(ReconnectTimeout);                
    };
};
};
