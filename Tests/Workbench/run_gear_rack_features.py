"""Bounded native rack validation; does not use the live server or a user's editor world."""

import hashlib
import json
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "Tests/Workbench"
EXE = Path("P:/SteamLibrary/steamapps/common/Arma Reforger Tools/Workbench/ArmaReforgerWorkbenchSteamDiag.exe")
GAME = Path("P:/SteamLibrary/steamapps/common/Arma Reforger")
LOGS = Path.home() / "Documents/My Games/ArmaReforgerWorkbench/logs"
FLAGS = subprocess.CREATE_NO_WINDOW


def main():
    manifest = ROOT / "addon.gproj"
    assert ROOT == Path("C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev")
    assert 'GUID "6A0C2D6CE9809C6E"' in manifest.read_text()
    active = subprocess.check_output(
        ["powershell.exe", "-NoProfile", "-Command", "Get-CimInstance Win32_Process | Where-Object Name -Like 'ArmaReforgerWorkbench*' | Select-Object -ExpandProperty ProcessId"],
        creationflags=FLAGS, text=True,
    ).strip()
    if active:
        raise RuntimeError("Workbench is already running; close it before this isolated check")
    sources = [TESTS / "DCO_GearRackReplicationRegression.c", TESTS / "DCO_GearRackFeatureRegression.c"]
    replication, features = (source.read_bytes() for source in sources)
    boundary = features.index(b"[WorkbenchPluginAttribute")
    data = replication + features
    staged_files = {
        ROOT / "Scripts/Game/DCO/Arsenal/DCO_GearRackFeatureFixtures.c": features[:boundary],
        ROOT / "Scripts/WorkbenchGame/DCO_GearRackFeaturePlugin.c": replication + b"\n" + features[boundary:],
    }
    for staged in staged_files:
        if staged.exists():
            raise RuntimeError(f"Refusing to replace an existing fixture: {staged}")
    for staged, content in staged_files.items():
        staged.parent.mkdir(parents=True, exist_ok=True)
        with staged.open("xb") as file:
            file.write(content)
    before = set(LOGS.iterdir())
    process = None
    output = ""
    try:
        process = subprocess.Popen(
            [str(EXE), "-gproj", str(manifest), "-wbModule=ResourceManager", "-plugin=DCO_GearRackFeaturePlugin"],
            cwd=GAME, creationflags=FLAGS, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        print(f"Native rack checks started for {manifest}; owned process {process.pid}", flush=True)
        deadline = time.monotonic() + 90
        compile_failed = False
        while process.poll() is None and time.monotonic() < deadline:
            for folder in set(LOGS.iterdir()) - before:
                script = folder / "script.log"
                if script.exists() and 'Can\'t compile' in script.read_text(errors="replace"):
                    compile_failed = True
                    break
            if compile_failed:
                break
            time.sleep(0.5)
        if process.poll() is None:
            subprocess.run(["taskkill.exe", "/PID", str(process.pid), "/T", "/F"], creationflags=FLAGS, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
            process.wait(timeout=10)
        folders = sorted(set(LOGS.iterdir()) - before)
        for folder in folders:
            script = folder / "console.log"
            if script.exists():
                output += script.read_text(errors="replace")
        match = re.search(r"\[RACK-TEST\] RESULT passed=(\d+) failed=(\d+)", output)
        release = (ROOT / "Configs/Release/BifrostRelease.conf").read_text()
        fingerprint = re.search(r"source-sha256=([0-9a-f]{64})", release)
        result = dict(project=str(manifest), testSourceSha256=hashlib.sha256(data).hexdigest(),
                      runtimeSourceSha256=fingerprint.group(1) if fingerprint else None, exitCode=process.returncode,
                      compileFailed=compile_failed, nativeResult=match.group(0) if match else None,
                      logFolders=[str(p) for p in folders])
        (TESTS / "gear-rack-features-result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result, indent=2))
        for line in output.splitlines():
            if "[RACK-TEST]" in line or re.search(r"SCRIPT\s+\(E\)", line):
                print(line)
        if not match or int(match.group(2)) or process.returncode:
            raise SystemExit(1)
    finally:
        if process and process.poll() is None:
            subprocess.run(["taskkill.exe", "/PID", str(process.pid), "/T", "/F"], creationflags=FLAGS, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
            process.wait(timeout=10)
        for staged, content in staged_files.items():
            if staged.read_bytes() != content:
                raise RuntimeError("Staged test changed; retained for inspection")
            staged.unlink()


if __name__ == "__main__":
    main()
