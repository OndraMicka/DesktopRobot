# Wi-Fi time setup

The firmware uses Wi-Fi only to obtain a fresh time from NTP. Time drives the
robot's daily behavior schedule and logs; the OLED does not show a clock. After
successful synchronization, the firmware stops SNTP and turns the radio off.

## Configure saved networks

1. Copy `esp32/include/WifiConfig.example.h` to the Git-ignored
   `esp32/include/WifiConfig.h` if that private file does not already exist.
2. Add an entry to `WifiConfig::NETWORKS` for each 2.4 GHz network, using its
   SSID and password. Keep credentials in the ignored local file only.
3. Build and upload the firmware as described in the
   [firmware guide](../esp32/README.md).

`WiFiMulti` scans for the saved networks and selects an available candidate;
if a connection attempt fails, it can try another saved network. Wi-Fi radios
associate with one access point at a time, so the robot does not connect to two
networks simultaneously. The default connect timeout is 15 seconds, the NTP
timeout is 15 seconds, and retries after failure start after 30 seconds. These
values and both NTP servers are in `WifiConfig.h`.

## Startup and time behavior

- OLED startup diagnostics run first. A latched OLED ERROR prevents the time
  synchronization attempt.
- The time manager configures the ESP32's own Wi-Fi and timezone. It does not
  use the computer's internet connection or clock.
- Wi-Fi network selection uses the Arduino `WiFiMulti` implementation. Its scan
  and connection polling can briefly block the main loop (up to about one
  second per poll); behavior and animation updates otherwise continue between
  polls. SNTP synchronization itself is handled by the system callback.
- A fresh SNTP notification and a valid timestamp are required. A plausible
  system-clock value left from an earlier run does not count as a new sync.
- On success, SNTP stops, Wi-Fi disconnects, and the radio switches to
  `WIFI_OFF`. The system clock then continues offline. A reboot requires a fresh
  network sync; periodic resynchronization is not implemented.
- The local timezone is `CET-1CEST,M3.5.0/2,M10.5.0/3`. Seasonal time changes
  are converted from UTC without reconnecting.

`status` shows the active source, validity, time-manager state, and whether the
radio is on. Relevant time-manager states include `WIFI_CONNECTING`,
`NTP_WAITING`, `RETRY_WAIT`, `NTP_SYNCED`, and `MANUAL`. Successful connection
also displays a brief Wi-Fi animation on the OLED. Network failure is not a
latched hardware ERROR; without another valid time source, schedule-based state
changes remain inactive and the robot uses the IDLE fallback.

The radio profile disables Wi-Fi sleep and requests a TX power limit of
8.5 dBm (`ROBOT_WIFI_TX_POWER_QDBM=34`). Logs show the applied setting and
reported power. This is a diagnostic configuration; lower TX power can reduce
range. The source can override the limit through PlatformIO `build_flags`.

## Manual and simulated clocks

- `TIME <local epoch seconds>` sets a manual local fallback clock, stops current
  synchronization, turns off Wi-Fi, and prevents automatic attempts until the
  next reboot. For example, `TIME 43200` is 12:00 on day zero.
- `time HH:MM` selects a separate simulated clock for testing schedule
  behavior. It runs at normal speed and does not replace the real time source.
- `time auto` returns to the real NTP/manual source. It does not clear the
  manual-time lock. If no real source is valid, status reports `UNSET` and the
  schedule is inactive.
- No time setting is persisted in NVS.

## Source files

- `esp32/src/time/TimeManager.h/.cpp`: network/time state machine, NTP callback,
  retries, radio shutdown, manual time, and UTC-to-local conversion.
- `esp32/src/Robot.h/.cpp`: clock selection, status/log output, Wi-Fi feedback,
  and time delivery to `BehaviourProgram`.
- `esp32/include/WifiConfig.example.h`: public multi-network template.
- `esp32/host/TimeManagerTests.h`: fake network adapter and host time tests.

Host tests can be run with `./esp32/host/run.ps1`. These tests do not exercise a
physical access point, NTP server, or ESP32 radio. Physical verification should
confirm a fresh NTP sync, valid local time, `wifi=OFF` afterward, and behavior
while disconnected from the computer. No physical verification result is
asserted here.
