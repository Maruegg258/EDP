# EDP

Custom firmware project for the Elecrow CrowPanel ESP32 5.79-inch E-paper HMI display.

## Project goals

- Build a minimal, understandable firmware stack for the CrowPanel ESP32-S3.
- Keep the E-paper driver independent from networking code.
- Record refresh behavior and hardware assumptions in version control.
- Avoid hidden credentials and unnecessary network services.
- Use secure TLS validation for production network access.

## Planned structure

```text
EDP/
├─ README.md
├─ .gitignore
├─ docs/
│  ├─ HARDWARE.md
│  ├─ REFRESH_NOTES.md
│  └─ SECURITY.md
├─ firmware/
│  └─ CrowPanelCrypto/
│     ├─ CrowPanelCrypto.ino
│     ├─ config.example.h
│     ├─ EpaperBus.h
│     ├─ EpaperBus.cpp
│     ├─ CrowEPD579.h
│     ├─ CrowEPD579.cpp
│     ├─ GraphicsBW.h
│     └─ GraphicsBW.cpp
└─ references/
   └─ README.md
```

## Development rule

The repository is the source of truth for firmware and project notes. Tested changes should be committed with descriptive messages. Local secrets such as Wi-Fi credentials must never be committed.
