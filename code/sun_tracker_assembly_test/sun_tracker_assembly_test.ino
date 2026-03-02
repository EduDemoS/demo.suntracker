/*
 * This file is part of the EduDemoS Sun-Tracker which is
 * co-funded by the European Union.
 * Copyright (C) 2025  Gerda Stetter Stiftung - Technik macht Spaß!
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <Arduino.h>

/*  To setup your project:
    1. Create a copy of configuration.default.cpp
    2. Name the copy "configuration.cpp"
    3. Adjust the settings in configuration.cpp according to your needs 
       (look for @todo-comments) */
#include "configuration.cpp"

// Include Libraries
#include <Ticker.h>

#include <MqttClient.h>
#include <WiFiSecureClientProvider.h>

#include <SimpleSoftTimer.h>
#include <SimpleStateProcessor.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <AccelStepper.h>

#include <EEPROM.h>

using namespace HolisticSolutions;
using namespace HolisticSolutions::WiFi;
using namespace HolisticSolutions::Mqtt;

#ifndef LED_BUILTIN_AUX
#define LED_BUILTIN_AUX 16
#endif

#if ENABLE_WIFI
// WiFi and MQTT Functions:
static void TaskCommunication(void);
static void TaskHeartbeat(void);

static bool WiFiConnect();
static bool MqttConnect();

static void SendJsonDoc(const char *topic, const JsonDocument &doc, bool retain = false);

template <typename T>
void SendDatapoint(const char *topic, const T& value, bool retain = false, const char *key = "value") {
  JsonDocument data;
  data[key] = value;
  SendJsonDoc(topic, data, retain);
}
#endif

// All pin numbers printed on the board! Check the Pinout Diagram for the correct numbers
// if you want to make changes: https://www.az-delivery.de/en/products/nodemcu

typedef enum eTestState {
  TEST_ST_CONFIRM_SWITCH = 0,
#if ENABLE_WIFI
  TEST_ST_MQTT,
#endif
  TEST_ST_DISPLAY,
  TEST_ST_SERVO,
  TEST_ST_STEPPER,
  TEST_ST_LDR,
  TEST_ST_SOLAR,
  TEST_ST_PASSED,

  TEST_ST_COUNT
}tTestState;

typedef enum eStepperState {
  STEPPER_ST_CCW_ZERO = 0,
  STEPPER_ST_CW_ZERO,
  STEPPER_ST_TURN_CW,
  STEPPER_ST_TURN_CCW,
  STEPPER_ST_ZERO,

  STEPPER_ST_COUNT
}tStepperState;

typedef enum eCheckState {
  CHECK_STATE_UNKNOWN = 0,
  CHECK_STATE_START,
  CHECK_STATE_PASSED,

  CHECK_STATE_COUNT
}tCheckState;

typedef struct sLimitSwitchData {
  long lastPressTime;
  bool isPressed;
}tLimitSwitchData;

/*! \brief  Holds all stepper dependent data. 

            While members such as phase pins or pattern are considered configuration
            and thus immutable during runtime, the rest of the struct is divided into
            parts shared with the ISR and parts that are not. */
typedef struct sStepperData {
  Ticker *       timer;

  const uint8_t *phase_pins;
  uint8_t        phase_pins_len;

  const uint8_t *pattern;
  uint8_t        pattern_len;

  /*! \brief  Variables shared between two tasks (ISR and main loop)

      \warning Access to this area shall always be wrapped within a critical section
               or only take place from an ISR! */
  struct sStepperDataUnsafe {
    int8_t         pattern_current;
    int32_t        position_target;
    int32_t        position;
    int8_t         direction;
  }unsafe;
}tStepperData;

static void TaskUpdateLcd(void);
static void TaskUpdateServo(void);

static SSP_STATE_HANDLER(TestConfirmSwitch);
#if ENABLE_WIFI
static SSP_STATE_HANDLER(TestMqtt);
#endif
static SSP_STATE_HANDLER(TestDisplay);
static SSP_STATE_HANDLER(TestServo);
static SSP_STATE_HANDLER(TestStepper);
static SSP_STATE_HANDLER(TestLdr);
static SSP_STATE_HANDLER(TestSolar);
static SSP_STATE_HANDLER(TestPassed);


