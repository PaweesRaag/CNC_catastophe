# Windows setup guide

## 1. Install prerequisites
- Install Arduino IDE 2.x.
- Install Python 3.10 or newer.
- Connect the Arduino Uno by USB.
- In Arduino IDE Library Manager, install AccelStepper and Servo if they are not already available.

## 2. Prepare the Python environment

Open PowerShell in the cnc-catastrophe-v2 folder:

~~~powershell
py --version
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
~~~

If PowerShell blocks virtual-environment activation, use the Python executable directly at .venv\Scripts\python.exe or adjust the local execution policy according to your organisation's rules.

## 3. Compile and upload firmware
1. Open firmware/CNC_Catastrophe/CNC_Catastrophe.ino in Arduino IDE.
2. Select the Arduino Uno board.
3. Select the COM port shown for the connected board.
4. Compile, then upload.
5. Open Serial Monitor at 115200 baud and confirm CNC_CATASTROPHE_READY.
6. Close Serial Monitor before running the Python sender because the COM port can only be opened by one application at a time.

GitHub Actions also compiles the sketch for Arduino Uno, but local upload and hardware operation are separate checks.

## 4. Convert the example SVG

~~~powershell
python tools/svg_to_gcode.py examples/test_square.svg -o square.gcode
~~~

Open square.gcode in a text editor and inspect the dimensions, coordinates, feed and M3/M5 pen commands before sending it.

## 5. Identify the COM port

Use Device Manager → Ports (COM & LPT), or inspect the port list printed by the sender. Replace COM5 below with the actual port:

~~~powershell
python tools/cnc_sender.py --port COM5 --file square.gcode
~~~

The sender waits for the controller ready banner, homes by default and sends commands one at a time.

## 6. First run precautions
- Keep the pen raised or removed for the first dry run.
- Keep the mechanical area clear.
- Verify the physical E-stop before motion.
- Start at conservative feed and reduced travel.
- Never rely on closing the sender window as an emergency stop.
