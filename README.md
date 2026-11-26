# CNC Catastrophe

<p align="center">
  <strong>A two-axis, Arduino-based CNC pen plotter</strong><br/>
  Turning vector artwork into physical drawings through embedded motion control and a Python toolchain.
</p>

<p align="center">
  <a href="https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/python-checks.yml"><img alt="Python checks" src="https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/python-checks.yml/badge.svg?branch=main"></a>
  <a href="https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/arduino-build.yml"><img alt="Arduino build" src="https://github.com/PaweesRaag/CNC_catastophe/actions/workflows/arduino-build.yml/badge.svg?branch=main"></a>
</p>

> **Project status:** The software implementation, documentation and automated checks are in place. The Arduino Uno firmware compiles in CI and the Python test suite contains ten tests. Physical commissioning, motion calibration, safety verification and real drawing tests are still pending.

---

## Overview

**CNC Catastrophe** is a DIY two-axis pen-plotter project built around an Arduino Uno, two NEMA 17 stepper motors, two DRV8825 stepper drivers, lead-screw motion and a servo-operated pen lift.

The project is designed as an end-to-end workflow rather than an isolated motor-control sketch. Vector artwork is converted into a constrained G-code-like command file, transmitted by a Python serial sender, interpreted by the Arduino firmware, and translated into stepper and servo actions.

The repository preserves the original prototype under `v1/` and contains the more structured implementation under [`cnc-catastrophe-v2/`](cnc-catastrophe-v2/README.md).

### What the system is intended to do

- Convert supported SVG path artwork into absolute-coordinate G-code-style commands.
- Map and scale artwork to fit a configured 150 mm × 150 mm workspace.
- Communicate with an Arduino Uno over USB serial.
- Home the X and Y axes using normally-closed (NC) switches.
- Move two stepper-driven axes through lead-screw mechanisms.
- Raise and lower a pen through a servo-controlled mechanism.
- Enforce configured software travel limits and report controller state.
- Provide a documented workflow for setup, calibration, validation and troubleshooting.

## System workflow

```text
       Vector artwork
          SVG file
             |
             v
     Python SVG converter
   (path sampling / scaling)
             |
             v
       G-code file
             |
             v
     Python serial sender
   (stop-and-wait protocol)
             |
          USB serial
             |
             v
        Arduino Uno
     (command interpreter)
          /       \
         /         \
        v           v
   DRV8825 x2    Servo output
        |           |
        v           v
  NEMA 17 x2     Pen lift
        |
        v
   X/Y lead screws
        |
        v
   Two-axis plotter
```

The serial sender uses a stop-and-wait approach: it sends a command and waits for the controller's acknowledgement before sending the next one. This keeps the host and firmware command sequence synchronized; it is not a substitute for a safety-rated motion controller or hardware emergency stop.

## Hardware configuration

The following table records the current project configuration. Values marked as configured or calculated are software assumptions until confirmed against the assembled machine.

| Component / parameter | Project configuration |
|---|---|
| Main controller | Arduino Uno |
| Motion axes | X and Y |
| Stepper motors | 2 × NEMA 17 |
| Stepper drivers | 2 × DRV8825 |
| Motion transmission | Lead screws |
| Lead screw lead | 8 mm/revolution |
| Motor full steps | 200 steps/revolution |
| Microstepping setting | 1/16 |
| Calculated command scale | 400 steps/mm |
| Configured workspace | 150 mm × 150 mm |
| Homing inputs | 2 × normally-closed switches |
| Pen mechanism | Servo-operated pen lift |

### Steps-per-millimetre calculation

The configured step scale is derived from the motor step angle, microstepping setting and lead-screw lead:

```text
steps/mm = (full steps/revolution × microsteps) / lead (mm/revolution)

         = (200 × 16) / 8

         = 400 steps/mm
```

This value assumes both axes use the stated motor, driver microstep configuration and lead-screw lead. It is a configuration calculation—not a measured accuracy result. Actual travel must be calibrated with the assembled mechanism.