static SSP_STATE_HANDLER(StepperCCWZero);
static SSP_STATE_HANDLER(StepperCWZero);
static SSP_STATE_HANDLER(StepperTurnCW);
static SSP_STATE_HANDLER(StepperTurnCCW);
static SSP_STATE_HANDLER(StepperZero);

static void CheckStateSet(tCheckState state);
static int  CheckStateToJson(tCheckState state);

static bool DeviceScanFor(byte address);

static void    LimitSwitchSetup(void);
static void    LimitSwitchReset(void);
void IRAM_ATTR LimitSwitchISR();

static void ServoSetup(void);
static void SolarSetup(void);

static void           StepperSetup(void);
static void           StepperMove(bool ccw = false);
static void           StepperMoveBy(int32_t position);
static void           StepperMoveTo(int32_t position);
static int32_t        StepperPositionGet(void);
static void           StepperPositionZero(void);
static bool           StepperRunning(void);
static void           StepperStop(void);

static void           StepperPatternWrite(uint8_t pattern);
static bool           StepperSideWrite(void);
static void           StepperStopUnsafe(void);
       void IRAM_ATTR StepperTick(void);

int  readPCFChannel(int channelID);
bool scanForDevice(byte address);

static const tSSP_State TestStates[] = {
    SSP_STATE_DESCRIBE("Confirm switch  ", TestConfirmSwitch),
#if ENABLE_WIFI
    SSP_STATE_DESCRIBE("Wait for uplink ", TestMqtt),
#endif
    SSP_STATE_DESCRIBE("Test display    ", TestDisplay),
    SSP_STATE_DESCRIBE("Test servo move ", TestServo),
    SSP_STATE_DESCRIBE("Test stepper mot", TestStepper),
    SSP_STATE_DESCRIBE("Test LDRs       ", TestLdr),
    SSP_STATE_DESCRIBE("Test solar panel", TestSolar),
    SSP_STATE_DESCRIBE("All tests passed", TestPassed),

    SSP_STATE_LAST()
};

static const tSSP_State StepperStates[] = {
    SSP_STATE_DESCRIBE("CCW Zero        ", StepperCCWZero),
    SSP_STATE_DESCRIBE("CW Zero         ", StepperCWZero),
    SSP_STATE_DESCRIBE("Stepper Turn CW ", StepperTurnCW),
    SSP_STATE_DESCRIBE("Stepper Turn CCW", StepperTurnCCW),
    SSP_STATE_DESCRIBE("Stepper Zeroing ", StepperZero),

    SSP_STATE_LAST()
};

// Create WiFi and MQTT Objects:
#if ENABLE_WIFI
static const char                MQTT_ANIMATION[] = "-\\|/";
static size_t                    mqtt_animation_pos = 0;

static WiFiSecureClientProvider  upstream;
static MqttClient                mqtt(upstream);

static SimpleSoftTimer          _heartbeat_timeout(MQTT_UPDATE_PERIOD / 2);

static bool                     _test_state_send = false;
static bool                     _mqtt_connecting = false;
static bool                     _online = false;
static bool                     _toggle = false;
#endif

static SimpleStateProcessor test_fsm(TEST_ST_DISPLAY, TestStates, 0);
static SimpleSoftTimer      test_timeout(1000);
static SimpleSoftTimer      test_debounce(2000);
static tCheckState          test_states[TEST_ST_COUNT + 1] = { CHECK_STATE_UNKNOWN };
static bool                 test_confirmed_key_state = false;
static bool                 test_confirmed = false;
static size_t               test_state_current = 0;

static SimpleStateProcessor stepper_fsm(STEPPER_ST_CCW_ZERO, StepperStates, 0);

static volatile tLimitSwitchData LimitSwitchData;

static SimpleSoftTimer    lcd_update_timeout(LCD_UPDATE_PERIOD);
static LiquidCrystal_I2C  lcd(LCD_ADDR, 16, 2);
static bool               lcd_available = false;
static bool               lcd_dirty = true;
static char               lcd_lines[LCD_LINE_COUNT][LCD_LINE_LEN + 1] = {
                            "                ",
                            "                ",
                          };

