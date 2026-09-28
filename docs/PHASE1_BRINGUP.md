# Phase 1 bring-up

Phase 1 is intentionally split into small hardware checkpoints. Do not combine a new controller sequence with graphics, networking, or application features.

## Phase 1A — Reset / BUSY / command-bus sanity check

### Purpose

Verify the smallest path between the ESP32-S3 and the E-paper controller before any display RAM is written.

Sequence:

1. Enable panel power on GPIO 7.
2. Configure the known E-paper GPIO pins.
3. Perform hardware reset.
4. Wait for BUSY to become idle.
5. Send SSD1683 software reset command `0x12`.
6. Wait for BUSY to become idle again.
7. Stop.

This test does **not** write image RAM and does **not** trigger a physical refresh, so the visible E-paper contents are expected to remain unchanged.

### Serial output

At 115200 baud, a successful test should end with:

```text
EDP Phase 1A: SSD1683 reset bring-up
No framebuffer write or display refresh will be performed.
PASS: hardware reset + SWRESET completed and BUSY is idle.
```

A BUSY line that remains active for more than 5 seconds is treated as a failure instead of hanging the firmware forever.

### Evidence status

- GPIO mapping: from Elecrow board example and previously working hardware test.
- BUSY active-high behavior: matches Elecrow driver behavior.
- Hardware reset timing: matches the previously working Elecrow-derived sequence and the SSD1683-family operating flow.
- SWRESET command `0x12`: SSD1683 controller behavior and Elecrow reference.
- Dual-controller image addressing: **not tested in Phase 1A**.
- Display refresh commands: **not tested in Phase 1A**.

### Pass criterion

A PASS is only evidence that the reset / BUSY path did not time out. It is not yet proof that framebuffer addressing or refresh behavior is correct.

After a real-hardware PASS, record the result before moving to Phase 1B (white-frame RAM write + physical white refresh).
