# CNC Catastrophe

**A two-axis Arduino-based CNC pen plotter** using NEMA 17 stepper motors, DRV8825 drivers, lead screws and a servo-operated pen lift.

The active v2 implementation is in [cnc-catastrophe-v2](cnc-catastrophe-v2/README.md). The original prototype is preserved.

> Status: implementation and documentation baseline. GitHub Actions passes the Arduino Uno firmware compile and five Python converter tests. Physical machine commissioning and drawing tests remain outstanding.

## Start here
- [Project setup and overview](cnc-catastrophe-v2/README.md)
- [System architecture](cnc-catastrophe-v2/docs/ARCHITECTURE.md)
- [Hardware wiring and safety](cnc-catastrophe-v2/docs/HARDWARE.md)
- [Bill of materials](cnc-catastrophe-v2/docs/BOM.md)
- [Windows setup guide](cnc-catastrophe-v2/docs/WINDOWS_SETUP.md)
- [Calibration procedure](cnc-catastrophe-v2/docs/CALIBRATION.md)
- [Serial protocol](cnc-catastrophe-v2/docs/PROTOCOL.md)
- [Validation plan](cnc-catastrophe-v2/docs/VALIDATION.md)
- [Troubleshooting](cnc-catastrophe-v2/docs/TROUBLESHOOTING.md)
- [Technical walkthrough](cnc-catastrophe-v2/docs/PROJECT_WALKTHROUGH.md)

## Data flow
SVG artwork → SVG-to-G-code converter → G-code file → Python serial sender → Arduino Uno → DRV8825 drivers → X/Y steppers and pen servo.

## Hardware baseline
Arduino Uno · 2 × NEMA 17 · 2 × DRV8825 · 8 mm lead screws · NC home switches · pen servo.

Configured resolution: **400 steps/mm** = 200 full steps/rev × 16 microsteps ÷ 8 mm/rev.
