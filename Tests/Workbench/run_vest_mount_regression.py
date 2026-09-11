"""Run native clothing-mount checks with GRS and Minnesinger loaded locally."""
import json
import socket
import struct

from run_mission_panel_regression import packed, receive_string


def main():
    request = json.dumps({"APIFunc": "DCO_VestMountRegression"})
    payload = struct.pack("<i", 1) + packed("BifrostRegression") + packed("JsonRPC") + packed(request)
    with socket.create_connection(("127.0.0.1", 5775), timeout=30) as connection:
        connection.sendall(payload)
        status = receive_string(connection)
        if status != "Ok":
            raise RuntimeError(status)
        result = json.loads(receive_string(connection))
    print(json.dumps(result, indent=2))
    grs_skipped = any(line.startswith("SKIP: GRS") for line in result.get("details", []))
    expected = 13 if grs_skipped else 24
    if result.get("passed") != expected or result.get("failures"):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
