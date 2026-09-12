"""Validate staged release sources and fingerprint their canonical Git bytes."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = "Configs/Release/BifrostRelease.conf"
GUID = "6A0C2D6CE9809C6E"
RUNTIME = ("Assets/", "Configs/", "Prefabs/", "Scripts/Game/", "UI/", "Sounds/")
RESOURCES = {".et", ".layout", ".xob", ".emat", ".edds", ".ptc", ".acp", ".wav"}
TEXT = {".c", ".conf", ".et", ".layout", ".meta", ".emat", ".ptc", ".acp"}


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True,
                          creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).stdout


def staged_sources():
    entries = {}
    for row in git("ls-files", "--stage", "-z").split(b"\0"):
        if not row:
            continue
        info, path = row.split(b"\t", 1)
        mode, oid, stage = info.split()
        if stage != b"0":
            raise RuntimeError("Unmerged index entry")
        name = path.decode("utf-8")
        if name.startswith(RUNTIME):
            entries[name] = oid
    names = sorted(entries)
    if not names:
        return {}
    result = subprocess.run(["git", "cat-file", "--batch"], cwd=ROOT,
                            input=b"\n".join(entries[n] for n in names) + b"\n",
                            capture_output=True, check=True,
                            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    data = result.stdout
    blobs = {}
    offset = 0
    for name in names:
        end = data.index(b"\n", offset)
        size = int(data[offset:end].split()[-1])
        blobs[name] = data[end + 1:end + 1 + size]
        offset = end + size + 2
    return blobs


def verify(blobs):
    errors = []
    metadata = {}
    classes = set()
    for name, data in blobs.items():
        if not data:
            errors.append("Empty runtime file: " + name)
        if "/EnfusionMCP/" in name or re.search(r"(?:Probe|Regression)\.c$", name):
            errors.append("Development handler in runtime source: " + name)
        suffix = Path(name).suffix.lower()
        if suffix in RESOURCES and name + ".meta" not in blobs:
            errors.append("Missing metadata: " + name)
        if suffix not in TEXT:
            continue
        text = data.decode("utf-8-sig")
        if re.search(r"^(?:<{7}|={7}|>{7})(?: |$)", text, re.M):
            errors.append("Merge debris: " + name)
        if suffix == ".meta":
            match = re.search(r'Name\s+"\{([A-Fa-f0-9]{16})\}', text)
            if match:
                key = match[1].upper()
                if key in metadata:
                    errors.append("Duplicate GUID: " + name + " / " + metadata[key])
                metadata[key] = name
        if suffix == ".c":
            classes.update(re.findall(r"\bclass\s+(\w+)", text))
            if re.search(r"^\s*(?:(?:protected|private|public)\s+)?static\s+(?!const\b).*?=\s*(?:new\b|\{|\w+\s*\()", text, re.M):
                errors.append("Eager nonconstant static initializer: " + name)
    attributes = set()
    for name, data in blobs.items():
        if name.startswith("Configs/") and name.endswith(".conf"):
            attributes.update(re.findall(r"\bDCO_\w*Attribute\w*\b", data.decode("utf-8-sig")))
    errors.extend("Unresolved editor attribute: " + name for name in sorted(attributes - classes))
    digest = hashlib.sha256()
    for name in sorted(blobs):
        if name == MANIFEST:
            continue
        data = blobs[name]
        digest.update(name.encode("utf-8") + b"\0")
        digest.update(len(data).to_bytes(8, "little"))
        digest.update(data)
    return errors, digest.hexdigest(), len(metadata), len(attributes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--write-manifest", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
        parser.error("Version must have three numeric components")
    project = (ROOT / "addon.gproj").read_text(encoding="utf-8-sig")
    if f'GUID "{GUID}"' not in project or 'ID "BifrostDev"' not in project:
        raise SystemExit("Authoritative project identity mismatch")
    blobs = staged_sources()
    if not blobs:
        raise SystemExit("No staged release sources")
    errors, fingerprint, metadata, attributes = verify(blobs)
    manifest = ('SCR_UIInfo {\n Name "Bifrost Release ' + args.version + '"\n'
                ' Description "source-sha256=' + fingerprint + '; workshop-guid=' + GUID + '"\n}\n')
    if args.write_manifest and not errors:
        (ROOT / MANIFEST).write_text(manifest, encoding="utf-8")
    elif blobs.get(MANIFEST, b"").decode("utf-8-sig").replace("\r\n", "\n") != manifest:
        errors.append("Staged release manifest/version/fingerprint mismatch")
    for error in errors:
        print(error)
    print(f"{ROOT}\n{len(blobs)} runtime files; {metadata} metadata GUIDs; {attributes} attributes; {len(errors)} failures")
    print("Source SHA-256:", fingerprint)
    raise SystemExit(bool(errors))


if __name__ == "__main__":
    main()
