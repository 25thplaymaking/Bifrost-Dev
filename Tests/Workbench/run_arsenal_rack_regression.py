"""Native isolated rack, nested inventory and UI checks; not multiplayer acceptance."""
import json
import socket
import struct
from run_mission_panel_regression import packed, receive_string

import time

def request_result(poll):
    request = json.dumps({"APIFunc": "BIA_ArsenalRackRegression", "poll": poll})
    payload = struct.pack("<i", 1) + packed("BifrostRegression") + packed("JsonRPC") + packed(request)
    with socket.create_connection(("127.0.0.1", 5775), timeout=30) as connection:
        connection.sendall(payload)
        status = receive_string(connection)
        if status != "Ok":
            raise RuntimeError(status)
        return json.loads(receive_string(connection))

result = request_result(False)
deadline = time.monotonic() + 90
while result.get("running"):
    if time.monotonic() >= deadline:
        raise TimeoutError("Native rack fixture sweep did not finish within 90 seconds")
    time.sleep(0.25)
    result = request_result(True)
print(json.dumps(result, indent=2))
if (result.get("passed", 0) < 50 or result.get("failures")
        or "XL belt fixture sweep completed" not in result.get("details", [])):
    raise SystemExit(1)
