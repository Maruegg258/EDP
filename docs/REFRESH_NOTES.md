# Refresh behavior notes

This document records behavior observed on the development panel. It is intentionally separate from controller assumptions.

## Observed problem

A full refresh or fast full-screen refresh followed immediately by a partial refresh can produce blurred text / ghost-like residuals on this panel.

## Working maintenance pattern

The stable pattern tested before this repository was created is:

1. Fast-clear the physical panel to white.
2. Re-initialize the fast-mode controller state.
3. Set the controller's previous-frame RAM to white.
4. Build the newest dashboard in the ESP32 framebuffer.
5. Write the newest dashboard as current-frame RAM.
6. Perform a partial update from white to the dashboard.
7. Put the display controller to sleep.

Conceptually:

```text
physical panel = white
previous RAM   = white
current RAM    = new dashboard
        |
        +--> partial update
```

## Normal updates

For ordinary price changes, use partial refresh only.

## First boot

First boot needs a known clean baseline before partial-update state is trusted. The exact SSD1683 command sequence will be reimplemented and documented in the custom driver after datasheet cross-checking.

## Rule

Do not reintroduce periodic full refresh merely because it is conventional. Changes to refresh strategy must be tested on hardware and documented here.

## Phase 1F custom-driver reconstruction

The custom driver now contains an explicit maintenance primitive matching the previously stable physical sequence:

```text
fast-mode reset/init
-> fast clear to physical white
-> fast-mode reset/init
-> previous RAM white
-> current RAM new frame
-> partial refresh
-> previous RAM synchronized to new frame
```

The final synchronization step is intentionally retained so later normal updates can use the same coherent previous/current/physical-state model verified in Phase 1E.

### 2026-09-28 hardware verification

**PASS on the development panel.** The maintenance operation completed correctly and three ordinary partial updates immediately afterward remained sharp, with no obvious blurred/doubled text or residual square artifacts.

This custom-driver maintenance sequence is now the hardware-verified cleanup path for the project.


## Phase 7C-1 full-page navigation verification

### 2026-10-06 hardware verification

**PASS on the development panel.** Top-level page changes between the production DASHBOARD and deliberately sparse WEATHER / MARKETS placeholder frames were performed with the existing full-frame partial path:

```text
begin / reset
-> restore previous physical frame into controller RAM
-> write newly rendered full framebuffer
-> partial refresh
-> synchronize previous RAM to the new visible frame
-> sleep
```

Forward and reverse page cycling were both stable. Leaving a non-dashboard page visible while background Clock / Wi-Fi / Market / Weather data continued to stage did not cause the dashboard to overwrite the selected page. Returning to DASHBOARD rendered the newest pending dashboard state.

No additional Full Refresh was required for page switching, and the hardware test did not reveal a need to change the existing refresh strategy. The established rule remains: do not add periodic or page-change Full Refresh behavior without new hardware evidence.


## Phase 7C-3 live market page verification

### 2026-10-07 hardware verification

**PASS on the development panel.** The MARKETS page displayed live last-valid BTC / ETH / HYPE prices and refreshed in place when the visible price strings changed during the existing 60-second production market polling flow.

The application continued to own all display decisions: `MarketDataService` updated data only, visible-price comparison decided whether the current MARKETS page needed a refresh, and the physical update used the existing verified full-frame partial path. Market updates did not force a return to DASHBOARD, and returning to DASHBOARD restored the newest pending dashboard state correctly.

Repeated page switching and in-place market refreshes remained visually clear in hardware testing. No new Full Refresh behavior was required.


## Phase 7D manual DISPLAY CLEAN (implementation pending hardware verification)

### 2026-10-08 design checkpoint

The Dashboard DETAIL menu now provides an on-demand **DISPLAY CLEAN** action. It is explicitly **not** an SSD1683 raw Full Refresh and it does not add periodic Full Refresh behavior. The Application redraws the latest Dashboard and calls the existing Phase 1F hardware-verified `CrowEPD579::maintenanceRefresh(frameBuffer)` sequence, synchronizes `previousFrameBuffer`, commits the Dashboard coalescer's pending snapshot only after successful display operation, and returns to the Dashboard.

The Phase 1F primitive has already been hardware-verified, but invoking it from the new DETAIL action has **not** yet been verified on hardware. The regression test must examine the screen immediately after maintenance **and** subsequent ordinary partial updates, especially for blurred text / ghosting. Do not label this checkpoint PASS until the new UI-to-maintenance integration has been observed on the panel.

The STANDBY moon uses an ordinary previous-frame-restore + partial update before the controller sleeps; it does not require Full Refresh.
