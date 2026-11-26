from __future__ import annotations

import pytest

from tools.cnc_sender import send_line


class FakeSerial:
    def __init__(self, responses: list[bytes]) -> None:
        self.responses = list(responses)
        self.writes: list[bytes] = []
        self.flushed = False

    def write(self, data: bytes) -> None:
        self.writes.append(data)

    def flush(self) -> None:
        self.flushed = True

    def readline(self) -> bytes:
        if self.responses:
            return self.responses.pop(0)
        return b""


def test_sender_waits_for_ok_acknowledgement() -> None:
    port = FakeSerial([b"moving X/Y\n", b"ok\n"])
    send_line(port, "G1 X10 Y10 F60", 1.0)

    assert port.writes == [b"G1 X10 Y10 F60\n"]
    assert port.flushed


def test_sender_raises_on_controller_error() -> None:
    port = FakeSerial([b"error: soft travel limit exceeded\n"])
    with pytest.raises(RuntimeError, match="soft travel limit exceeded"):
        send_line(port, "G1 X999 Y999 F60", 1.0)


def test_sender_times_out_without_acknowledgement() -> None:
    port = FakeSerial([])
    with pytest.raises(TimeoutError, match="No 'ok' response"):
        send_line(port, "M5", 0.001)
