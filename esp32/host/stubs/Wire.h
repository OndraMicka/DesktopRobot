#pragma once
#include <cstdint>
struct WireStub {
 bool busOK=true;
 int ack=0;
 bool begin(uint8_t,uint8_t) { return busOK; }
 void setClock(uint32_t) {}
 void setTimeOut(int) {}
 void beginTransmission(uint8_t) {}
 int endTransmission() { return ack; }
};
inline WireStub Wire;
