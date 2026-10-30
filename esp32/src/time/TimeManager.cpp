#include "TimeManager.h"
#include <ctime>
#include <cstdio>
#include <cstdlib>
#ifdef ARDUINO
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <atomic>
#ifndef ROBOT_WIFI_TX_POWER_QDBM
#define ROBOT_WIFI_TX_POWER_QDBM 34 // Diagnostic profile: 8.5 dBm. Override via build_flags.
#endif
#if __has_include("WifiConfig.h")
#include "WifiConfig.h"
#else
#include "../../include/WifiConfig.example.h"
#endif
#endif

namespace {
#ifdef ARDUINO
class EspTimeNetwork final : public TimeNetwork {
    inline static std::atomic<bool> synchronized_{false};
    inline static std::atomic<unsigned> reason_{0};
    inline static std::atomic<bool> connecting_{false};
    char radioDetails_[144]{};
    bool eventRegistered_ = false;
    bool ntpStarted_ = false;
    static void disconnected(arduino_event_t* event) {
        if (connecting_.load(std::memory_order_acquire))
            reason_.store(event->event_info.wifi_sta_disconnected.reason,std::memory_order_release);
    }
    static void notified(struct timeval*) { synchronized_.store(true, std::memory_order_release); }
public:
    void configureTimezone(const char* tz) override { setenv("TZ",tz,1); tzset(); }
    bool connect(const char* ssid, const char* password) override {
        if (!eventRegistered_) {
            WiFi.onEvent(disconnected,ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
            eventRegistered_=true;
        }
        reason_.store(0,std::memory_order_release);
        connecting_.store(true,std::memory_order_release);
        WiFi.persistent(false);
        if (!WiFi.mode(WIFI_STA)) return false;
        WiFi.setAutoReconnect(false);
        const bool sleepSet=WiFi.setSleep(false);
        const auto result=WiFi.begin(ssid,password);
        const bool powerSet=WiFi.setTxPower(static_cast<wifi_power_t>(ROBOT_WIFI_TX_POWER_QDBM));
        const int actual=static_cast<int>(WiFi.getTxPower());
        std::snprintf(radioDetails_,sizeof(radioDetails_),
            "[TIME] Radio profile: power_save=%s TX=%d.%02d dBm tx_setting=%s",
            sleepSet ? "OFF" : "SET_FAILED",actual/4,(actual%4)*25,powerSet ? "APPLIED" : "FAILED");
        return result != WL_CONNECT_FAILED;
    }
    Connection connection() const override {
        switch(WiFi.status()) {
        case WL_CONNECTED: return Connection::Connected;
        case WL_NO_SSID_AVAIL: return Connection::NoAccessPoint;
        case WL_CONNECT_FAILED: return Connection::AuthenticationFailed;
        case WL_CONNECTION_LOST: return Connection::Disconnected;
        default: return Connection::Pending;
        }
    }
    const char* radioDetails() const override { return radioDetails_; }
    unsigned disconnectReason() const override { return reason_.load(std::memory_order_acquire); }
    void startNtp(const char* tz, const char* server1, const char* server2) override {
        synchronized_.store(false,std::memory_order_release);
        esp_sntp_set_time_sync_notification_cb(notified);
        ntpStarted_=true;
        configTzTime(tz,server1,server2);
    }
    bool takeSync(int64_t& epoch) override {
        if (!synchronized_.exchange(false,std::memory_order_acq_rel)) return false;
        epoch=static_cast<int64_t>(std::time(nullptr));
        return true;
    }
    int64_t utcNow() const override { return static_cast<int64_t>(std::time(nullptr)); }
    bool stop() override {
        connecting_.store(false,std::memory_order_release);
        if (ntpStarted_) {
            esp_sntp_stop();
            esp_sntp_set_time_sync_notification_cb(nullptr);
            ntpStarted_=false;
        }
        synchronized_.store(false,std::memory_order_release);
        if (WiFi.getMode()!=WIFI_OFF) {
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(false,false,0);
            WiFi.mode(WIFI_OFF);
        }
        return WiFi.getMode()==WIFI_OFF;
    }
};
TimeConfig platformConfig() {
    return {WifiConfig::SSID,WifiConfig::PASSWORD,WifiConfig::CONNECT_TIMEOUT_MS,
        WifiConfig::NTP_TIMEOUT_MS,WifiConfig::RETRY_INTERVAL_MS,WifiConfig::NTP_SERVER_1,WifiConfig::NTP_SERVER_2};
}
#else
class EspTimeNetwork final : public TimeNetwork {
public:
    void configureTimezone(const char*) override {}
    bool connect(const char*,const char*) override { return false; }
    Connection connection() const override { return Connection::Pending; }
    void startNtp(const char*,const char*,const char*) override {}
    bool takeSync(int64_t&) override { return false; }
    int64_t utcNow() const override { return 0; }
    bool stop() override { return true; }
};
TimeConfig platformConfig() { return {}; }
#endif
TimeNetwork& platformNetwork() { static EspTimeNetwork network; return network; }
const char* reasonName(unsigned reason) {
    switch(reason) {
    case 0: return "NO_DISCONNECT_EVENT";
    case 2: return "AUTH_EXPIRED";
    case 15: return "FOUR_WAY_HANDSHAKE_TIMEOUT";
    case 200: return "BEACON_TIMEOUT";
    case 201: return "NO_AP_FOUND";
    case 202: return "AUTH_FAIL";
    case 203: return "ASSOCIATION_FAILED";
    case 204: return "HANDSHAKE_TIMEOUT";
    case 210: return "NO_COMPATIBLE_SECURITY";
    case 211: return "AUTHMODE_THRESHOLD";
    case 212: return "SIGNAL_BELOW_THRESHOLD";
    default: return "OTHER_WIFI_REASON";
    }
}
bool utcFields(int64_t epoch, std::tm& fields) {
    const std::time_t value=static_cast<std::time_t>(epoch);
#ifdef ARDUINO
    return gmtime_r(&value,&fields)!=nullptr;
#else
    const auto* result=std::gmtime(&value);
    if (!result) return false;
    fields=*result; return true;
#endif
}
}
TimeManager::TimeManager(): TimeManager(platformNetwork(),platformConfig()) {}
TimeManager::TimeManager(TimeNetwork& network,TimeConfig config): network_(network),config_(config) {}
void TimeManager::report(const char* message) { if (log_) log_(context_,message); }
void TimeManager::begin(uint32_t now,Log log,void* context) {
    log_=log; context_=context; lastUpdate_=now;
    manual_=false; manualTime_={}; manualRemainder_=0;
    network_.configureTimezone(TIMEZONE);
    shutdown(now); attempt(now);
}
void TimeManager::shutdown(uint32_t now) {
    lastShutdown_=now;
    radioOff_=network_.stop();
    report(radioOff_ ? "[TIME] Wi-Fi radio OFF; SNTP stopped" : "[TIME] ERROR: failed to switch Wi-Fi radio OFF; will retry shutdown");
}
void TimeManager::attempt(uint32_t now) {
    if (!radioOff_) { fail("Wi-Fi radio shutdown incomplete",now); return; }
    error_=nullptr; stageStart_=now; reportedReason_=0;
    if (!config_.ssid || !*config_.ssid) { fail("Wi-Fi configuration missing: fill include/WifiConfig.h",now); return; }
    report("[TIME] Connecting to configured Wi-Fi (credentials hidden)");
    radioOff_=false; stage_=Stage::Connecting;
    const bool started=network_.connect(config_.ssid,config_.password);
    if (const char* details=network_.radioDetails()) report(details);
    if (!started) fail("Wi-Fi initialization/connection start failed",now);
}
void TimeManager::fail(const char* reason,uint32_t now) {
    const unsigned code=network_.disconnectReason();
    if (code) {
        std::snprintf(errorDetails_,sizeof(errorDetails_),"%s; reason=%u (%s)",reason,code,reasonName(code));
        error_=errorDetails_;
    } else error_=reason;
    stage_=Stage::Retry; stageStart_=now;
    char message[192]; std::snprintf(message,sizeof(message),"[TIME] FAILED: %s; retry in %lu ms",error_,static_cast<unsigned long>(config_.retryIntervalMs));
    report(message); shutdown(now);
}
void TimeManager::update(uint32_t now) {
    const uint32_t elapsed=now-lastUpdate_; lastUpdate_=now;
    if (manual_) {
        const uint64_t ms=uint64_t(manualRemainder_)+elapsed;
        manualRemainder_=ms%1000;
        const uint64_t seconds=manualTime_.second+ms/1000;
        manualTime_.day+=seconds/86400; manualTime_.second=seconds%86400;
    }
    if (!radioOff_ && stage_!=Stage::Connecting && stage_!=Stage::Ntp && now-lastShutdown_>=1000) shutdown(now);
    if (manual_ || stage_==Stage::NotStarted || stage_==Stage::Synchronized) return;
    if (stage_==Stage::Retry) {
        if (now-stageStart_>=config_.retryIntervalMs) attempt(now);
        return;
    }
    const unsigned code=network_.disconnectReason();
    if (code && code!=reportedReason_) {
        reportedReason_=code;
        char message[144];
        std::snprintf(message,sizeof(message),"[TIME] Wi-Fi disconnect reason=%u (%s)",code,reasonName(code));
        report(message);
    }
    const auto connection=network_.connection();
    if (stage_==Stage::Connecting) {
        if (connection==TimeNetwork::Connection::Connected) {
            report("[TIME] Wi-Fi connected; waiting for verified NTP response");
            stage_=Stage::Ntp; stageStart_=now;
            network_.startNtp(TIMEZONE,config_.server1,config_.server2);
        } else if (connection==TimeNetwork::Connection::NoAccessPoint) fail("Wi-Fi access point not found",now);
        else if (connection==TimeNetwork::Connection::AuthenticationFailed) fail("Wi-Fi connection rejected (authentication or association)",now);
        else if (now-stageStart_>=config_.connectTimeoutMs) fail("Wi-Fi connection timeout",now);
    } else if (stage_==Stage::Ntp) {
        int64_t epoch=0;
        if (network_.takeSync(epoch)) {
            if (!fromUtc(epoch).valid) { fail("NTP returned invalid date/time",now); return; }
            stage_=Stage::Synchronized; error_=nullptr;
            report("[TIME] NTP synchronization VERIFIED; system RTC continues offline");
            shutdown(now);
        } else if (connection!=TimeNetwork::Connection::Connected) fail("Wi-Fi disconnected while waiting for NTP",now);
        else if (now-stageStart_>=config_.ntpTimeoutMs) fail("NTP response timeout (Wi-Fi alone is not time synchronization)",now);
    }
}
void TimeManager::setManualLocalTime(uint64_t seconds,uint32_t now) {
    // Trust only this local clock until reboot, including late callbacks from an aborted attempt.
    manual_=true; stage_=Stage::Manual; error_=nullptr;
    manualTime_={static_cast<int64_t>(seconds/86400),static_cast<uint32_t>(seconds%86400),true};
    lastUpdate_=now; manualRemainder_=0; shutdown(now);
    report("[TIME] MANUAL local time; automatic synchronization cancelled until restart");
}
LocalTime TimeManager::localTime() const {
    if (manual_) return manualTime_;
    return stage_==Stage::Synchronized ? fromUtc(network_.utcNow()) : LocalTime{};
}
const char* TimeManager::status() const {
    switch(stage_) {
    case Stage::Connecting: return "WIFI_CONNECTING";
    case Stage::Ntp: return "NTP_WAITING";
    case Stage::Retry: return "RETRY_WAIT";
    case Stage::Synchronized: return "NTP_SYNCED";
    case Stage::Manual: return "MANUAL";
    default: return "NOT_STARTED";
    }
}
LocalTime TimeManager::fromUtc(int64_t epoch) {
    if (epoch<1577836800) return {};
    std::tm utc{};
    if (!utcFields(epoch,utc)) return {};
    // Exact CET-1CEST,M3.5.0/2,M10.5.0/3 rules, calculated from UTC so that
    // conversion remains deterministic on Windows hosts with limited POSIX TZ support.
    const int lastSunday=31-(utc.tm_wday+31-utc.tm_mday)%7;
    const bool summer=(utc.tm_mon>2 && utc.tm_mon<9)
        || (utc.tm_mon==2 && (utc.tm_mday>lastSunday || (utc.tm_mday==lastSunday && utc.tm_hour>=1)))
        || (utc.tm_mon==9 && (utc.tm_mday<lastSunday || (utc.tm_mday==lastSunday && utc.tm_hour<1)));
    const int64_t local=epoch+(summer ? 7200 : 3600);
    return {local/86400,static_cast<uint32_t>(local%86400),true};
}
