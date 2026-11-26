# Validation and test plan

The tests below define what must be checked before calling the machine physically validated. They are not claimed as completed.

## Host-side checks
- Python syntax check for both tools.
- Convert the supplied square and verify G21/G90, pen-up travel, pen-down paths and final pen-up.
- Reject empty SVGs and invalid viewBoxes.
- Reject non-positive workspace, sample distance and feed values.
- Confirm oversized artwork is scaled to fit.
- Inspect generated coordinates against configured soft limits.

Run from the v2 directory:

~~~bash
python -m pip install -r requirements.txt
python -m pip install pytest
python -m py_compile tools/cnc_sender.py tools/svg_to_gcode.py
python -m pytest -q
~~~

## Firmware build
- Select Arduino Uno / AVR target.
- Install AccelStepper.
- Compile the firmware sketch.
- Record Arduino IDE/CLI, AVR core and library versions.
- Resolve compiler warnings that indicate unsafe conversions or undefined behaviour.

## Controller and serial checks
- Confirm startup banner at 115200 baud.
- Confirm supported commands return ok and unsupported commands return errors.
- Confirm sender waits for each acknowledgement.
- Confirm sender detects controller errors and timeouts.

## Hardware checks
- Verify NC switch polarity and open-wire detection.
- Verify axis direction at low speed.
- Verify homing seek, release and slow re-approach.
- Verify soft-limit rejection.
- Verify hardware E-stop independently removes drive power.
- Verify pen servo endpoints, homing repeatability and usable travel.

## Drawing checks
- Dry-run the square with pen lifted.
- Plot the square at low feed and measure width, height and corner alignment.
- Test a diagonal pattern.
- Test multiple separated paths to confirm pen-up travel.
- Test a more complex supported SVG.

## Release acceptance criteria
A physically validated release requires a successful firmware build, passing host-side tests, completed safety checks, calibrated homing/travel, and recorded square/diagonal drawing results. If hardware tests have not been performed, describe this as an implemented prototype with a commissioning plan, not as a physically validated machine.
