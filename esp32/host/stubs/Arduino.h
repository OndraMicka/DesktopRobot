#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#define HIGH 1
#define INPUT 0
inline uint32_t testNow=0;
inline int testPin=0, testReads=0;
inline uint32_t millis() { return testNow; }
inline int digitalRead(uint8_t) { ++testReads; return testPin; }
inline void pinMode(uint8_t,int) {}
struct SerialStub {
 std::string log;
 void begin(int) {}
 void println(const char* text) { log+=text; log+='\n'; }
 template<class... T> void printf(const char* format,T... args) {
  char text[256]; std::snprintf(text,sizeof(text),format,args...); log+=text;
 }
};
inline SerialStub Serial;