### Pin assignment

The current firmware pin map is:

| Function | Arduino Uno pin |
|---|---:|
| X direction | D2 |
| X step | D3 |
| Y direction | D4 |
| Y step | D5 |
| Pen servo signal | D6 |
| Shared driver enable | D8 |
| X home switch | D9 |
| Y home switch | D10 |
| E-stop monitor input | D11 |

See [Hardware and safety](cnc-catastrophe-v2/docs/HARDWARE.md) before connecting or powering the drivers. Do not infer driver current settings or power-supply ratings from this table; those depend on the exact motor and DRV8825 carrier-board hardware.

## Software and firmware

### Arduino motion-control firmware

The firmware in `cnc-catastrophe-v2/firmware/CNC_Catastrophe/` is responsible for:

- Parsing the supported serial command set.
- Controlling step and direction outputs through the AccelStepper library.
- Operating the pen servo through the Servo library.
- Performing sequential X-then-Y homing with fast seek, backoff and slow re-approach.
- Tracking the homed state and enforcing configured software travel bounds.
- Monitoring the firmware E-stop input and handling alarm state.
- Returning command acknowledgements, errors and position/state reports.
- Waiting for a configurable servo settling interval before acknowledging pen commands.

Homing assumes normally-closed switches wired so that the healthy, unactuated circuit reads LOW and an actuated switch or open wire reads HIGH. Verify this electrical behaviour on the actual wiring before enabling motor movement.

### Python SVG-to-G-code converter

`tools/svg_to_gcode.py` converts supported SVG path geometry into the command format understood by the firmware.

Its intended responsibilities include:

- Reading SVG path geometry.
- Sampling curves into line segments at a configured sampling distance.
- Mapping artwork into the configured work area.
- Scaling oversized artwork to fit the available workspace.
- Producing pen-up travel moves and pen-down drawing moves.
- Rejecting unsupported or invalid input conditions, including drawings with no non-zero-length path geometry.
- Restricting feed values to the firmware's supported 10–300 mm/min range.

This is deliberately a limited SVG converter, not a complete graphics or CAM engine. See [the v2 README](cnc-catastrophe-v2/README.md) for supported input details and limitations.

### Python serial sender

`tools/cnc_sender.py` sends generated command files to the controller over a serial connection.

The sender:

1. Opens the selected serial port.
2. Waits for the controller's startup/ready message.
3. Homes the machine by default.
4. Sends one command at a time.
5. Waits for an `ok` acknowledgement or reports a controller error/timeout.

The automatic homing behaviour is intentional: a drawing should not start from an assumed position. The `--no-home` option exists for controlled development scenarios and should not be used as a shortcut around a valid machine reference.

## Supported controller commands

| Command | Purpose |
|---|---|
| `$H` | Home X, then Y |
| `$X` | Clear alarm after inspecting and correcting its cause; position is invalidated |
| `G0 X... Y...` | Rapid absolute move |
| `G1 X... Y... F...` | Absolute move at a feed rate in mm/min |
| `M3` | Lower pen |
| `M5` | Raise pen |
| `M114` | Report position and controller state |
| `G21` | Accepted no-op; units are always millimetres |
| `G90` | Accepted no-op; coordinates are always absolute |

Only the documented command subset is implemented. The firmware is **not** a full RS-274/G-code interpreter. Arcs, relative positioning, coordinate offsets, tool changes, canned cycles and line numbers are not supported.

## Repository structure

