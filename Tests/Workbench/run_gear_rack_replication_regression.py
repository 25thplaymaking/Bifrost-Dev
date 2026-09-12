"""Check resolved native rack inventory settings without touching a play world."""
import json
from pathlib import Path
import socket
import struct

from run_mission_panel_regression import packed, receive_string


def main():
    request = json.dumps({"APIFunc": "DCO_GearRackReplicationRegression"})
    payload = struct.pack("<i", 1) + packed("BifrostRegression") + packed("JsonRPC") + packed(request)
    with socket.create_connection(("127.0.0.1", 5775), timeout=20) as connection:
        connection.sendall(payload)
        status = receive_string(connection)
        if status != "Ok":
            raise RuntimeError(status)
        result = json.loads(receive_string(connection))
    result["projectPath"] = str(Path(__file__).resolve().parents[2])
    result["projectGuid"] = "6A0C2D6CE9809C6E"
    print(json.dumps(result, indent=2))
    if result.get("passed") != 20 or result.get("failures"):
        raise SystemExit(1)
    Path(__file__).with_name("gear_rack_replication_result.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
