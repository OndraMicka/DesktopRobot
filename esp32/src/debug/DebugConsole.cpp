#include "DebugConsole.h"
#if ROBOT_DEBUG_CONSOLE
#include "../Robot.h"
#include <Arduino.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cctype>

void DebugConsole::log(const char* format, ...) {
    char message[640];
    va_list args; va_start(args,format);
    const int n=std::vsnprintf(message,sizeof(message),format,args); va_end(args);
    if (n<0) return;
    const unsigned count=static_cast<unsigned>(n)<sizeof(message) ? static_cast<unsigned>(n) : sizeof(message)-1;
    const unsigned used=(txWrite_+sizeof(tx_)-txRead_)%sizeof(tx_);
    if (count>=sizeof(tx_)-used) { dropped_=true; return; }
    for(unsigned i=0;i<count;++i) { tx_[txWrite_]=message[i]; txWrite_=(txWrite_+1)%sizeof(tx_); }
}
void DebugConsole::flush() {
    int budget=Serial.availableForWrite();
    if (budget>64) budget=64;
    while(budget>0 && txRead_!=txWrite_) {
        if (Serial.write(static_cast<uint8_t>(tx_[txRead_]))!=1) break;
        txRead_=(txRead_+1)%sizeof(tx_); --budget;
    }
    if (dropped_ && txRead_==txWrite_) { dropped_=false; log("[CONSOLE] Output overflow: some messages dropped\n"); }
}
void DebugConsole::update() {
    flush();
    for(unsigned i=0;i<64 && Serial.available();++i) {
        const char c=static_cast<char>(Serial.read());
        if (c=='\r') continue;
        if (c=='\n') {
            line_[length_]='\0';
            if (overflow_) log("[CONSOLE] Command too long (max 95 characters)\n");
            else execute();
            length_=0; overflow_=false;
            break; // At most one command per loop, even when the host floods input.
        }
        if (c=='\b' || c==127) { if(length_ && !overflow_) --length_; continue; }
        if (static_cast<unsigned char>(c)<32) continue;
        if (length_+1<sizeof(line_) && !overflow_) line_[length_++]=c;
        else overflow_=true; // Discard the entire overlong line, never execute its suffix.
    }
}
void DebugConsole::execute() {
    char* command=line_;
    while(*command==' ' || *command=='\t') ++command;
    char* end=command+std::strlen(command);
    while(end>command && std::isspace(static_cast<unsigned char>(end[-1]))) *--end='\0';
    if (!*command) return;
    char* argument=command;
    while(*argument && !std::isspace(static_cast<unsigned char>(*argument))) {
        *argument=static_cast<char>(std::tolower(static_cast<unsigned char>(*argument))); ++argument;
    }
    if (*argument) { *argument++='\0'; while(std::isspace(static_cast<unsigned char>(*argument))) ++argument; }
    const bool hasArgument=*argument!='\0';
    if (std::strcmp(command,"help")==0 && !hasArgument) {
        log("Commands: help, status, state, diag, wake, tired, sleep, tap, hold, release, shake <1..600000 ms>, time HH:MM, time auto\n");
        log("TIME <local epoch seconds> sets real fallback clock (legacy). Manual state requests keep the next schedule boundary. ERROR permits read-only commands.\n");
        return;
    }
    if ((std::strcmp(command,"status")==0 || std::strcmp(command,"state")==0) && !hasArgument) {
        char status[512]; robot_.describeStatus(status,sizeof(status)); log("%s\n",status); return;
    }
    if (std::strcmp(command,"diag")==0 && !hasArgument) {
        log("[DIAG] Startup: %s; cause=%s; TOUCH CONFIGURED (not electrically verified). No reset or reinitialization.\n",
            robot_.diagnostics().ok ? "OK" : "FAILED", robot_.diagnostics().error ? robot_.diagnostics().error : "none"); return;
    }
    if (robot_.state()==RobotState::ERROR) { log("[CONSOLE] Rejected: ERROR is latched until restart\n"); return; }
    if (!hasArgument) {
        if (std::strcmp(command,"wake")==0) { robot_.requestState(RobotState::IDLE,true); return; }
        if (std::strcmp(command,"tired")==0) { robot_.requestState(RobotState::TIRED); return; }
        if (std::strcmp(command,"sleep")==0) { robot_.requestState(RobotState::SLEEP); return; }
        if (std::strcmp(command,"tap")==0) { robot_.handleEvent(InputEvent::TAP); return; }
        if (std::strcmp(command,"hold")==0) { robot_.handleEvent(InputEvent::HOLD); return; }
        if (std::strcmp(command,"release")==0) { robot_.handleEvent(InputEvent::RELEASE); return; }
    }
    if (std::strcmp(command,"shake")==0 && hasArgument) {
        uint32_t ms=0;
        for(const char* c=argument;*c;++c) {
            if (*c<'0' || *c>'9' || ms>60000) { log("[CONSOLE] Usage: shake <1..600000 ms>\n"); return; }
            ms=ms*10+static_cast<unsigned>(*c-'0');
        }
        if (!ms || ms>600000) { log("[CONSOLE] Usage: shake <1..600000 ms>\n"); return; }
        robot_.simulateShake(ms); return;
    }
    if (std::strcmp(command,"time")==0 && hasArgument) {
        if (std::strcmp(argument,"auto")==0) { robot_.useAutoTime(); return; }
        if (std::strlen(argument)==5 && argument[2]==':' && std::isdigit(static_cast<unsigned char>(argument[0]))
            && std::isdigit(static_cast<unsigned char>(argument[1])) && std::isdigit(static_cast<unsigned char>(argument[3]))
            && std::isdigit(static_cast<unsigned char>(argument[4]))) {
            const unsigned hour=(argument[0]-'0')*10+argument[1]-'0', minute=(argument[3]-'0')*10+argument[4]-'0';
            if (hour<24 && minute<60) { robot_.setSimulatedTime(hour*3600+minute*60); return; }
        } else {
            // Backward compatible absolute local-time setter; never changes the simulated clock.
            uint64_t seconds=0; bool valid=true;
            for(const char* c=argument;*c;++c) {
                if (*c<'0' || *c>'9' || seconds>(UINT64_MAX-static_cast<unsigned>(*c-'0'))/10) { valid=false; break; }
                seconds=seconds*10+static_cast<unsigned>(*c-'0');
            }
            if (valid) { robot_.setLocalTime(seconds); return; }
        }
        log("[CONSOLE] Usage: time HH:MM | time auto\n"); return;
    }
    log("[CONSOLE] Unknown command or arguments. Type help\n");
}
#endif
