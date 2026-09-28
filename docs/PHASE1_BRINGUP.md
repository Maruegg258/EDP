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


## Hardware verification log

### 2026-09-28 — Phase 1A PASS

Observed serial output on the development CrowPanel:

```text
EDP Phase 1A: SSD1683 reset bring-up
No framebuffer write or display refresh will be performed.
PASS: hardware reset + SWRESET completed and BUSY is idle.
```

Result: **PASS on real hardware.**

What this verifies on the tested unit:

- GPIO 7 successfully enables the E-paper power path.
- The configured RESET and BUSY GPIOs are usable with the current board.
- BUSY active-high handling completes without timeout.
- Hardware reset timing is accepted by the panel/controller setup.
- Sending SSD1683 `SWRESET (0x12)` over the current command bus path completes and BUSY returns idle.

What this does **not** yet verify:

- image-RAM addressing
- master/slave controller split
- the 8-pixel seam/address offset
- white/black framebuffer polarity
- physical refresh commands
- fast or partial refresh behavior

Next step: **Phase 1B — white-frame RAM write + physical white refresh.**


## Phase 1B — White-frame RAM + physical white refresh

### Purpose

Verify the first physical display operation using our own driver implementation.

The test deliberately renders no text or graphics. It establishes a known all-white baseline across both SSD1683 controllers.

### Test sequence

1. Run the already-verified Phase 1A reset/SWRESET path.
2. Configure the refresh environment used by the previously verified CrowPanel sequence.
3. Configure the master SSD1683 RAM window as 400 x 272.
4. Write master current RAM (`0x24`) as white.
5. Prepare the master previous plane (`0x26`) for the full clear cycle.
6. Configure the cascaded/slave SSD1683 RAM window as 400 x 272 with reversed X addressing.
7. Write slave current RAM (`0xA4`) as white.
8. Prepare the slave previous plane (`0xA6`) for the full clear cycle.
9. Trigger one full update with `0x22 = 0xF7`, followed by `0x20`.
10. After the physical panel reaches white, write both previous RAM planes as white.
11. Enter controller deep sleep.

### Expected serial output

```text
EDP Phase 1B: dual-SSD1683 white-screen bring-up
This test will perform one physical full refresh.
Step 1/3: reset controllers...
Step 2/3: write white RAM and refresh panel...
Step 3/3: enter controller deep sleep...
PASS: physical panel should now be uniformly white.
Inspect the full 792x272 visible area, especially the center seam.
```

### Expected physical result

The complete 792 x 272 visible panel should become uniformly white.

Inspect especially:

- left half
- right half
- the center controller seam
- top and bottom edges

A serial `PASS` only means the command sequence completed without a BUSY timeout. Phase 1B is considered hardware-verified only after the physical panel is visually confirmed to be uniformly white.

### Evidence status before hardware test

- SSD1683 supports 400 source x 300 gate outputs and cascade operation.
- The panel is 792 x 272 and uses SSD1683.
- The master/slave 400 x 272 addressing convention and secondary-controller command set are based on the Elecrow reference sequence that previously worked on this exact development unit.
- Register behavior is being isolated behind our own driver API rather than exposing vendor-style register calls to the application layer.
- The full-white result for this new implementation is **not yet hardware-verified**.


### 2026-09-28 — Phase 1B PASS

Observed serial output on the development CrowPanel:

```text
EDP Phase 1B: dual-SSD1683 white-screen bring-up
This test will perform one physical full refresh.
Step 1/3: reset controllers...
Step 2/3: write white RAM and refresh panel...
Step 3/3: enter controller deep sleep...
PASS: physical panel should now be uniformly white.
Inspect the full 792x272 visible area, especially the center seam.
```

Physical result: **PASS on real hardware.** The entire visible 792 x 272 panel became uniformly white. Left/right halves, center seam, and panel edges appeared normal.

What this verifies on the tested unit:

- master and slave SSD1683 RAM addressing paths are both operational
- the 400 x 272 per-controller RAM depth used by this driver reaches both halves of the panel
- the current-frame RAM white polarity is correct for the tested clear sequence
- the full-refresh command path completes successfully
- the cascaded controller seam does not show an obvious alignment or refresh defect in an all-white frame
- controller deep sleep after the refresh does not disturb the displayed white image

Still not verified by this checkpoint:

- black pixel/data polarity
- non-uniform image mapping across the 800-wide logical framebuffer
- exact visible 792-pixel mapping and 8-pixel seam offset for graphics
- partial refresh using this custom driver
- fast refresh and maintenance-refresh behavior

Next step: **Phase 1C — simple black test pattern.**


## Phase 1C — Black geometry and seam mapping test

### Purpose

Verify non-uniform black/white image data across both SSD1683 controllers before introducing fonts.