static Servo              servo;
static int                servo_ctrl_pos = SERVO_BOUNDARY_MIN;
static int                servo_ctrl_speed = 1;                      /* [deg/SERVO_UPDATE_PERIOD] */
static int                servo_ctrl_dst = SERVO_BOUNDARY_MIN;
static SimpleSoftTimer    servo_ctrl_timeout(SERVO_UPDATE_PERIOD);

static int                servo_test_pos_inc = SERVO_INCREMENT;

static volatile tStepperData stepper;
static          bool         stepper_zero;

void setup() {
  // Double the CPU Frequency:
  os_update_cpu_frequency(160);

  Serial.begin(SERIAL_BAUD_RATE);
  while(!Serial) delay(1);

  Serial.println("Starting SunFlower Test");

  Wire.begin();

  pinMode(LED_BUILTIN_AUX,  OUTPUT);
  digitalWrite(LED_BUILTIN_AUX, HIGH);

  TestsSetup();
  ServoSetup();

  lcd_update_timeout.restart();
  servo_ctrl_timeout.restart();
  test_timeout.restart();

#if ENABLE_WIFI
  // Connect to WiFi and MQTT:
  Serial.print("Resetting WiFi...");
  upstream.reset();
  Serial.println("done");
  
  Serial.print("Resetting MQTT client...");
  mqtt.reset();
  Serial.println("done");

  _mqtt_connecting = false;
#else
  Serial.println("IoT uplink disabled");
#endif
}

void loop() {
  if (Serial.available()) {
    while (Serial.available()) {
      Serial.read();
    }
    test_confirmed = true;
  }

  // @todo Ugly variant for debouncing by simply holding off, do something better.
  if (test_debounce.isTimeout() 
      && !digitalRead(TEST_CONFIRM_PIN)) {
    test_confirmed = true;
    test_debounce.restart();
  }

  test_fsm.run();

  // Reset test confirmation event
  test_confirmed = false;

  TaskUpdateServo();
  TaskUpdateLcd();

#if ENABLE_WIFI
  TaskCommunication();
  TaskHeartbeat();
#endif
}

static SSP_STATE_HANDLER(TestConfirmSwitch) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      print_lcd(1, 0, "Press conf. sw. ");
      test_timeout.start(1000);
      break;
    }

    case SSP_REASON_DO: {
      if (test_timeout.isTimeout()) {
        Serial.println("Waiting for test confirmation...");
        test_timeout.restart();
      }

      if (test_confirmed) {
        Serial.println("Confirmation detected, moving on...");
        print_lcd(1, 0, "Got it!        ");

#if ENABLE_WIFI
        fsm->NextStateSet(TEST_ST_MQTT);
#else
        fsm->NextStateSet(TEST_ST_STEPPER);
#endif
      }
      break;
    }

    case SSP_REASON_EXIT: {
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default:
    {
      break;
    }
  }

  return 0;
}

#if ENABLE_WIFI
static SSP_STATE_HANDLER(TestMqtt) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);

      Serial.println("Waiting for uplink");
      print_lcd(1, 0, "WiFi...         ");

      WiFiConnect();

      test_timeout.start(500);
      break;
    }

    case SSP_REASON_DO: {
      if (test_timeout.isTimeout()) {
        test_timeout.restart();

        mqtt_animation_pos++;
        if (MQTT_ANIMATION[mqtt_animation_pos] == 0) {
          mqtt_animation_pos = 0;
        }

        char animation = MQTT_ANIMATION[mqtt_animation_pos];

        Serial.print("Waiting for ");
        if (!upstream.connected()) {
          Serial.print("WiFi ");
          print_lcd(1, 0, "WiFi connection ");
        }
        else if (!mqtt.connected()) {
          Serial.print("MQTT ");
          print_lcd(1, 0, "MQTT Broker    ");

        }

        Serial.println(animation);
        print_lcd(1, 15, String(animation));
      }

      if (upstream.connected() && 
          !_mqtt_connecting) {
          MqttConnect();
          _mqtt_connecting = true;
      }

      if (mqtt.connected()) {
        fsm->NextStateSet(TEST_ST_STEPPER);
      }
      break;
    }

    case SSP_REASON_EXIT: {
      _online = true;
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}
#endif

