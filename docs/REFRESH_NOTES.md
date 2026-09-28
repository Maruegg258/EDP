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
