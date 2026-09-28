# Hardware notes

## Verified on the development unit

- MCU: ESP32-S3, revision v0.2
- External flash: 8 MB, Quad I/O, 3.3 V
- Embedded PSRAM: 8 MB
- Display: CrowPanel 5.79-inch black/white E-paper
- Physical visible resolution: 792 x 272
- Vendor driver model: dual SSD1683 controllers
- Vendor framebuffer/addressing width: 800 x 272, with an 8-pixel controller seam offset

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
