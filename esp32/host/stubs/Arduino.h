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
 std::string rx;
 std::size_t rxPosition=0;
 int txCapacity=64;
 int available() const { return static_cast<int>(rx.size()-rxPosition); }
 int read() { return available() ? static_cast<unsigned char>(rx[rxPosition++]) : -1; }
 int availableForWrite() const { return txCapacity; }
 std::size_t write(uint8_t c) { if(txCapacity<=0) return 0; log+=static_cast<char>(c); return 1; }
 void feed(const std::string& text) { rx+=text; }

 void begin(int) {}
 void println(const char* text) { log+=text; log+='\n'; }
 template<class... T> void printf(const char* format,T... args) {
  char text[256]; std::snprintf(text,sizeof(text),format,args...); log+=text;
 }
};
inline SerialStub Serial;