```text
.
├── .github/
│   └── workflows/
│       ├── python-checks.yml
│       └── arduino-build.yml
├── cnc-catastrophe-v2/
│   ├── firmware/
│   │   └── CNC_Catastrophe/
│   │       └── CNC_Catastrophe.ino
│   ├── tools/
│   │   ├── cnc_sender.py
│   │   └── svg_to_gcode.py
│   ├── tests/
│   │   ├── test_cnc_sender.py
│   │   └── test_svg_to_gcode.py
│   ├── examples/
│   │   └── test_square.svg
│   ├── docs/
│   │   ├── ARCHITECTURE.md
│   │   ├── BOM.md
│   │   ├── CALIBRATION.md
│   │   ├── HARDWARE.md
│   │   ├── PROJECT_WALKTHROUGH.md
│   │   ├── PROTOCOL.md
│   │   ├── TROUBLESHOOTING.md
│   │   ├── VALIDATION.md
│   │   └── WINDOWS_SETUP.md
│   ├── requirements.txt
│   └── README.md
├── v1/
├── .gitignore
├── LICENSE
└── README.md
```

The `v1/` directory is retained as the original prototype. New development and the documented workflow are centred on `cnc-catastrophe-v2/`.

## Getting started

### 1. Clone the repository

```bash
git clone https://github.com/PaweesRaag/CNC_catastophe.git
cd CNC_catastophe
```

### 2. Prepare the Arduino environment

1. Install the Arduino IDE or Arduino CLI.
2. Install the **AccelStepper** library.
3. Ensure the Arduino AVR core is installed for the Uno.
4. Open `cnc-catastrophe-v2/firmware/CNC_Catastrophe/CNC_Catastrophe.ino`.
5. Select **Arduino Uno** as the target board.

The Servo library is provided by the Arduino AVR core. Compile first; do not connect motor power or run motion commands until the wiring and safety checks in the hardware guide have been completed.

### 3. Prepare the Python environment

Python 3.10 or later is recommended.

```bash
cd cnc-catastrophe-v2
python -m venv .venv
```

Activate the environment, then install the project dependencies:

