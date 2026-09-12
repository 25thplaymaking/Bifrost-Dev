"""Check native stand placement without opening or editing the user's world."""
import json
import socket
import struct
from pathlib import Path
from run_mission_panel_regression import packed, receive_string

request = json.dumps({'APIFunc': 'BIA_StandPlacementRegression'})
payload = struct.pack('<i', 1) + packed('BifrostRegression') + packed('JsonRPC') + packed(request)
with socket.create_connection(('127.0.0.1', 5775), timeout=30) as connection:
    connection.sendall(payload)
    status = receive_string(connection)
    if status != 'Ok':
        raise RuntimeError(status)
    result = json.loads(receive_string(connection))
print(json.dumps(result, indent=2))
Path(__file__).with_name('stand_placement_result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
if result.get('passed', 0) < 120 or result.get('failures'):
    raise SystemExit(1)
