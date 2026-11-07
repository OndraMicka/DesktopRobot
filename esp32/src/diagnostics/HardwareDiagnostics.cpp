#include "HardwareDiagnostics.h"
#include "../config/Pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdio>
namespace {
void report(HardwareDiagnostics::Log log,void* context,const char* message) {
 if (log) log(context,message); else Serial.println(message);
}
}
DiagnosticResult HardwareDiagnostics::run(OutputDisplay& display, InputController& input, Log log, void* context) {
 Serial.begin(115200);
 report(log,context,"[BOOT] DesktopRobot");
 input.begin();
 char message[96]; std::snprintf(message,sizeof(message),"[CHECK] TOUCH GPIO%u: CONFIGURED (not electrically verifiable)",Pins::TOUCH);
 report(log,context,message);
 if (!Wire.begin(Pins::SDA,Pins::SCL)) {
  report(log,context,"[ERROR] OLED I2C initialization failed");
  return {false,"I2C initialization failed"};
 }
 Wire.setClock(Pins::I2C_HZ); Wire.setTimeOut(50);
 Wire.beginTransmission(Pins::OLED_ADDRESS);
 if (Wire.endTransmission()!=0) {
  report(log,context,"[ERROR] OLED did not respond at configured I2C address");
  return {false,"Display not responding at configured address"};
 }
 if (!display.begin()) {
  report(log,context,"[ERROR] OLED initialization failed");
  return {false,"Display initialization failed"};
 }
 Wire.beginTransmission(Pins::OLED_ADDRESS);
 if (Wire.endTransmission()!=0) {
  report(log,context,"[ERROR] OLED stopped responding during initialization");
  return {false,"Display stopped responding during initialization"};
 }
 std::snprintf(message,sizeof(message),"[CHECK] OLED 0x%02X: OK",Pins::OLED_ADDRESS); report(log,context,message);
 if (input.beginMotionSensor()) report(log,context,"[CHECK] MPU6050 0x68: OK");
 else report(log,context,"[ERROR] MPU6050 unavailable; shake input disabled");
 return {true,nullptr};
}