static SSP_STATE_HANDLER(TestDisplay) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      test_timeout.start(1000);
      break;
    }

    case SSP_REASON_DO: {
      if (DeviceScanFor(LCD_ADDR)) {
        Serial.println("Display found!");
        
        lcd.begin();
        lcd_available = true;
        print_lcd(1, 0, "Display found");

        fsm->NextStateSet(TEST_ST_CONFIRM_SWITCH);
      }
      else if (test_timeout.isTimeout()) {
        Serial.println("Scanning for display...");
        test_timeout.restart();
      }
      break;
    }

    case SSP_REASON_EXIT: {
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(TestServo) {  
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      Serial.println("Setting up servo...");
      print_lcd(1, 0, "                ");
      ServoSetup();
      test_timeout.start(1000);
      break;
    }

    case SSP_REASON_DO: {
      if (test_timeout.isTimeout()) {
        test_timeout.restart();

        if (servo_ctrl_dst == servo_ctrl_pos) {
          // Calculate new servo position and saturate at boundaries of 0 and 180
          // If a boundary is hit, we switch directions.
          servo_ctrl_dst += servo_test_pos_inc;
          if (servo_ctrl_dst <= SERVO_BOUNDARY_MIN) {
            servo_ctrl_dst = SERVO_BOUNDARY_MIN;
            servo_test_pos_inc = -servo_test_pos_inc;
          }
          else if (servo_ctrl_dst >= SERVO_BOUNDARY_MAX) {
            servo_ctrl_dst = SERVO_BOUNDARY_MAX;
            servo_test_pos_inc = -servo_test_pos_inc;
          }

          String pos_str = String(servo_ctrl_dst);
          while (pos_str.length() < 3) {
            pos_str = " " + pos_str;
          }
          pos_str = "Servo pos: " + pos_str;
          
          Serial.println(pos_str);
          print_lcd(1, 0, pos_str);
        }
      }

      if (test_confirmed) {
        fsm->NextStateSet(TEST_ST_SOLAR);
      }
      break;
    }

    case SSP_REASON_EXIT: {
      CheckStateSet(CHECK_STATE_PASSED);
      Serial.println("Setting to 90°");
      servo_ctrl_dst = 90;
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(TestStepper) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      print_lcd(1, 0, "                ");
      stepper_zero = false;
      StepperSetup();
      break;
    }

    case SSP_REASON_DO: {
      stepper_fsm.run();

      if(stepper_zero) {
        fsm->NextStateSet(TEST_ST_SERVO);
      }
      break;
    }

    case SSP_REASON_EXIT: {
      StepperSideWrite();
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}

int  readPCFChannel(int channelID)
{
  Wire.beginTransmission(PCF8591_ADDR);
  Wire.write(0x40 | channelID);
  byte errorPCF = Wire.endTransmission();

  // Check for PCF errors:
  if(errorPCF)
  {
    // Evaluate Error code:
    Serial.print("I2C Error: ");
    Serial.print(errorPCF);
    Serial.print(" - ");
    switch(errorPCF)
    {
      case 1:  Serial.println("Data too long for buffer."); break;
      case 2:  Serial.println("NACK on address transmission (Device not found)"); break;
      case 3:  Serial.println("NACK on data transmission."); break;
      case 4:  Serial.println("Unknown I2C failure."); break;
      default: Serial.println("Unknown I2C failure."); break;
    }

    // Check for PCF Module:
    if(!DeviceScanFor(PCF8591_ADDR))
    {
      Serial.print("       --> PCF8591     not detected! Check wiring. Check I2C address in datasheet(0x");
      Serial.print(PCF8591_ADDR, HEX);
      Serial.println("). If this happens sporadically, power off, wait 15s, power on.");
    }
    else
    {
      Serial.println("       --> PCF8591     is connected.");
    }
    
    delay(100);
    return 0;
  }

  //Return voltage value:
  Wire.requestFrom(PCF8591_ADDR, 2);
  if(Wire.available() >= 2)
  {
    Wire.read();
    return Wire.read();
  }
  else
  {
    // PCF is not responding (despite no error before, this should not happen):
    Serial.println("I2C Error: PCF8591 is not responding with data!");
    delay(100);
  }
  return 0;
}

static SSP_STATE_HANDLER(TestLdr) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      print_lcd(1, 0, "                ");
      break;
    }

    case SSP_REASON_DO: {
      String Readings[4];

      for(uint8_t i = 0; i < 4; i++)
      {
        int Reading = readPCFChannel(i);
        String ReadingStr = String(Reading);

        if(Reading < 10) { 
          ReadingStr = String("0") + ReadingStr;
        }

        Readings[i] = ReadingStr;
      }

      print_lcd(0, 0, String(" ") + Readings[0] + " TEST LDR " + Readings[1] + String(" "));
      print_lcd(1, 0, String(" ") + Readings[2] + "          " + Readings[3] + String(" "));

      if (test_confirmed) {
        fsm->NextStateSet(TEST_ST_PASSED);
      }
      break;
    }

    case SSP_REASON_EXIT: {
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(TestSolar) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      CheckStateSet(CHECK_STATE_START);
      print_lcd(1, 0, "                ");
      ServoSetup();
      break;
    }

    case SSP_REASON_DO: {
      if (test_confirmed) {
        fsm->NextStateSet(TEST_ST_LDR);
      }
      uint16_t solarReading = analogRead(SOLAR_ADC_PIN);
      float solarVoltage = float((solarReading) * 3.3 / 1024.0) * float((RESISTOR_SOL1 + RESISTOR_SOL2) / RESISTOR_SOL1);
      print_lcd(1, 0, String(solarVoltage) + "V                      ");
      break;
    }

    case SSP_REASON_EXIT: {
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(TestPassed) {
    switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "yeah!           ");
      CheckStateSet(CHECK_STATE_PASSED);
      break;
    }

    case SSP_REASON_DO: {
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}


