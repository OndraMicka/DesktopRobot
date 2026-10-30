#include "HardwareDiagnostics.h"
#include "../config/Pins.h"
#include <Arduino.h>
#include <Wire.h>
DiagnosticResult HardwareDiagnostics::run(OutputDisplay& display, InputController& input) {
 Serial.begin(115200);
 Serial.println("\n[BOOT] DesktopRobot");
 input.begin();
 Serial.printf("[CHECK] TOUCH GPIO%u: CONFIGURED (not electrically verifiable)\n",Pins::TOUCH);
 if (!Wire.begin(Pins::SDA,Pins::SCL)) {
  Serial.println("[CHECK] OLED: FAILED (I2C initialization)");
  return {false,"I2C initialization failed"};
 }
 Wire.setClock(Pins::I2C_HZ); Wire.setTimeOut(50);
 Wire.beginTransmission(Pins::OLED_ADDRESS);
 if (Wire.endTransmission()!=0) {
  Serial.println("[CHECK] OLED: FAILED (no ACK)");
  return {false,"Display not responding at configured address"};
 }
 if (!display.begin()) {
  Serial.println("[CHECK] OLED: FAILED (initialization)");
  return {false,"Display initialization failed"};
 }
 Wire.beginTransmission(Pins::OLED_ADDRESS);
 if (Wire.endTransmission()!=0) {
  Serial.println("[CHECK] OLED: FAILED (ACK after initialization)");
  return {false,"Display stopped responding during initialization"};
 }
 Serial.printf("[CHECK] OLED 0x%02X: OK\n",Pins::OLED_ADDRESS);
 return {true,nullptr};
}
