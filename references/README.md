# References

External material used to understand the hardware is recorded here. External source code is reference material, not the source of truth for this project.

## Elecrow

Repository:

`Elecrow-RD/CrowPanel-ESP32-5.79-E-paper-HMI-Display-with-272-792`

Relevant vendor example:

`example/arduino/Examples/5.79_wifi`

Information taken as hardware/reference input includes:

- board GPIO mapping
- dual-controller display architecture
- 792 x 272 visible geometry
- 800 x 272 vendor framebuffer/addressing convention
- known SSD1683 command sequences to be independently cross-checked

The vendor repository did not advertise a repository-level license in GitHub metadata when this project was initialized. Therefore this project should avoid wholesale copying of vendor implementation code and instead reimplement behavior from hardware facts, controller documentation, and our own tests.

## Next primary reference

Obtain and cross-check the SSD1683 controller datasheet before implementing the production refresh sequence.