static SSP_STATE_HANDLER(StepperCCWZero) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "Zeroing CCW     ");
      StepperPositionZero();

      // Go for a half turn to find the zero switch.
      StepperMoveTo(-1 * STEPPER_STEPS_PER_REVOLUTION_MAX / 2);
      break;
    }

    case SSP_REASON_DO: {
      if (LimitSwitchData.isPressed) {
        StepperStop();
        StepperPositionZero();
        LimitSwitchData.isPressed = false;

        fsm->NextStateSet(STEPPER_ST_TURN_CW); 
        break;
      }

      /* If we didn't find the zero switch after a bit more than half a 
         revolution, we gotta look to the other side in order to prevent 
         entangeling of wires in case they are already mounted. */
      if(!StepperRunning()) {
        fsm->NextStateSet(STEPPER_ST_CW_ZERO);
      }
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(StepperCWZero) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "Zeroing CW     ");

      // As we didn't find the zero switch during half CCW turn, 
      // let's move a complete revolution CW so we hopefully find
      // it.
      StepperMoveBy(STEPPER_STEPS_PER_REVOLUTION_MAX);
      break;
    }

    case SSP_REASON_DO: {
      if (LimitSwitchData.isPressed) {
        StepperPositionZero();
        LimitSwitchData.isPressed = false;

        fsm->NextStateSet(STEPPER_ST_TURN_CCW);
      }

      /* If we actually reach the desired position, we have missed the zero 
         switch, wich might be a sign, that the switch is not yet present. */
      if (!StepperRunning()) {
        print_lcd(1, 0, "Err: Limit miss");
      }
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(StepperTurnCW) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "Turning CW      ");
      StepperMoveTo(STEPPER_STEPS_PER_REVOLUTION / 2);
      break;
    }

    case SSP_REASON_DO: {
      if (!StepperRunning()) {
        Serial.printf("Turning at: %d\n", StepperPositionGet());
        fsm->NextStateSet(STEPPER_ST_TURN_CCW);
        break;
      }
      
      if(test_confirmed) {
        fsm->NextStateSet(STEPPER_ST_ZERO);
      }
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(StepperTurnCCW) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "Turning CCW     ");
      StepperMoveTo(-1 * (STEPPER_STEPS_PER_REVOLUTION / 2));
      break;
    }

    case SSP_REASON_DO: {      
      if (!StepperRunning()) {
        Serial.printf("Turning at: %d\n", StepperPositionGet());
        fsm->NextStateSet(STEPPER_ST_TURN_CW);
        break;
      }

      if(test_confirmed) {
        fsm->NextStateSet(STEPPER_ST_ZERO);
      }
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}

