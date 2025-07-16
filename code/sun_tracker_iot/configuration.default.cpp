/*                                                                                                           *
 *  Thank you for building EduDemoS!                                                                         *
 *                                                                                                           *
 *  This file is part of the EduDemoS Project and is licensed under the Creative Commons Attribution         *
 *  NonCommercial ShareAlike 4.0 International license (CC BY-NC-SA 4.0). For full license details, see the  *
 *  license.txt file or visit https://creativecommons.org/licenses/by-nc-sa/4.0/                             *     
 *                                                                                                           *
 *  EduDemoS is co-funded by the European Union. You can find more information at edudemos.eu                *                                                         
 *                                                                                                           *
 *  +-----------------------------------------------------------------------------------------------------+  *
 *                                                                                                           *
 *  This file lets you change certain parameters of the Sun-Tracker and adapt its behaviour.                 */


// Toggle features:
#define ENABLE_WIFI           1 // [0, 1]   If WiFi/MQTT should be used (MQTT data below has to be correct).
#define ENABLE_SERIAL_DATA    1 // [0, 1]   If LDR/solar values should be sent via serial.
#define ENABLE_STARTUP_CHECKS 1 // [0, 1]   If hardware/connection checks should be made on startup.
#define ENABLE_TIMING_LOG     0 // [0, 1]   Enable timing logs for debug use, set to 0 unless you are after a bug

// Sunflower behaviour:
#define LCD_UPDATE_PERIOD   50  // [ms]     Time to wait between updating the LCD display.
#define SENS_UPDATE_PERIOD  50  // [ms]     Time to wait between reading/evaluating sensors.
#define MIN_BRIGHTNESS      1   // [0, 255] Minimum necessary brightness to start moving.
#define DIFF_BRIGHTNESS     1   // [0, 255] Minimum necessary brightness difference
                                //          between neighbouring LDRs to start moving.
#define BACKTRACK_LIMIT     100 // [deg]    How far the Flower can turn before reversing
                                //          to prevent tangling wires. Must be > 95deg

// WiFi Settings:
/* @todo Update the WiFi settings according to your secrets-sheet */
#define WIFI_SSID           "SECRET_WIFI_SSID"      //  Name of your WiFi network
#define WIFI_PASSWORD       "SECRET_WIFI_PASSWORD"  //  Password of your WiFi network
#define WIFI_MAX_ATTEMPTS   20                      //  How many times to attempt to connect
                                                    //  before resuming without WiFi

// MQTT Settings:
/* @todo Update the MQTT settings according to your secrets-sheet */
#define MQTT_WORKSHOP_ID        "undefined"                 // Workshop ID as provided 
#define MQTT_TEAM_ID            "xx"                        // Team ID (two digits, leading zero - e.g. "01" or "10")
#define MQTT_USERNAME           "MQTT_USERNAME"             // MQTT Username
#define MQTT_PASSWORD           "MQTT_PASSWORD"             // MQTT Password

#define MQTT_SERVER             "iot-mqtt-broker.gbssg.ch"  // MQTT Broker Address
#define MQTT_PORT               8883                        // MQTT Broker Port
#define MQTT_UPDATE_PERIOD      2000                        // [ms] Time to wait before sending data to MQTT
#define MQTT_MAX_ATTEMPTS       10

// Serial Settings:
#define SERIAL_BAUD_RATE    9600    // [bits/s] // Communication speed for USB Connection

// I2C Settings:
#define PCF8591_ADDR        0x48    // I2C Address of the PCF8591 Module
#define LCD_ADDR            0x27    // I2C Address of the LCD Display

// Pins:
#define PIN_ENDSTOP         12      // GPIO Pins for all connections. The numbers are different from the ones
#define PIN_SOLAR_ADC       A0      // printed on the board! Check the Pinout Diagram for the correct numbers
#define PIN_SERVO_PWM       14      // if you want to make changes:
#define PIN_STEPPER_0       2       // https://www.az-delivery.de/en/products/nodemcu
#define PIN_STEPPER_1       0
#define PIN_STEPPER_2       15
#define PIN_STEPPER_3       13

// Resistors:
#define RESISTOR_SOL1       2.2     // Resistor 1 of the Solar Panel voltage divider
#define RESISTOR_SOL2       10.0    // REsistor 2 of the Solar Panel voltage divider

// Fine control Servo Motor angle:
#define SERVO_PWM_180DEG 2000   // Increase to ~2300 if Servo doesn't fully turn
#define SERVO_PWM_0DEG   1000   // Decrease to ~ 700 if Servo doesn't fully turn