# CNC Catastrophe

[![Python checks](https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/python-checks.yml/badge.svg?branch=main)](https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/python-checks.yml?query=branch%3Amain) [![Arduino build](https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/arduino-build.yml/badge.svg?branch=main)](https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/arduino-build.yml?query=branch%3Amain)


Two-axis lead-screw pen-plotter prototype for Arduino Uno, two NEMA 17 motors, two DRV8825 drivers, normally-closed home switches and a pen-lift servo.

> Development snapshot. GitHub Actions successfully compiles the firmware for Arduino Uno and passes eight Python tests. The firmware has not been tested on the physical machine; motion, travel, current and safety behaviour remain unvalidated.

## Configured machine

| Parameter | Value |
|---|---:|
| Controller | Arduino Uno |
| Drivers | 2 x DRV8825 |
| Motor resolution | 200 full steps/rev |
| Microstepping | 1/16 |
| Lead screw lead | 8 mm/rev |
| Calculated resolution | 400 steps/mm |
| Configured workspace | 150 x 150 mm |
| Pen servo | D6 |

Resolution calculation: 200 x 16 / 8 = 400 steps/mm. This assumes identical motors, microstep settings and screw leads on both axes.

## Layout

- firmware/CNC_Catastrophe/CNC_Catastrophe.ino - Arduino firmware
- tools/svg_to_gcode.py - SVG path converter
- tools/cnc_sender.py - stop-and-wait serial sender
- examples/test_square.svg - example drawing
- docs/ARCHITECTURE.md - system architecture and data flow
- docs/BOM.md - hardware bill of materials and configuration record
- docs/WINDOWS_SETUP.md - Windows installation and operation guide
- docs/HARDWARE.md - wiring and safety notes
- docs/CALIBRATION.md - commissioning and calibration procedure
- docs/PROTOCOL.md - supported serial commands
- docs/VALIDATION.md - software, firmware and hardware test plan
- docs/TROUBLESHOOTING.md - common faults and recovery
- docs/PROJECT_WALKTHROUGH.md - technical explanation and interview preparation
- tests/test_svg_to_gcode.py and tests/test_cnc_sender.py - host-side regression tests

## Setup

1. Install Arduino IDE and the AccelStepper library. Servo is included with the Arduino AVR core.
2. Open the firmware sketch and select Arduino Uno.
3. Install Python 3.10+ and run: python -m pip install -r requirements.txt
4. Convert the example: python tools/svg_to_gcode.py examples/test_square.svg -o square.gcode
5. Send on Windows: python tools/cnc_sender.py --port COM5 --file square.gcode

The sender waits for the ready banner, homes the machine, then sends one command at a time and waits for an acknowledgement.

## Development checks

From this directory, run:

~~~bash
python -m pip install -r requirements.txt
python -m pip install pytest
python -m py_compile tools/cnc_sender.py tools/svg_to_gcode.py
python -m pytest -q
~~~

GitHub Actions also checks the Python tools/tests and compiles the sketch for Arduino Uno. A passing CI build does not replace physical commissioning.

## Documentation

See the [architecture](docs/ARCHITECTURE.md), [BOM](docs/BOM.md), [Windows setup](docs/WINDOWS_SETUP.md), [hardware](docs/HARDWARE.md), [calibration](docs/CALIBRATION.md), [protocol](docs/PROTOCOL.md), [validation](docs/VALIDATION.md), [troubleshooting](docs/TROUBLESHOOTING.md) and [project walkthrough](docs/PROJECT_WALKTHROUGH.md).

## Supported commands

- $H - home X then Y
- $X - clear alarm after inspecting and correcting the cause; position is invalidated
- G0 X... Y... - rapid absolute move
- G1 X... Y... F... - absolute linear move; feed in mm/min
- M3 / M5 - pen down / pen up
- M114 - report position and state
- G21 / G90 - accepted no-ops; units are always mm and coordinates absolute

This is not a full RS-274 interpreter. Arcs, relative positioning, offsets, tool changes, canned cycles and line numbers are unsupported.

## Known limitations and safety

- Firmware compiles for Arduino Uno in GitHub Actions; it has not been tested on the physical machine.
- Homing assumes NC switches to GND: healthy/unpressed reads LOW; open/actuated/broken wire reads HIGH.
- Only negative-end home switches are represented. Positive travel is limited in software, not by independent far-end switches.
- D11 is a software-monitored NC E-stop input. A real E-stop must independently interrupt driver power or driver enable through appropriately rated hardware. Never rely on USB or firmware as the only emergency stop.
- Driver current limits must be set for the actual motor and carrier board. Motor phase current and supply voltage are not yet recorded. Do not guess VREF.
- Motion routines are blocking. PC/USB failure is not a safe stop.
- The two AccelStepper profiles are coordinated approximately, not by a mathematically exact path interpolator. Validate diagonal accuracy before plotting.
- SVG transforms, text, clipping, raster images and advanced SVG features are not supported. Convert artwork to simple paths and flatten transforms first.
- Confirm usable mechanical travel. The homing release offset and 150 mm software workspace must fit within the real travel.

## Verification checklist

- [x] Compile for Arduino Uno in GitHub Actions.
- [ ] Compile locally with Arduino IDE / arduino-cli if using a separate local toolchain.
- [ ] Verify driver current limits, motor supply and common ground.
- [ ] Verify NC switch polarity and travel direction.
- [ ] Verify a hardware E-stop physically removes drive power.
- [ ] Verify homing repeatability.
- [ ] Measure actual travel against commanded travel on each axis.
- [ ] Verify diagonal motion, pen lift and soft limits.
- [ ] Run the square test at low speed on paper.
- [ ] Record actual test results; do not claim unperformed tests.