static SSP_STATE_HANDLER(StepperZero) {
  switch (reason) {
    case SSP_REASON_ENTER: {
      print_lcd(1, 0, "Zeroing...      ");
      StepperMoveTo(0);
      break;
    }

    case SSP_REASON_DO: {
      if(!StepperRunning()) {
        stepper_zero = true;
        break;
      }
      break;
    }

    case SSP_REASON_EXIT:
    default: {
      break;
    }
  }
  return 0;
}

static void CheckStateSet(tCheckState state) {
  test_states[test_state_current] = state;
  if (state == CHECK_STATE_PASSED) {
    if (test_state_current <= TEST_ST_COUNT) {
      test_state_current++;
    }
  }
}

static int  CheckStateToJson(tCheckState state) {
  int result = 0;

  switch (state) {
    case CHECK_STATE_PASSED: {
      result = 2;
      break;
    }
    case CHECK_STATE_START: {
      result = 1;
      break;
    }
    default: {
      result = 0;
      break;
    }
  }

  return result;
}

static void TaskUpdateServo(void) {
  if (servo_ctrl_timeout.isTimeout()) {
    servo_ctrl_timeout.restart();

    int pos_delta = servo_ctrl_dst - servo_ctrl_pos;

    if (pos_delta != 0) {
      int step = min(abs(servo_ctrl_speed), abs(pos_delta));

      if (pos_delta < 0) {
        step = -1 * step;
      }

      servo_ctrl_pos += step;
      servo.write(servo_ctrl_pos);
    }
  }
}

static void TaskUpdateLcd(void) {
  if (!lcd_available || !lcd_dirty) return;

  if (lcd_update_timeout.isTimeout()) {
    lcd_update_timeout.restart();

    /* Copy the current state to the first line... */
    if(strcmp(test_fsm.CurrentStateNameGet(), "Test LDRs       ") != 0) {
      strncpy(lcd_lines[0], test_fsm.CurrentStateNameGet(), LCD_LINE_LEN);
    }

    for (int i = 0;i < 2;i++) {
      lcd_lines[i][LCD_LINE_LEN] = '\0';

      lcd.setCursor(0, i);
      lcd.print(lcd_lines[i]);
    }

    lcd_dirty = false;
  }
}

// WiFi and MQTT Functions:
#if ENABLE_WIFI
static void TaskCommunication() {
  upstream.run();
  if (upstream.connected()) {
    mqtt.run();
  }
}

static void TaskHeartbeat() {
  if (!_online) return;

  if (_heartbeat_timeout.isTimeout()) {
    _heartbeat_timeout.restart();

    if (!mqtt.connected()) {
      Serial.println("...and my heart skips, skips a beat (no connection)");
      return;
    }

    if (_test_state_send) {
      JsonDocument teststate_doc;
      JsonArray array = teststate_doc.to<JsonArray>();
      for (size_t i = 0;i < TEST_ST_COUNT;i++) {
        int state = CheckStateToJson(test_states[i]);
        array.add(state);
      }
      SendJsonDoc("data/testresult", teststate_doc);
    }
    else {
      JsonDocument heartbeat;
      heartbeat["state"] = _toggle ? "on" : "off";
      heartbeat["time"] = millis();
      SendJsonDoc("data/heartbeat", heartbeat);
      _toggle = !_toggle;
    }

    _test_state_send = !_test_state_send;
  }
}

// Connect to WiFi network.
static bool WiFiConnect()
{
  Serial.print("Connecting to WiFi network '");
  Serial.print(WIFI_SSID);
  Serial.print("' .");

  // Attempt WiFI connection:
  upstream.reset();
  upstream.connect(WIFI_SSID, WIFI_PASSWORD);

  return true;
}

// Establish connection with the MQTT Broker.
static bool MqttConnect()
{
  Serial.print("Connecting to MQTT server  '" MQTT_SERVER "' .");

  /* Only use this mode for experimental setups, 
    refrain from any productive use!*/
  mqtt.InsecureAccept();
  mqtt.TopicPrefixSet("EduDemoS/" MQTT_CLIENT_ID);

  mqtt.CredentialsSet(MQTT_USERNAME, MQTT_PASSWORD);
  mqtt.connect(MQTT_CLIENT_ID, MQTT_SERVER, MQTT_PORT);

  return true;
}

