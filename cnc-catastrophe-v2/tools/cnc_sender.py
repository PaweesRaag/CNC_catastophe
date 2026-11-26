#!/usr/bin/env python3
"""Stop-and-wait serial sender for CNC Catastrophe firmware."""
from __future__ import annotations
import argparse
import time
from pathlib import Path

import serial
from serial.tools import list_ports

READY = "CNC_CATASTROPHE_READY"


def wait_for_ready(port: serial.Serial, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = port.readline().decode("utf-8", errors="replace").strip()
        if line:
            print(f"<< {line}")
        if READY in line:
            return
    raise TimeoutError("Arduino did not announce readiness; check port, baud and firmware")


def send_line(port: serial.Serial, command: str, timeout: float) -> None:
    command = command.strip()
    if not command or command.startswith(";"):
        return
    port.write((command + "\n").encode("ascii"))
    port.flush()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        response = port.readline().decode("utf-8", errors="replace").strip()
        if not response:
            continue
        print(f"<< {response}")
        if response.lower().startswith("error:"):
            raise RuntimeError(f"Controller rejected {command!r}: {response}")
        if response.lower() == "ok":
            return
    raise TimeoutError(f"No 'ok' response for command: {command}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Arduino port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--file", required=True, type=Path, help="G-code file")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--no-home", action="store_true", help="Skip automatic homing (not recommended)")
    parser.add_argument("--command-timeout", type=float, default=1800.0)
    args = parser.parse_args()
    if not args.file.is_file():
        parser.error(f"G-code file not found: {args.file}")

    print("Available serial ports:")
    for item in list_ports.comports():
        print(f"  {item.device}: {item.description}")

    with serial.Serial(args.port, args.baud, timeout=0.25, write_timeout=2) as port:
        # Opening an Uno serial port normally resets it.
        time.sleep(2.5)
        port.reset_input_buffer()
        wait_for_ready(port, 10)
        if not args.no_home:
            print(">> $H")
            send_line(port, "$H", max(args.command_timeout, 210))
        try:
            for number, raw in enumerate(args.file.read_text(encoding="utf-8").splitlines(), 1):
                command = raw.split(";", 1)[0].strip()
                if not command:
                    continue
                print(f">> {command}")
                send_line(port, command, args.command_timeout)
        except Exception:
            # Best-effort pen-up command; the physical E-stop remains the real stop mechanism.
            try:
                send_line(port, "M5", 5)
            except Exception:
                pass
            raise
    print("Job sent. Confirm the drawing physically before relying on calibration.")


if __name__ == "__main__":
    main()
