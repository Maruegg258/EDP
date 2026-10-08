# Phase 7D — Dashboard DETAIL system actions

**Implementation committed; hardware verification pending (2026-10-08).**

## Scope

- Dashboard MENU (GPIO1) enters a two-item DETAIL menu.
- UP (GPIO4) and DOWN (GPIO6) move the selection between `STANDBY` and `DISPLAY CLEAN`.
- MENU executes the highlighted action. EXIT (GPIO2) closes DETAIL and restores the latest Dashboard image.
- Other top-level pages retain existing Phase 7C navigation behavior; they do not expose this Dashboard-specific menu.
- No new background network service, BLE service, OTA, or remote control endpoint is introduced.

## STANDBY

The UI layer draws a small monochrome crescent moon in a new framebuffer and makes no hardware calls. The Application:

1. Rejects sleep if MENU or EXIT is still held LOW.
2. Arms ESP32-S3 EXT1 `ESP_EXT1_WAKEUP_ANY_LOW` on **GPIO1 | GPIO2**. GPIO4/GPIO6/GPIO5 do not wake the chip.
3. Refreshes the moon using the existing verified previous-frame restore + full-frame partial update.
4. Calls the E-paper controller sleep command, sets **GPIO7 LOW** to disable panel power, and enables digital GPIO hold for GPIO7 during deep sleep.
5. Disables Wi-Fi and enters `esp_deep_sleep_start()`.

No timer wake is armed; normal polling, networking and firmware `loop()` stop during Deep Sleep. Bluetooth was not enabled in the production firmware and is not started as part of this feature.

On GPIO1/GPIO2 wake, ESP32-S3 resets/restarts the sketch. Firmware releases GPIO7's deep-sleep hold after configuring the output LOW, then performs normal Dashboard startup, Wi-Fi reconnect and service synchronization. A short wake-button release guard prevents treating the waking press as an ordinary MENU selection when possible.

**Important hardware qualification:** the E-paper visual persistence, GPIO7 power-gate polarity, EXT1 button wake response, retention of the wake-pin pull-ups while asleep, and real sleep current are **not yet verified in this checkpoint**. GPIO1/GPIO2 fall within the documented ESP32-S3 RTC GPIO 0–21 range, and the board's external pull-ups were previously verified for active-low button use in normal operation. The electrical sleep-current and external pull-up behavior still require practical testing. A USB-connected development board may draw more than bare ESP32 deep-sleep current.

## DISPLAY CLEAN

This action is **not** SSD1683 raw Full Refresh. It recomposes the newest Dashboard state, then uses the Phase 1F hardware-verified `CrowEPD579::maintenanceRefresh()`:

```text
fast-mode reset/init
-> fast clear to physical white
-> fast-mode reset/init
-> previous RAM white
-> current RAM newest Dashboard
-> partial refresh
-> synchronize previous RAM to newest Dashboard
```

The Application updates its saved physical framebuffer after success and commits pending Dashboard state only after the physical operation succeeds. It returns to PAGE DASHBOARD without reboot or extra E-paper refresh. The existing no-periodic-Full-Refresh rule remains unchanged.

## Regression checks requested

- From PAGE DASHBOARD, MENU displays two items; UP/DOWN select; EXIT returns to newest Dashboard.
- Leave DETAIL visible across a scheduled clock/market update: it should not be overwritten automatically.
- Select DISPLAY CLEAN and verify refreshed Dashboard remains crisp. Verify later minute/price partial updates are not blurry.
- Select STANDBY: only the small moon remains; check no further Serial polling/output and no Wi-Fi connection. Test MENU and EXIT wake separately.
- Confirm that UP/DOWN cannot wake from Deep Sleep.
- Confirm wake restores production Dashboard, Wi-Fi, NTP and weather/market retrieval.
- Recheck transition to WEATHER/MARKETS after wake; ensure no stale framebuffer artifacts.
- Measure sleep power independently if low-current verification is required.

Do not mark hardware PASS until the above hardware tests have been observed.