This checkpoint tests:

- black pixel polarity
- raw 800 x 272 framebuffer transfer
- 792 x 272 visible-coordinate mapping
- orientation
- both panel halves
- the 396 + 8 + 396 controller seam convention

### Test pattern

The firmware builds a white raw framebuffer and draws:

- asymmetric black blocks in all four visible corners
- one horizontal black line crossing most of the display
- one vertical reference line on each controller half
- a black outline rectangle crossing the center seam
- a 4-pixel-wide vertical marker spanning visible x=394..397

The visible-to-controller mapping used for this test is:

```text
visible x 0..395   -> raw x 0..395
raw x     396..403 -> controller seam gap (not drawn)
visible x 396..791 -> raw x 404..799
```

This mapping comes from the Elecrow reference implementation and is **not considered independently hardware-verified until this test passes visually**.

### Expected serial output

```text
EDP Phase 1C: black geometry + seam mapping test
This test performs one full refresh with a non-uniform frame.
Step 1/4: reset controllers...
Step 2/4: build 792x272 visible test pattern...
Step 3/4: write dual-controller frame and refresh...
Step 4/4: enter controller deep sleep...
PASS: command sequence completed.
Physical inspection is REQUIRED before Phase 1C is accepted.
Check corners, orientation, center rectangle, and center seam.
```

### Physical acceptance criteria

Phase 1C passes only if all of the following are visually true:

1. Background is white and all intended test marks are black.
2. All four corner blocks appear in their expected corners.
3. Top/bottom and left/right orientation is correct.
4. The long horizontal line is continuous across the center.
5. The center rectangle crosses the controller boundary without a visible 8-pixel hole.
6. The x=394..397 seam marker appears as one continuous narrow black bar.
7. No half of the panel is mirrored, shifted, missing, or inverted.

If any condition fails, do not proceed to text rendering. Record the observed geometry and correct the mapping first.


### 2026-09-28 — Phase 1C PASS

Serial result: **PASS as expected.**

Physical result: **PASS on real hardware.** The complete geometry test rendered correctly.

Verified by this test:

- black pixel/data polarity is correct
- the raw 800 x 272 framebuffer transfer is correct
- visible orientation is correct
- both SSD1683-controlled halves render the intended geometry
- the visible-coordinate mapping `0..395 -> raw 0..395` and `396..791 -> raw 404..799` is correct on this panel
- the 8-pixel logical controller seam gap is correctly hidden from the 792-pixel visible coordinate space
- graphics crossing the controller boundary remain visually continuous
- the center seam marker, center rectangle, long horizontal line, and asymmetric corner markers all displayed normally

Phase 1C therefore promotes the previously Elecrow-derived `396 + 8 + 396` mapping from a reference assumption to an **observed hardware-verified behavior on the development unit**.

Still not verified:

- text/font rendering in our graphics layer
- partial refresh using the custom driver
- fast refresh using the custom driver
- the maintenance refresh sequence in the new driver implementation

Next step: **Phase 1D — render `HELLO` using our own graphics/text path.**


## Phase 1D — Project-owned `HELLO` text rendering

### Purpose

Verify a minimal text-rendering path without using Elecrow's `EPDfont.h`.

The Phase 1D font is owned by this project and intentionally implements only the four glyphs required by `HELLO`: `H`, `E`, `L`, and `O`. Each glyph is a simple 5 x 7 bitmap. The complete font and typography system remain Phase 2 work.

### Rendering path

```text
Font5x7 bitmap
    ->
visible-coordinate glyph renderer
    ->
396 + 8 + 396 seam mapping
    ->
800 x 272 raw framebuffer
    ->
dual SSD1683 full-frame writer
    ->
physical panel
```

The word `HELLO` is rendered at 8x scale and centered horizontally and vertically. Its rendered width crosses the center controller boundary, so successful output also confirms that text pixels remain continuous across the seam mapping verified in Phase 1C.

### Expected serial output

```text
EDP Phase 1D: project-owned HELLO text rendering
This test uses a minimal 5x7 font created for EDP.
Step 1/4: reset controllers...
Step 2/4: render HELLO into the 800x272 framebuffer...
Step 3/4: write frame and perform full refresh...
Step 4/4: enter controller deep sleep...
PASS: command sequence completed.
Physical inspection is REQUIRED before Phase 1D is accepted.
Expected result: centered black HELLO on a white background.
```

### Physical acceptance criteria

Phase 1D passes only if:

1. The background is white.
2. `HELLO` is clearly readable in black.
3. In the chosen product-use orientation, `HELLO` is upright and readable; the glyphs are not mirrored or internally inverted.
4. Stroke blocks are square and not garbled.
5. Letter spacing is consistent.
6. The word crosses the controller boundary without a visible discontinuity.
7. No unexpected geometry from the previous Phase 1C frame remains after the full refresh.

