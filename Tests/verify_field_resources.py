"""Validate shipped field-test resources; this does not exercise the game engine."""
from pathlib import Path
import hashlib
import json
import re
import wave
import argparse

ROOT = Path(__file__).resolve().parents[1]
SOUNDS = ROOT / "Sounds/Bifrost"
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+')


def balanced(text):
    depth = 0
    for token in TOKEN.findall(text):
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1
            assert depth >= 0, "Unexpected closing brace"
    assert depth == 0, "Unclosed resource block"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--final-mix", type=Path, help="Extracted current-game Sounds/FinalMix.afm")
    args = parser.parse_args()
    entries = json.loads((SOUNDS / "sources.json").read_text())
    assert {e["name"] for e in entries} == {"crowd", "talking", "barking", "shouting", "battle", "rifle"}
    for entry in entries:
        file = SOUNDS / (entry["name"] + ".wav")
        assert entry["licence"] == "CC0-1.0"
        assert hashlib.sha256(file.read_bytes()).hexdigest() == entry["output_sha256"]
        with wave.open(str(file)) as audio:
            assert (audio.getnchannels(), audio.getframerate(), audio.getsampwidth()) == (1, 48000, 2)
            assert abs(audio.getnframes() / 48000 - entry["duration"]) < 0.001
            frames = audio.readframes(audio.getnframes())
            assert len(frames) == audio.getnframes() * 2 and any(frames)
        assert entry["guid"] in file.with_suffix(".wav.meta").read_text()

    graph = (SOUNDS / "Ambience.acp").read_text()
    balanced(graph)
    balanced((SOUNDS / "Controls.sig").read_text())
    for entry in entries:
        assert graph.count('name "Bifrost' + entry["name"].capitalize() + '"') == 1
        assert graph.count("{" + entry["guid"] + "}") == 1
    assert len(re.findall(r'\bSoundClass\s*{', graph)) == 6
    assert len(re.findall(r'\bBankLocalClass\s*{', graph)) == 6
    assert graph.count('"Infinite loop" 0') == 6, "Script controls looping; banks must finish naturally"
    assert graph.count('"Loop count" 1') == 6, "Each native event must play one recording"
    assert re.search(r'\bfrequency\s*{', graph) and re.search(r'\bauxOuts\s*{', graph)
    if args.final_mix:
        mixer = args.final_mix.read_text()
        buses = set(re.findall(r'OBusClass[^{}]*\{\s*id\s+(\d+)', mixer))
        outputs = set(re.findall(r'\b(?:OSPort|outStatePort)\s+(\d+)', graph))
        assert outputs <= buses, f"Audio outputs missing from current game mixer: {outputs - buses}"
    signal_links = re.findall(r'IOPConnectionClass[^{}]*\{\s*port\s+\d+\s+conn\s*\{\s*ConnectionsClass[^{}]*\{\s*id\s+(\d+)', graph)
    assert len(signal_links) == 4 and set(signal_links) == {"9"}, "Reverb inputs must use native signal connections, not constant connections"
    for file in SOUNDS.glob("*.meta"):
        text = file.read_text()
        balanced(text)
        expected = "WAVResourceClass" if ".wav." in file.name else "AudioProjectResourceClass" if ".acp." in file.name else "AudioSignalResourceClass"
        assert expected + " PC" in text

    attrs = (ROOT / "Configs/Editor/AttributeLists/DCO_Attributes.conf").read_text()
    audio_types = re.findall(r'\b(DCO_Audio\w+EditorAttribute)\s+"', attrs)
    assert len(audio_types) == len(set(audio_types)) == 9, "Entity attributes need unique native types"
    for file in (ROOT / "Prefabs/E_DCO_Teleporter.et", ROOT / "UI/layouts/DCO_GMHint.layout"):
        balanced(file.read_text())
    print("PASS: 6 audio files and source hashes, audio graphs, platform metadata, 9 distinct settings, teleporter and hint syntax")


if __name__ == "__main__":
    main()
