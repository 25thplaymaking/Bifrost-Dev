"""Run the isolated regression handler in an already-running Workbench."""

import json
import socket
import struct


def packed(text):
    data = text.encode("utf-8")
    return struct.pack("<i", len(data)) + data


def receive_exact(connection, length):
    result = bytearray()
    while len(result) < length:
        chunk = connection.recv(length - len(result))
        if not chunk:
            raise RuntimeError("Workbench closed the connection before completing its reply")
        result.extend(chunk)
    return result


def receive_string(connection):
    length = struct.unpack("<i", receive_exact(connection, 4))[0]
    if not 0 <= length <= 16 * 1024 * 1024:
        raise RuntimeError("Invalid Workbench reply length")
    return receive_exact(connection, length).decode("utf-8")


def main():
    request = json.dumps({"APIFunc": "DCO_MissionPanelRegression"})
    payload = struct.pack("<i", 1) + packed("BifrostRegression") + packed("JsonRPC") + packed(request)
    with socket.create_connection(("127.0.0.1", 5775), timeout=10) as connection:
        connection.sendall(payload)
        status = receive_string(connection)
        if status != "Ok":
            raise RuntimeError(status)
        result = json.loads(receive_string(connection))
    print(json.dumps(result, indent=2))
    if result.get("passed") != 18 or result.get("failures"):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
