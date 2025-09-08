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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*

*  +-----------------------------------------------------------------------------------------------------+  *
*                                                                                                           *
*  Upload this code to your ESP8266 board, especially if you have used it before. Connect your Servo Motor  *
*  to Pin D5 and power. This will rotate it to the 90deg position. This code will also reset the permanent  *
*  storage used by the Sun-Tracker to remember the last side of the zero-position it was on. This prevents  *
*  tangling up the wires inside the Stem.                                                                   */


// Includes:
#include <EEPROM.h>
#include <Servo.h>

// Define the Servo Pin (has to be capable of PWM)
#define PIN_SERVO 14  // Pin D5

// Storage Signature address:
#define EEPROM_SIZE           2
#define EEPROM_ADDR_SIGNATURE 0
#define EEPROM_ADDR_SIDE      1

// Create Servo Object
Servo servo;

bool writeStepperSide();

 void setup()
 {
  // Setup USB communication:
  Serial.begin(9600);
  Serial.println("Turning Servo to 90 deg.");

  // Setup Servomotor and rotate it to 90deg:
  servo.attach(PIN_SERVO);
  servo.write(90);

  // Reset storage:
  writeStepperSide();
 }

 void loop()
 {
  // Tell the user that everything is done.
  Serial.println("This ESP and the Servo Motor are prepared.");
  delay(1000);
 }


 // Writes the signature and the current side of the stepper to permanent storage.
bool writeStepperSide()
{
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