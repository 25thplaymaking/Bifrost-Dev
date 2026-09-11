"""Exercise property-session cleanup through Workbench's local NET API."""
import json
import socket
import struct

from run_mission_panel_regression import packed, receive_string


def main():
    request = json.dumps({"APIFunc": "DCO_SessionLifecycleRegression"})
    payload = struct.pack("<i", 1) + packed("BifrostRegression") + packed("JsonRPC") + packed(request)
    with socket.create_connection(("127.0.0.1", 5775), timeout=15) as connection:
        connection.sendall(payload)
        status = receive_string(connection)
        if status != "Ok":
            raise RuntimeError(status)
        result = json.loads(receive_string(connection))
    print(json.dumps(result, indent=2))
    if result.get("passed") != 34 or result.get("failures") or not result.get("studioLights") or not result.get("audioGraph"):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
