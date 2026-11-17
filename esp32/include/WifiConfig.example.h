#pragma once
#include <cstdint>

// Copy to WifiConfig.h and fill in your 2.4 GHz networks. Never commit that file.
namespace WifiConfig {
inline constexpr WifiCredential NETWORKS[] = {
    {"", ""}, // Home
    {"", ""}, // Another place
};
inline constexpr std::size_t NETWORK_COUNT = sizeof(NETWORKS) / sizeof(NETWORKS[0]);
inline constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
inline constexpr uint32_t NTP_TIMEOUT_MS = 15000;
inline constexpr uint32_t RETRY_INTERVAL_MS = 30000;
inline constexpr const char* NTP_SERVER_1 = "pool.ntp.org";
inline constexpr const char* NTP_SERVER_2 = "time.cloudflare.com";
}
