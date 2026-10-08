# Hardware notes

## Verified on the development unit

- MCU: ESP32-S3, revision v0.2
- External flash: 8 MB, Quad I/O, 3.3 V
- Embedded PSRAM: 8 MB
- Display: CrowPanel 5.79-inch black/white E-paper
- Physical visible resolution: 792 x 272
- Vendor driver model: dual SSD1683 controllers
- Vendor framebuffer/addressing width: 800 x 272, with an 8-pixel controller seam offset
- Hardware-verified visible mapping on the development unit: 396 visible pixels + 8 logical seam pixels + 396 visible pixels = 800 controller-addressed pixels

## Board connections used by the vendor example

| Signal | GPIO |
| --- | ---: |
| E-paper power | 7 |
| SCK | 12 |
| MOSI | 11 |
| RESET | 47 |
| DC | 46 |
| CS | 45 |
| BUSY | 48 |

These values are hardware facts used to bootstrap our own driver. Controller command behavior must be checked against the SSD1683 documentation and our own measurements before it is treated as canonical.

## Flash state

The development unit was fully erased with esptool before this repository was initialized. The complete 8 MB pre-erase flash image was backed up separately and is not stored in this public repository.


## Phase 7 physical controls — reference mapping

The following mapping is taken from the Elecrow 5.79-inch board documentation and its `5.79_key` Arduino example. It is retained as the **vendor/reference naming**, separate from the EDP application's logical control mapping.

Phase 7A-1 hardware testing on 2026-10-06 verified that GPIO1, GPIO2, GPIO4, and GPIO6 produce stable active-low input events on the development unit with the project-owned 30 ms debounce path.

| Control | GPIO | Reference behavior | Phase 7A-1 use |
| --- | ---: | --- | --- |
| MENU | 2 | Active LOW | Serial event probe |
| EXIT | 1 | Active LOW | Serial event probe |
| Rotary UP | 6 | Active LOW | Serial event probe |
| Rotary DOWN | 4 | Active LOW | Serial event probe |
| Rotary CONF / push | 5 | Active LOW | Intentionally unused |

The board circuitry provides pull-up resistors for these inputs. The Elecrow example configures them as `INPUT` and treats LOW as active.

Phase 7A-1 uses a non-blocking 30 ms software debounce and reports one event after a stable release. The four tested inputs behaved normally on hardware.

No Phase 7A-1 input event controls the dashboard or triggers E-paper refresh. GPIO5 is initialized as an input but produces no application event.

## Phase 7A-2 EDP logical control mapping

The EDP firmware intentionally assigns application semantics according to the panel's actual physical use rather than preserving Elecrow's example labels. This is a software mapping only; the board wiring is unchanged.

| EDP logical action | GPIO | Intended UI meaning |
| --- | ---: | --- |
| MENU | 1 | Confirm / enter / open details |
| UP | 4 | Previous page / previous option |
| DOWN | 6 | Next page / next option |
| EXIT | 2 | Cancel / return to previous level |
| Rotary push | 5 | Intentionally unused |

`ButtonManager` now emits provider-independent application events `MENU`, `UP`, `DOWN`, and `EXIT`. Phase 7A-2 remains Serial-only: these events do not yet perform page navigation or trigger an E-paper refresh.

### 2026-10-06 Phase 7A-2 hardware verification

**PASS on the development unit.** GPIO1 produced `MENU`, GPIO4 produced `UP`, GPIO6 produced `DOWN`, and GPIO2 produced `EXIT` through the debounced release-event path. GPIO5 remained intentionally unused. No input event affected the production dashboard or triggered E-paper refresh.


## Phase 7D standby / wake — functionally hardware verified

- **GPIO1 (EDP MENU) and GPIO2 (EDP EXIT)**: selected ESP32-S3 EXT1 `ANY_LOW` deep-sleep wake pins. These two pins are in the ESP32-S3 RTC-GPIO 0–21 range. Board-normal active-low events were verified in Phase 7A; MENU/EXIT wake behavior has now passed the Phase 7D functional test. Pull-up resistance and actual sleep-state pin voltage have not been electrically measured.
- **GPIO4 / GPIO6 rotary input and GPIO5 push**: intentionally excluded from deep-sleep wake.
- **GPIO7 E-paper power enable**: project driver powers the panel with HIGH during normal operation. Phase 7D drives LOW during deep sleep with `gpio_hold_en` + `gpio_deep_sleep_hold_en` and unconditionally clears the hold on every boot. Standby and subsequent wake/display operation passed the functional test. Actual rail voltage/current and quantitative GPIO7 LOW retention still require electrical measurement.
- USB-powered full-board standby current may differ substantially from ESP32-S3 chip-only deep-sleep current.

### 2026-10-08 hardware verification

**Functional PASS:** the user confirmed the Phase 7D standby/wake tests were normal on the development panel. EXT1 MENU/EXIT wake and restoration of the application are treated as functionally verified, while GPIO7 rail voltage, wake-pin electrical levels, and complete-board sleep current remain **unmeasured**. On 2026-10-08, the user explicitly excluded electrical power measurement from the current USB-powered, no-battery scope; it is not a pending acceptance requirement. No quantitative power-consumption claim is supported. See [PHASE7_SYSTEM_ACTIONS.md](PHASE7_SYSTEM_ACTIONS.md).
