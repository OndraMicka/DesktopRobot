#pragma once
#include "time/TimeManager.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

struct FakeTimeNetwork : TimeNetwork {
    Connection link=Connection::Pending;
    bool started=false, ntp=false, off=true, notified=false, startOK=true, stopOK=true;
    unsigned connections=0, shutdowns=0, reason=0;
    int64_t epoch=0;
    std::string timezone;
    void configureTimezone(const char* tz) override { timezone=tz; }
    bool connect(const char*,const char*) override { ++connections; started=true; off=false; return startOK; }
    Connection connection() const override { return link; }
    unsigned disconnectReason() const override { return reason; }
    void startNtp(const char* tz,const char*,const char*) override { ntp=true; notified=false; timezone=tz; }
    bool takeSync(int64_t& value) override { if(!notified) return false; notified=false; value=epoch; return true; }
    int64_t utcNow() const override { return epoch; }
    bool stop() override { ++shutdowns; ntp=false; off=stopOK; return off; }
};
inline void testTimeManager() {
    TimeConfig config; config.ssid="test-network"; config.password="test-only";
    config.connectTimeoutMs=100; config.ntpTimeoutMs=200; config.retryIntervalMs=500;
    constexpr int64_t epoch=1768478400; // 2026-01-15 12:00 UTC
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0);
        assert(!tm.localTime().valid && !tm.radioOff());
        n.link=TimeNetwork::Connection::Connected; n.epoch=epoch;
        tm.update(10); assert(n.ntp && !tm.localTime().valid);
        tm.update(100); assert(!tm.localTime().valid); // An already plausible RTC is NOT an NTP result.
        n.notified=true; tm.update(101);
        assert(tm.localTime().valid && tm.localTime().second==13*3600 && n.off && !n.ntp);
        assert(tm.radioOff() && std::strcmp(tm.status(),"NTP_SYNCED")==0);
        n.epoch+=3600; tm.update(100000); assert(tm.localTime().second==14*3600 && n.connections==1);
        assert(n.timezone==TimeManager::TIMEZONE);
        // Restart repeats synchronization and does not trust the retained epoch alone.
        tm.begin(100001); assert(n.connections==2 && !tm.localTime().valid);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); tm.update(100);
        assert(n.off && !tm.localTime().valid && std::strstr(tm.lastError(),"Wi-Fi connection timeout"));
        tm.update(599); assert(n.connections==1); tm.update(600); assert(n.connections==2 && !n.off);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0);
        n.link=TimeNetwork::Connection::Connected; tm.update(1); tm.update(201);
        assert(n.off && !n.ntp && !tm.localTime().valid && std::strstr(tm.lastError(),"NTP response timeout"));
        tm.update(701); assert(n.connections==2);
    }
    for(auto failure:{TimeNetwork::Connection::NoAccessPoint,TimeNetwork::Connection::AuthenticationFailed}) {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); n.link=failure; tm.update(1);
        assert(n.off && tm.lastError() && !tm.localTime().valid);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); n.link=TimeNetwork::Connection::Connected; tm.update(1);
        n.link=TimeNetwork::Connection::Disconnected; tm.update(2);
        assert(n.off && std::strstr(tm.lastError(),"disconnected"));
    }
    {
        FakeTimeNetwork n; n.startOK=false; TimeManager tm(n,config); tm.begin(0);
        assert(n.off && tm.lastError() && !tm.localTime().valid);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); n.link=TimeNetwork::Connection::Connected; tm.update(1);
        n.epoch=0; n.notified=true; tm.update(2); assert(n.off && !tm.localTime().valid);
    }
    for(bool whileNtp:{false,true}) {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0);
        if(whileNtp) { n.link=TimeNetwork::Connection::Connected; tm.update(1); }
        tm.setManualLocalTime(6*3600,10); n.notified=true; n.epoch=epoch;
        tm.update(1010); assert(tm.isManual() && tm.localTime().second==6*3600+1 && n.off && !n.ntp);
        tm.update(1000010); assert(n.connections==1 && tm.isManual());
        tm.begin(1000011); assert(!tm.isManual() && !tm.localTime().valid && n.connections==2);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(UINT32_MAX-50); tm.update(49);
        assert(n.off && tm.lastError());
        tm.setManualLocalTime(86399,UINT32_MAX-50); tm.update(949);
        assert(tm.localTime().day==1 && tm.localTime().second==0);
    }
    {
        FakeTimeNetwork n; TimeConfig missing; TimeManager tm(n,missing); tm.begin(0);
        assert(!n.connections && n.off && tm.lastError() && !tm.localTime().valid);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); n.link=TimeNetwork::Connection::Connected; tm.update(1);
        n.epoch=epoch; n.notified=true; n.stopOK=false; tm.update(2); assert(tm.localTime().valid && !tm.radioOff());
        n.stopOK=true; tm.update(1002); assert(tm.radioOff() && n.off);
    }
    {
        FakeTimeNetwork n; TimeManager tm(n,config); tm.begin(0); n.reason=202; tm.update(100);
        assert(std::strstr(tm.lastError(),"reason=202 (AUTH_FAIL)") && n.off);
    }
    // UTC inputs are independent of the host timezone; exact Czech DST boundaries.
    assert(TimeManager::fromUtc(1768478400LL).second==46800); // 2026-01-15T12:00:00 UTC
    assert(TimeManager::fromUtc(1784116800LL).second==50400); // 2026-07-15T12:00:00 UTC
    assert(TimeManager::fromUtc(1774745999LL).second==7199); // 2026-03-29T00:59:59 UTC
    assert(TimeManager::fromUtc(1774746000LL).second==10800); // 2026-03-29T01:00:00 UTC
    assert(TimeManager::fromUtc(1792889999LL).second==10799); // 2026-10-25T00:59:59 UTC
    assert(TimeManager::fromUtc(1792890000LL).second==7200); // 2026-10-25T01:00:00 UTC
    assert(TimeManager::fromUtc(1837645200LL).second==10800); // 2028-03-26T01:00:00 UTC
    assert(TimeManager::fromUtc(1856394000LL).second==7200); // 2028-10-29T01:00:00 UTC
    assert(TimeManager::fromUtc(1768519800LL).day==20469 && TimeManager::fromUtc(1768519800LL).second==1800);
    assert(!TimeManager::fromUtc(0).valid);
    std::cout<<"PASS: verified NTP, radio OFF, offline clock, timeouts/retry, restart, manual priority, rollover, Czech winter/summer and DST boundaries\n";
}
