# Hardware and wiring

## Pin assignment

| Arduino Uno | Signal | Connection |
|---|---|---|
| D2 | X DIR | X DRV8825 DIR |
| D3 | X STEP | X DRV8825 STEP |
| D4 | Y DIR | Y DRV8825 DIR |
| D5 | Y STEP | Y DRV8825 STEP |
| D6 | Pen servo | Servo signal |
| D8 | Driver enable | Both DRV8825 ENABLE pins; active LOW |
| D9 | X home | Normally-closed switch input |
| D10 | Y home | Normally-closed switch input |
| D11 | E-stop monitor | Normally-closed E-stop loop |
| GND | Logic ground | Common logic ground |

## Switch convention

Firmware uses INPUT_PULLUP and assumes the NC contact connects its input to GND when healthy and unpressed. Input reads LOW normally and HIGH when actuated or when the circuit/wire opens. Confirm COM/NC terminals with a meter before wiring.

## DRV8825 requirements

- VMOT and motor GND connect to a suitable external motor supply, never the Arduino 5 V pin.
- Connect VDD and logic GND according to the carrier board documentation.
- Configure M0/M1/M2 for 1/16 microstepping using the exact carrier's truth table.
- Fit the carrier manufacturer's recommended bulk capacitor close to VMOT/GND with correct polarity and voltage rating.
- Set current limit using the carrier's documented VREF/current-sense relationship and the motor's rated phase current. Do not guess.
- Never connect or disconnect a stepper motor while its driver is powered.

## Emergency stop

D11 is only a logic input. A proper emergency stop must use a suitably rated, latching hardware circuit that removes motor-driver power and/or disables the drivers independently of the Arduino. Do not route motor current through a small signal switch. Keep the E-stop reachable and test it before motion.

## Before first powered movement

1. Confirm motor phase pairs and driver wiring.
2. Set each driver's current limit to the motor specification.
3. Verify supply voltage and polarity.
4. Verify common logic ground.
5. Verify both home inputs read LOW normally and HIGH when actuated/open-circuit.
6. Test one axis at a time at low speed with the mechanism clear.
7. Confirm homing direction. If an axis moves away from its home switch, power down and correct wiring/direction configuration; never defeat a switch.
8. Confirm the hardware E-stop physically removes drive power.
