#pragma once
#include "../behavior/BehaviourProgram.h"
#include <cstdint>

struct TimeConfig {
    const char* ssid = "";
    const char* password = "";
    uint32_t connectTimeoutMs = 15000;
    uint32_t ntpTimeoutMs = 15000;
    uint32_t retryIntervalMs = 30000;
    const char* server1 = "pool.ntp.org";
    const char* server2 = "time.cloudflare.com";
};

// Narrow platform boundary: network/system-clock operations can be tested without Wi-Fi.
class TimeNetwork {
public:
    enum class Connection { Pending, Connected, NoAccessPoint, AuthenticationFailed, Disconnected };
    virtual ~TimeNetwork() = default;
    virtual void configureTimezone(const char* timezone) = 0;
    virtual bool connect(const char* ssid, const char* password) = 0;
    virtual Connection connection() const = 0;
    virtual unsigned disconnectReason() const { return 0; }
    virtual const char* radioDetails() const { return nullptr; }
    virtual void startNtp(const char* timezone, const char* server1, const char* server2) = 0;
    virtual bool takeSync(int64_t& utc) = 0;
    virtual int64_t utcNow() const = 0;
    virtual bool stop() = 0; // Stop SNTP, disconnect, switch radio OFF and verify mode.
};

class TimeManager {
public:
    inline static constexpr const char* TIMEZONE = "CET-1CEST,M3.5.0/2,M10.5.0/3";
    using Log = void (*)(void*, const char*);
    TimeManager();
    TimeManager(TimeNetwork& network, TimeConfig config);
    void begin(uint32_t now, Log log = nullptr, void* context = nullptr);
    void update(uint32_t now);
    void setManualLocalTime(uint64_t seconds, uint32_t now);
    LocalTime localTime() const;
    const char* status() const;
    bool radioOff() const { return radioOff_; }
    bool isManual() const { return manual_; }
    const char* lastError() const { return error_; }
    static LocalTime fromUtc(int64_t epoch);
private:
    enum class Stage { NotStarted, Connecting, Ntp, Retry, Synchronized, Manual };
    TimeNetwork& network_;
    TimeConfig config_;
    Stage stage_ = Stage::NotStarted;
    Log log_ = nullptr;
    void* context_ = nullptr;
    const char* error_ = nullptr;
    char errorDetails_[160]{};
    unsigned reportedReason_ = 0;
    uint32_t stageStart_ = 0, lastUpdate_ = 0, manualRemainder_ = 0, lastShutdown_ = 0;
    LocalTime manualTime_{};
    bool manual_ = false, radioOff_ = true;
    void attempt(uint32_t now);
    void fail(const char* reason, uint32_t now);
    void shutdown(uint32_t now);
    void report(const char* message);
};