**Windows PowerShell**
```powershell
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

**Linux / macOS**
```bash
source .venv/bin/activate
python -m pip install -r requirements.txt
```

### 4. Convert the example SVG

From the `cnc-catastrophe-v2/` directory:

```bash
python tools/svg_to_gcode.py examples/test_square.svg -o square.gcode
```

Inspect the generated command file before sending it to hardware. Keep initial test artwork small and within the verified mechanical travel.

### 5. Send a job

For example, on Windows:

```powershell
python tools/cnc_sender.py --port COM5 --file square.gcode
```

Replace `COM5` with the port assigned to your Arduino. The sender waits for the ready banner and homes by default before transmitting the job. The machine must be wired, current-limited, homed and safety-checked before running a drawing.

For the full setup and operation procedure, see [Windows setup](cnc-catastrophe-v2/docs/WINDOWS_SETUP.md).

## Development and automated checks

The repository includes separate GitHub Actions workflows for Python checks and Arduino Uno firmware compilation.

Run the host-side checks locally from `cnc-catastrophe-v2/`:

```bash
python -m pip install -r requirements.txt
python -m pip install pytest
python -m py_compile tools/cnc_sender.py tools/svg_to_gcode.py
python -m pytest -q
```

The Python suite currently contains ten tests covering the sender and SVG converter. CI also compiles the firmware for the Arduino Uno target.

A passing automated build confirms that the software passes those checks and that the sketch compiles for its configured board. It does **not** prove that the assembled machine moves correctly, is electrically safe, achieves a particular positioning accuracy or can complete a drawing.

## Safety and known limitations

This project controls physical moving hardware. Review the detailed [hardware guide](cnc-catastrophe-v2/docs/HARDWARE.md), [calibration guide](cnc-catastrophe-v2/docs/CALIBRATION.md) and [validation plan](cnc-catastrophe-v2/docs/VALIDATION.md) before commissioning.

Important limitations to understand:

- **Physical E-stop:** D11 is only a firmware-monitored input. A real emergency stop must independently interrupt driver power or driver enable through appropriately rated hardware. Never rely on firmware, USB or the host computer as the sole emergency-stop mechanism.
- **Driver current:** Set each DRV8825 current limit for the actual motor and the exact carrier board. The motor phase-current rating, carrier-board sense resistor and supply voltage must be identified first. Do not guess VREF.
- **Travel limits:** The firmware represents negative-end home switches and uses software limits for positive travel. There are no independent positive-end switches in the current configuration.
- **Homing and origin:** Homing uses a release offset; the released position becomes the software origin. Confirm the actual usable mechanical stroke before relying on the configured 150 mm × 150 mm workspace.
- **Motion coordination:** The two AccelStepper profiles are coordinated approximately. This implementation is not a mathematically exact linear interpolator; diagonal accuracy must be measured.
- **Blocking routines:** Motion operations are blocking. A host disconnect or USB failure is not a safe emergency stop.
- **SVG support:** Complex SVG features such as text objects, transforms, clipping and raster images are outside the supported converter subset. Convert artwork into simple paths and flatten transforms before conversion.
- **Servo setup:** Pen-up and pen-down angles and the settling delay may need adjustment for the actual servo, linkage and pen mount.

Do not energize the motion system until the actual hardware, wiring, current settings, switch polarity and independent E-stop have been checked.

## Physical commissioning checklist

The following list distinguishes automated software verification from tests that still need to be performed on the assembled machine.

### Automated / software verification

- [x] Arduino Uno firmware compiles in GitHub Actions.
- [x] Python CI workflow runs the ten-test suite.
- [x] SVG converter validates feed limits and rejects empty drawable geometry.
- [x] Documentation covers setup, hardware, protocol, calibration, validation and troubleshooting.

### Hardware validation — pending

- [ ] Confirm motor and DRV8825 carrier-board specifications.
- [ ] Verify supply voltage, wiring, common ground and current limits.
- [ ] Verify NC home-switch polarity and open-wire behaviour.
- [ ] Verify independent hardware E-stop operation with drive power present.
- [ ] Test servo endpoints and pen clearance.
- [ ] Test each motor direction at low speed with the mechanism safe.
- [ ] Confirm repeatable X and Y homing.
- [ ] Measure actual travel and calibrate steps/mm for each axis.
- [ ] Verify software limits and safe usable workspace.
- [ ] Test diagonal movement and pen-up/pen-down sequencing.
- [ ] Run a small square, then additional shapes, on paper.
- [ ] Record measured results and capture actual machine photographs/video.

## Documentation

The detailed guides live in [`cnc-catastrophe-v2/docs/`](cnc-catastrophe-v2/docs/):

| Guide | What it covers |
|---|---|
| [Architecture](cnc-catastrophe-v2/docs/ARCHITECTURE.md) | Components, responsibilities and data flow |
| [Hardware](cnc-catastrophe-v2/docs/HARDWARE.md) | Pin map, wiring assumptions and safety |
| [Bill of materials](cnc-catastrophe-v2/docs/BOM.md) | Parts list and hardware configuration record |
| [Windows setup](cnc-catastrophe-v2/docs/WINDOWS_SETUP.md) | Installation and operating workflow |
| [Calibration](cnc-catastrophe-v2/docs/CALIBRATION.md) | Homing, travel measurement and calibration |
| [Serial protocol](cnc-catastrophe-v2/docs/PROTOCOL.md) | Supported commands and acknowledgement behaviour |
| [Validation plan](cnc-catastrophe-v2/docs/VALIDATION.md) | Software, firmware and physical test plan |
| [Troubleshooting](cnc-catastrophe-v2/docs/TROUBLESHOOTING.md) | Fault diagnosis and recovery |
| [Project walkthrough](cnc-catastrophe-v2/docs/PROJECT_WALKTHROUGH.md) | Design rationale and technical explanation |

## Project goals and future work

The current implementation establishes the main software path from SVG artwork to controller commands. The next major milestone is not adding more features; it is commissioning the real mechanism and recording evidence.

Once the machine is physically validated, useful follow-on improvements could include more precise coordinated XY interpolation, stronger travel protection, improved position feedback, richer SVG support and repeatable plot-quality measurements.

## License

See [LICENSE](LICENSE) for the terms governing this repository.
