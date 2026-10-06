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

The following mapping is taken from the Elecrow 5.79-inch board documentation and its `5.79_key` Arduino example. It is a **vendor/reference fact pending verification on our development unit** for Phase 7A-1.

| Control | GPIO | Reference behavior | Phase 7A-1 use |
| --- | ---: | --- | --- |
| MENU | 2 | Active LOW | Serial event probe |
| EXIT | 1 | Active LOW | Serial event probe |
| Rotary UP | 6 | Active LOW | Serial event probe |
| Rotary DOWN | 4 | Active LOW | Serial event probe |
| Rotary CONF / push | 5 | Active LOW | Intentionally unused |

The board circuitry provides pull-up resistors for these inputs. The Elecrow example configures them as `INPUT` and treats LOW as active.

Phase 7A-1 uses a non-blocking 30 ms software debounce and reports one event after a stable release. It deliberately retains Elecrow's `UP` / `DOWN` naming until physical clockwise/counter-clockwise or left/right direction is verified on our unit.

No Phase 7A-1 input event controls the dashboard or triggers E-paper refresh. GPIO5 is initialized as an input but produces no application event.