A successful Phase 1D verifies only this minimal text path. It does not yet certify the future full font system, font metrics, or partial-refresh behavior.


### 2026-09-28 — Phase 1D PASS

Serial result: **PASS as expected.**

Physical result: **PASS on real hardware.** `HELLO` rendered correctly in black on a white background. The word was readable, glyph strokes and spacing were normal, and the text crossed the controller seam without visible discontinuity.

Orientation note:

The panel has no meaningful absolute "up" independent of how the product is mounted. For this project, orientation is defined relative to the chosen normal-use mounting direction. Rotating the complete physical panel 180 degrees naturally rotates the displayed content with it and does not indicate a framebuffer error.

For Phase 1D, the relevant checks are therefore:

- glyphs are not mirrored
- glyphs are not internally upside-down relative to the chosen normal-use orientation
- left-to-right character order is correct
- text crosses the controller seam continuously

Verified by Phase 1D:

- the project-owned minimal glyph bitmap is correct for `HELLO`
- glyph-to-pixel rendering works
- scaled text rendering works
- visible-coordinate mapping remains correct for text
- text can cross the dual-controller seam without corruption
- the previous Phase 1C geometry is fully replaced by the new full frame

Still not verified:

- a complete production font set
- proportional font metrics or typography polish
- partial refresh using the custom driver
- fast refresh using the custom driver
- maintenance-refresh behavior using the custom driver

Next step: **Phase 1E — partial refresh and previous/current RAM synchronization.**


## Phase 1E-A — Consecutive partial refresh without sleep/reset

### Purpose

Verify the custom driver's partial-refresh path while removing deep sleep and hardware reset as variables.

### State model

Before every partial update, the expected state is:

```text
previous RAM = physical image A
current RAM  = physical image A
```

The test then rebuilds the complete framebuffer as image B and performs:

```text
current RAM = B
    ->
partial refresh (0x22 = 0xDC, then 0x20)
    ->
physical panel = B
    ->
previous RAM = B
```

The previous RAM synchronization happens only after BUSY reports that the physical partial refresh has completed.

### Test sequence

1. Reset/SWRESET once.
2. Build a baseline containing `HELLO` plus one 32 x 32 black square.
3. Perform one full refresh; `displayFullFrame()` synchronizes previous/current RAM to the baseline.
4. Wait 3 seconds.
5. Rebuild the entire framebuffer with the same `HELLO` and move the square.
6. Write only the new current frame, perform partial refresh, then copy that frame to previous RAM.
7. Repeat the move/partial/sync cycle four times total.
8. Do **not** deep-sleep or hardware-reset the controller during the sequence.

Square visible X positions are 80 (baseline), then 240, 400, 560, and 680.

### Physical acceptance criteria

Phase 1E-A passes only if:

1. Each partial refresh moves the black square to the new position.
2. The old square disappears cleanly after each move.
3. Only one square is visible after every update.
4. `HELLO`, which is unchanged in every framebuffer, remains equally sharp after all four partial updates.
5. No new double edges, bolding, light ghost text, or controller-seam artifact appears.
6. The fourth partial update behaves as cleanly as the first.

Serial `PASS` only confirms that all refresh commands completed without BUSY timeout. Visual inspection is required before this checkpoint is accepted.

### Scope

This test intentionally does **not** enter deep sleep and does not hardware-reset between partial updates. If Phase 1E-A passes, Phase 1E-B will add sleep/reset to isolate whether controller reset state affects previous/current RAM behavior.


### 2026-09-28 — Phase 1E-A PASS

Serial result: **PASS as expected.**

Physical result: **PASS on real hardware.** Across the four consecutive partial updates, the moving square updated correctly, prior square positions did not leave obvious residual images, and `HELLO` remained visually normal.

Verified by Phase 1E-A:

- the custom partial-refresh command path completes repeatedly without BUSY timeout
- current-frame RAM can be replaced with a new full frame before each partial update
- previous RAM synchronization after BUSY idle is sufficient for consecutive awake-state partial updates on this development unit
- unchanged text can survive at least four consecutive partial updates without obvious blur, double edges, or visible ghost accumulation
- old moving-object positions are cleared cleanly enough to avoid obvious residual squares
- no controller reset or deep sleep is required between these consecutive partial updates

Important scope limit:

This result applies only while the SSD1683 controllers remain awake and are not hardware-reset between updates. It does **not** yet prove that previous/current RAM state survives deep sleep or that our reset/reinitialization path reconstructs that state correctly.

Next step: **Phase 1E-B — partial refresh across deep-sleep / hardware-reset cycles.**
