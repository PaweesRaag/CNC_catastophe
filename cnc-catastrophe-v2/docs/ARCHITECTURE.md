# System architecture

## Overview
CNC Catastrophe is a two-axis Cartesian pen plotter. Desktop software converts vector artwork into absolute millimetre coordinates, streams commands to an Arduino Uno, and the controller drives two stepper axes and a pen-lift servo.

~~~text
SVG artwork
    ↓
SVG-to-G-code converter
    ↓
G-code file
    ↓
Python serial sender ── USB serial, 115200 baud ── Arduino Uno
                                                     ├─ STEP/DIR → DRV8825 → X/Y NEMA 17
                                                     ├─ Servo signal → pen lift
                                                     └─ NC switches / E-stop monitor
~~~

## Hardware layer

| Component | Responsibility |
|---|---|
| Arduino Uno | Parses commands and coordinates motion |
| X/Y NEMA 17 motors | Produce axis motion |
| DRV8825 drivers | Translate STEP/DIR pulses into motor phase current |
| Lead screws | Convert motor rotation into linear travel |
| NC home switches | Establish negative-end reference positions |
| Pen servo | Raises and lowers the pen |
| Hardware E-stop | Independently removes drive power or disables drivers |

## Steps-per-millimetre calculation

steps/mm = (full_steps/rev × microsteps) / lead_mm_per_rev

(200 × 16) / 8 = **400 steps/mm**.

Recalculate if motor resolution, microstep mode or screw lead changes.

## Firmware responsibilities
- Parse a deliberately small G-code-like command subset.
- Maintain homed and alarm state.
- Home X and Y sequentially using NC switches.
- Enforce configured software travel limits.
- Generate STEP/DIR motion through AccelStepper.
- Control pen-up and pen-down servo positions.
- Return acknowledgements, status and errors.

This is not a full G-code interpreter. Arcs, relative coordinates, offsets, tool changes and look-ahead planning are not implemented.

## Host software
The SVG converter samples supported path geometry into line segments, maps artwork into the configured workspace and emits absolute commands. Convert text and primitive shapes to paths and flatten transforms in a vector editor first.

The Python sender waits for the firmware ready banner, optionally homes, then sends one command at a time and waits for an acknowledgement. This stop-and-wait protocol avoids overflowing the Uno's small serial input buffer.

## Motion model
Each axis uses an AccelStepper acceleration profile with velocity and acceleration limits scaled according to its share of the requested XY displacement. This is a lightweight coordinated-motion approximation, not an industrial CNC planner. Diagonal accuracy and corner behaviour require physical measurement.

## Safety model and limitations
- NC switch convention: healthy/closed = LOW; actuated/open wire = HIGH with INPUT_PULLUP.
- Only negative-end home switches are represented.
- Positive travel relies on firmware soft limits.
- The D11 E-stop input is only a firmware monitor. A hardware latching circuit must independently interrupt driver power or disable the drivers.
- USB disconnection and firmware lockups are not safe-stop mechanisms.
- Motion routines are blocking.
- Workspace must fit the real mechanical stroke after the homing release offset.

## Job lifecycle
1. Inspect the machine and clear the travel area.
2. Verify switches and hardware E-stop.
3. Home the machine.
4. Convert a supported SVG to G-code.
5. Inspect dimensions, feed and generated coordinates.
6. Perform a pen-up dry run.
7. Plot at conservative feed.
8. Inspect and record measured results.