static void SendJsonDoc(const char *topic, const JsonDocument &doc, bool retain) {
  String payload;
  serializeJson(doc, payload);
  mqtt.publish(topic, payload, retain);
}
#endif

static void LimitSwitchSetup(void) {
  Serial.println("Setting up limit switch");
  LimitSwitchReset();
  pinMode(LIMITSWITCH_PIN, INPUT_PULLUP);
  attachInterrupt(LIMITSWITCH_PIN, LimitSwitchISR, FALLING);
}

static void LimitSwitchReset(void) {
  LimitSwitchData.isPressed = false;
  LimitSwitchData.lastPressTime = 0;
}

void IRAM_ATTR LimitSwitchISR()
{
  // Debounce switch
  if ((millis() - LimitSwitchData.lastPressTime) > LIMITSWITCH_DEBOUNCE_TIME) {
    LimitSwitchData.isPressed = true;
    LimitSwitchData.lastPressTime = millis();
  }
}

static void ServoSetup(void) {
  servo.attach(SERVO_PIN_PWM);
}

static void SolarSetup(void) {
  pinMode(SOLAR_ADC_PIN,    INPUT);
}

static void StepperSetup(void) {
  static const uint8_t StepperPhasePins[] = {
    STEPPER_PIN_0, 
    STEPPER_PIN_1, 
    STEPPER_PIN_2, 
    STEPPER_PIN_3, 
  };

  static const uint8_t HalfStepPattern[] = {
    0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09, 0x01, 0x00
  };

  LimitSwitchSetup();

  memset((void *)&stepper, 0, sizeof(stepper));

  stepper.timer = new Ticker();

  stepper.phase_pins = StepperPhasePins;
  stepper.phase_pins_len = sizeof(StepperPhasePins)/sizeof(StepperPhasePins[0]);
  stepper.pattern = HalfStepPattern;
  stepper.pattern_len = 0;

  /* Count the number of steps */
  while (HalfStepPattern[stepper.pattern_len] != 0) {
    stepper.pattern_len++;
  }

  /* Add a guard to prevent erroneus behaviour of the stepper's ISR. */
  while (stepper.pattern_len == 0) {
    Serial.println("Error: There is no stepper pattern configured!");
  }

  for (size_t i = 0;i < stepper.phase_pins_len;i++) {
    pinMode(stepper.phase_pins[i], OUTPUT);
  }

  stepper.unsafe.pattern_current = 0;
  stepper.unsafe.direction = 0;
  stepper.unsafe.position = 0;

  stepper.timer->attach(0.001, StepperTick);
}

/*! \brief  Sets the stepper in motion without limitation

    \param[in]  ccw 
                - false for clock wise motion 
                - true for counter clock wise motion */
static void StepperMove(bool ccw) {
  StepperMoveTo(ccw ? INT32_MIN : INT32_MAX);
}

/*! \brief  Moves the stepper to an absolute position

    \param[in]  position [steps] Position to move to */
static void StepperMoveTo(int32_t position) {
  noInterrupts();
  {
    stepper.unsafe.position_target = position;
    if (stepper.unsafe.position < stepper.unsafe.position_target) {
      stepper.unsafe.direction = 1;
    }
    else if (stepper.unsafe.position > stepper.unsafe.position_target) {
      stepper.unsafe.direction = -1;
    }
  }
  interrupts();
}

/*! \brief  Moves the stepper relative to the current position

    \param[in]  increment [steps] Number of steps to run
                                  - Positive numbers: clockwise turns
                                  - Negative numbers: counter clock wise turns */
static void StepperMoveBy(int32_t increment) {
  StepperMoveTo(StepperPositionGet() + increment);
}

/*! \brief  Stops the stepper's motion. */
static void StepperStop(void) {
  noInterrupts();
  {
    StepperStopUnsafe();
  }
  interrupts();
}

/*! \brief  Stops the stepper's motion.

    \warning  Only call this within a critical section or an ISR. */
static void StepperStopUnsafe(void) {
  stepper.unsafe.direction = 0;
}

