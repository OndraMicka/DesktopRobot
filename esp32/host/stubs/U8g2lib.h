#pragma once
#include <cstdint>
#include <cstring>
#include <array>
#define U8G2_R0 0
#define U8X8_PIN_NONE 255
inline const uint8_t u8g2_font_6x12_tf[]={0};
inline int sentFrames=0;
inline bool displayOK=true;
inline bool errorTextDrawn=false;
inline std::array<uint8_t,1024> firstDisplayFrame{}, lastDisplayFrame{};
class U8G2_SH1106_128X64_NONAME_F_HW_I2C {
 uint8_t buffer_[1024]{};
public:
 U8G2_SH1106_128X64_NONAME_F_HW_I2C(int,int,int,int) {}
 void setI2CAddress(int) {}
 void setBusClock(uint32_t) {}
 bool begin() { return displayOK; }
 void clearBuffer() { std::memset(buffer_,0,sizeof(buffer_)); }
 uint8_t* getBufferPtr() { return buffer_; }
 void setFont(const uint8_t*) {}
 void drawStr(int x,int y,const char* s) { errorTextDrawn=x==49 && y==62 && std::strcmp(s,"ERROR")==0; }
 void sendBuffer() {
  if (!sentFrames) std::memcpy(firstDisplayFrame.data(),buffer_,sizeof(buffer_));
  std::memcpy(lastDisplayFrame.data(),buffer_,sizeof(buffer_));
  ++sentFrames;
 }
};
