# 🎮 Dual Stepper & Servo Joystick Control System (Arduino)
<img width="959" height="665" alt="Screenshot 2025-10-16 222632" src="https://github.com/user-attachments/assets/606ef31e-df2d-4579-a549-1cd342334e56" />

## 📘 Overview
This project demonstrates how to control **two stepper motors** and a **servo motor** using a **joystick module** with an Arduino.  
It uses the **AccelStepper** and **MultiStepper** libraries to manage stepper motor movement efficiently and smoothly.

The joystick's **X and Y axes** control the two stepper motors, allowing directional motion (forward and reverse).  
The **joystick button** toggles the servo motor between two positions (0° and 90°), simulating a grab/release or engage/disengage function.

This setup can act as a simplified prototype for:
- **CNC axis control**
- **Camera pan-tilt systems**
- **Robotic arms**
- **Plotter/engraving control systems**

---

## ⚙️ Components Used

| Component | Quantity | Description |
|------------|-----------|-------------|
| Arduino UNO / Nano | 1 | Main microcontroller board |
| Stepper Motor (28BYJ-48 or NEMA 17) | 2 | Motors for X and Y axis movement |
| Stepper Driver (ULN2003 or A4988) | 2 | Interface between Arduino and stepper motors |
| Servo Motor (SG90 / MG90S) | 1 | Controlled via joystick button |
| Joystick Module | 1 | Analog X/Y control + push button |
| Breadboard & Jumper Wires | - | For circuit connections |
| External Power Supply (optional) | 1 | Recommended for stepper motors |

---

## 🧩 Libraries Used

### 1. [AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/)
- Enables smooth acceleration/deceleration of steppers.
- Handles speed control and direction management.
- Allows both constant speed and position-based motion.

### 2. [MultiStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/)
- Enables **coordinated motion** of multiple steppers.
- Ensures both motors reach target positions simultaneously.
- (Included here for expansion; not actively used in the current version.)

### 3. [Servo](https://www.arduino.cc/en/reference/servo)
- Standard Arduino library to control servo motors via PWM signals.

---

## 🪜 Circuit Connections

| Arduino Pin | Component | Description |
|--------------|------------|-------------|
| D2 | DIR1 | Direction control for Stepper 1 |
| D3 | STEP1 | Step signal for Stepper 1 |
| D4 | DIR2 | Direction control for Stepper 2 |
| D5 | STEP2 | Step signal for Stepper 2 |
| D6 | Servo signal | PWM signal for servo control |
| A0 | Joystick Vertical (Y-axis) | Analog control for Stepper 2 |
| A1 | Joystick Horizontal (X-axis) | Analog control for Stepper 1 |
| A2 | Joystick Button (Select) | Toggles servo position |

> ⚠️ **Note:** Connect **joystick VCC → 5V** and **GND → GND**.  
> Stepper motor drivers need **separate power** (typically 5–12V depending on motor type).

---

## 💻 Code Summary

### 🔹 Main Features:
1. **Dual stepper motor control**  
   - Uses joystick X/Y to set motor direction.  
   - Constant speed control (forward/reverse).  
   - Dead-zone filtering prevents jitter near center.

2. **Servo toggle via joystick button**  
   - Pressing joystick button toggles servo between 0° and 90°.  
   - Simple software debounce to avoid false toggles.

3. **Scalable for CNC/Plotter**  
   - MultiStepper is included for potential coordinated path execution.

---

## 🧠 How It Works

1. **Analog Joystick Input**
   - X and Y axes give values from `0–1023`, centered around ~512.
   - Center position (≈512) corresponds to no movement.
   - Values above or below a threshold set motor direction.

2. **Stepper Motion Control**
   - `AccelStepper` allows control via:
     ```cpp
     stepper1.setSpeed(±200);
     stepper1.runSpeed();
     ```
   - Positive speed = forward rotation  
     Negative speed = reverse rotation

3. **Servo Toggle**
   - Joystick button (active LOW) toggles between 0° and 90° using:
     ```cpp
     servo1.write(0);
     servo1.write(90);
     ```