/*! \return [steps] current position. 
            - Positive numbers: clockwise turns
            - Negative numbers: counter clock wise turns */
static int32_t StepperPositionGet(void) {
  int32_t localPosition = 0;

  noInterrupts();
  {
    localPosition = stepper.unsafe.position;
  }
  interrupts();

  return localPosition;
}

/*! \brief  Set the current position to zero */
static void StepperPositionZero(void) {
  noInterrupts();
  {
    stepper.unsafe.position = 0;
  }
  interrupts();
}

/*! \brief  Indicates whether the stepper is running or not

            Can be used to detect whether the position has been reached

    \return 
            - true if the stepper is still running
            - false if the stepper is idle */
static bool StepperRunning(void) {
  int8_t localDirection = 0;

  noInterrupts();
  {
    localDirection = stepper.unsafe.direction;
  }
  interrupts();

  return localDirection != 0;
}

/*! \brief  ISR taking care of stepper pattern output and position control. 

            For a reliable stepper operation, the frequency for phase shifts
            has to be rather constant. This interrupt service routine serves
            as a pace maker as well as control "algorithm" for the position. */
void StepperTick(void) {
  if (stepper.unsafe.direction > 0) {
    stepper.unsafe.pattern_current++;
    /* Wrap at the upper end */
    if (stepper.unsafe.pattern_current >= stepper.pattern_len) {
      stepper.unsafe.pattern_current = 0;
    } 

    /* Saturate at the limits in order to prevent warp around effects. */
    if (stepper.unsafe.position < INT32_MAX) {
      stepper.unsafe.position++;
    }
  }
  else if (stepper.unsafe.direction < 0) {
    stepper.unsafe.pattern_current--;
    /* Wrap at the lower end */
    if (stepper.unsafe.pattern_current < 0) {
      stepper.unsafe.pattern_current = stepper.pattern_len - 1;
    }

    /* Saturate at the limits in order to prevent warp around effects. */
    if (stepper.unsafe.position > INT32_MIN) {
      stepper.unsafe.position--;
    }
  }

  if (stepper.unsafe.direction != 0) {
    StepperPatternWrite(stepper.pattern[stepper.unsafe.pattern_current]);
    
    if ((stepper.unsafe.position_target != INT32_MIN)
        && (stepper.unsafe.position_target != INT32_MAX) 
        && (stepper.unsafe.position == stepper.unsafe.position_target)) {
      StepperStopUnsafe();
    }
  }
}

/*! \brief Converts a given pattern into actual output for all phases
    \param[in]  pattern   Bitmask which phases have to be turned on. 
                          Bitnumber => Output number */
static void StepperPatternWrite(uint8_t pattern) {
  for (size_t i = 0;i < stepper.phase_pins_len;i++) {
    digitalWrite(stepper.phase_pins[i], pattern & (1 << i) ? HIGH : LOW);
  }
}

// 
/*! \brief  Writes the signature and the current side of the stepper to permanent storage. 
    \return 
            - true if EEPROM was successfully written
            - false if an error occured */
static bool StepperSideWrite(void) {
  Serial.print("Writing Stepper Side to storage... ");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_ADDR_SIGNATURE, 255);
  EEPROM.write(EEPROM_ADDR_SIDE,      255);
  bool success = EEPROM.commit();
  EEPROM.end();

  if(!success)
  {
    Serial.println();
    Serial.println("EEPROM Error: Could not commit.");
    return false;
  }
  Serial.println("Done.");
  return true;
}

static void TestsSetup(void) {
  pinMode(TEST_CONFIRM_PIN, INPUT_PULLUP);
  test_confirmed = false;
  for (size_t i = 0;i < TEST_ST_COUNT;i++) {
    test_states[i] = CHECK_STATE_UNKNOWN;
  }
  test_fsm.reset();
}

static bool DeviceScanFor(byte address)
{
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0); // Returns true if device is found.
}

static void print_lcd(int line, int col, const String msg) {
  size_t maxlen = LCD_LINE_LEN - col;
  char *start = &(lcd_lines[line][col]);

  if (strncmp(start, msg.c_str(), maxlen) != 0) {
    lcd_dirty = true;
    strncpy(start, msg.c_str(), maxlen);
  }
}
