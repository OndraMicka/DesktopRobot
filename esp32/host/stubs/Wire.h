#pragma once
#include <cstddef>
#include <cstdint>
struct WireStub {
 bool busOK=true;
 int ack=0;
 uint8_t reg=0;
 bool begin(uint8_t,uint8_t) { return busOK; }
 void setClock(uint32_t) {}
 void setTimeOut(int) {}
 void beginTransmission(uint8_t) {}
 size_t write(uint8_t value) { reg=value; return 1; }
 int endTransmission(bool=true) { return ack; }
 int requestFrom(uint8_t,uint8_t) { return 0; }
 int read() { return -1; }
};
inline WireStub Wire;
