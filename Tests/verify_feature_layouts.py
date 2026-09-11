"""Check literal bindings and generated mission/composition controls against shipped layouts.
This is source integrity evidence, not an engine interaction test.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+')


def widgets(text):
    tokens = TOKEN.findall(text)
    stack, result = [], {}
    for i, token in enumerate(tokens):
        if token == "{":
            kind = tokens[i - 1] if i else ""
            if kind.startswith('"') and i > 1:
                kind = tokens[i - 2]
            if kind == ":" and i > 3:
                kind = tokens[i - 4]
            stack.append(kind)
        elif token == "}":
            stack.pop()
        elif token == "Name" and stack and stack[-1].endswith("WidgetClass"):
            name = tokens[i + 1].strip('"')
            result[name] = stack[-1].removesuffix("Class")
    return result


def review(layout_texts):
    errors, checks = [], 0
    layouts = {path: widgets(text) for path, text in layout_texts.items()}
    names = {name for layout in layouts.values() for name in layout}
    external = {"ContextMenu", "m_wBackground", "KitsStageWorld"}
    for folder in ("Scripts/Game/DCO/GMUI", "Scripts/Game/UI2", "Scripts/Game/DCO/Arsenal/Vehicle"):
        for source in ROOT.joinpath(folder).rglob("*.c"):
            text = source.read_text(encoding="utf-8-sig")
            for name in re.findall(r'FindAnyWidget\("([^"\n]+)"\)', text):
                checks += 1
                if name not in names and name not in external:
                    errors.append(f"{source.name}: missing widget {name}")
    pairs = {
        "Scripts/Game/DCO/GMUI/Tools/DCO_GMMissionPanel.c": "UI/layouts/DCO_GMMissionTools.layout",
        "Scripts/Game/DCO/GMUI/Compositions/DCO_GMCompositionPanel.c": "UI/layouts/DCO_GMCompositions.layout",
        "Scripts/Game/DCO/GMUI/Create/DCO_GMCreatePanelComponent.c": "UI/layouts/DCO_GMPanel.layout",
    }
    for source in ROOT.joinpath("Scripts/Game/UI2/Screens").glob("BIA_*Screen.c"):
        name = source.stem.removeprefix("BIA_").removesuffix("Screen")
        pairs[source.relative_to(ROOT).as_posix()] = f"UI/layouts/Menus/ArmoryV2/GRSA_Screen{name}.layout"
    for source, layout in pairs.items():
        text = ROOT.joinpath(source).read_text(encoding="utf-8-sig")
        for kind, name in re.findall(r'(\w+Widget)\.Cast\([^;\n]*?FindAnyWidget\("([^"\n]+)"\)\)', text):
            checks += 1
            actual = layouts[layout].get(name)
            if name == "KitsStageWorld" and actual is None and "KitsStage" in layouts[layout] and "CreateFallbackRender(legacyPreview.GetParent())" in text:
                continue
            if actual != kind:
                errors.append(f"{layout}: {name} is {actual}, code expects {kind}")
    generated = [("UI/layouts/DCO_GMMissionTools.layout", "DCO_Mission" + suffix, "ButtonWidget")
                 for suffix in ("Close", "Apply", "Scope", "Include", "Previous", "Next")]
    for index in range(8):
        for suffix, kind in (("", "ButtonWidget"), ("_Label", "TextWidget"), ("_Metadata", "TextWidget"), ("_Bg", "ImageWidget")):
            generated.append(("UI/layouts/DCO_GMCompositions.layout", f"DCO_CompositionRow{index}{suffix}", kind))
    for layout, name, kind in generated:
        checks += 1
        if layouts[layout].get(name) != kind:
            errors.append(f"{layout}: generated control {name} must be {kind}")
    return checks, errors


if __name__ == "__main__":
    texts = {p.relative_to(ROOT).as_posix(): p.read_text(encoding="utf-8-sig") for p in ROOT.glob("UI/**/*.layout")}
    checks, errors = review(texts)
    # In-memory faults verify that the checker catches missing and mistyped controls.
    broken = dict(texts)
    mission = "UI/layouts/DCO_GMMissionTools.layout"
    broken[mission] = broken[mission].replace('Name "DCO_MissionApply"', 'Name "MissingApply"')
    assert any("DCO_MissionApply" in e for e in review(broken)[1])
    broken = dict(texts)
    broken[mission] = re.sub(r'EditBoxWidgetClass([^{}]*(?:"\{[^}]+\}"))?', lambda m: m.group(0).replace("EditBoxWidgetClass", "TextWidgetClass"), broken[mission])
    assert any("code expects EditBoxWidget" in e for e in review(broken)[1])
    print(f"{checks} binding checks across {len(texts)} layouts; {len(errors)} failures; 2 fault-injection checks passed")
    for error in errors:
        print(error)
    raise SystemExit(bool(errors))
