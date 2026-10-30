#pragma once
#include "../config/Debug.h"
#if ROBOT_DEBUG_CONSOLE
#include <cstddef>
#include <cstdint>
class Robot;
class DebugConsole {
public:
    explicit DebugConsole(Robot& robot): robot_(robot) {}
    void update();
    void log(const char* format, ...);
private:
    Robot& robot_;
    char line_[96]{};
    unsigned length_ = 0;
    bool overflow_ = false;
    char tx_[2048]{};
    unsigned txRead_ = 0, txWrite_ = 0;
    bool dropped_ = false;
    void execute();
    void flush();
};
#endif
