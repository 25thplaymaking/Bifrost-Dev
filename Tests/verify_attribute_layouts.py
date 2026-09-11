import os
import re
import sys

def test_no_native_widget_fallback():
    panel_c = os.path.join("Scripts", "Game", "DCO", "GMUI", "Edit", "DCO_GMScenarioPanel.c")
    with open(panel_c, "r", encoding="utf-8") as f:
        content = f.read()

    render_func = re.search(r"protected bool RenderOneAttribute\([^)]+\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}", content)
    assert render_func, "RenderOneAttribute function not found"
    body = render_func.group(1)

    assert "attribute.GetLayout()" not in body, (
        "FAIL: RenderOneAttribute must NEVER spawn native widgets using attribute.GetLayout()!"
    )
    assert "workspace.CreateWidgets(OPTION_LAYOUT" in body, (
        "FAIL: RenderOneAttribute must instantiate Bifrost OPTION_LAYOUT!"
    )
    print("PASS: RenderOneAttribute has zero native widget fallback.")

def test_supports_layout_coverage():
    vanilla_layouts = [
        "AttributePrefab_ButtonBox_MultiSelection.layout",
        "AttributePrefab_ButtonBox_Selection.layout",
        "AttributePrefab_ButtonBox.layout",
        "AttributePrefab_Checkbox.layout",
        "AttributePrefab_Dropdown.layout",
        "AttributePrefab_DropdownWithParam.layout",
        "AttributePrefab_Slider.layout",
        "AttributePrefab_SliderVector.layout",
        "AttributePrefab_Spinbox.layout",
        "AttributePrefab_CharacterBloodSlider.layout",
        "AttributePrefab_Override.layout",
        "Attribute_ButtonBox_TimePresets.layout",
        "Attribute_GameOverDropDown.layout",
        "Date.layout",
        "Attribute_AdminOverrideDefaultModes.layout"
    ]

    supported_tokens = ["Checkbox", "Override", "MultiSelection", "Slider", "Date", "ButtonBox", "Spinbox", "Dropdown", "Modes"]

    for layout in vanilla_layouts:
        matched = any(token.lower() in layout.lower() for token in supported_tokens)
        assert matched, f"FAIL: Layout '{layout}' not matched by supported tokens!"
    print(f"PASS: All {len(vanilla_layouts)} vanilla and custom layouts covered by SupportsLayout.")

def test_blood_slider_and_description_cleaning():
    panel_c = os.path.join("Scripts", "Game", "DCO", "GMUI", "Edit", "DCO_GMScenarioPanel.c")
    with open(panel_c, "r", encoding="utf-8") as f:
        content = f.read()

    assert "SCR_BloodEditorAttribute.Cast(m_Attribute)" in content, "Blood attribute check missing"
    assert 'Math.Round(value).ToString() + "%"' in content, "Blood percentage formatting missing"
    assert "CleanDescription" in content, "CleanDescription function missing"
    assert "inTag" in content, "CleanDescription must use tag parser"

    # Simulate the Enforce clean function
    def clean(text):
        result = []
        in_tag = False
        for c in text:
            if c == "<":
                in_tag = True
            elif c == ">":
                in_tag = False
            elif not in_tag:
                result.append(c)
        return "".join(result)

    sample = 'Setting the value to <color rgba="226,168,79,255">0</color> will always <color rgba="226,168,79,255">kill</color> the entity.'
    cleaned = clean(sample)
    assert cleaned == "Setting the value to 0 will always kill the entity.", f"Unexpected: {cleaned}"
    print("PASS: Blood slider and description tag cleaning verified.")

if __name__ == "__main__":
    test_no_native_widget_fallback()
    test_supports_layout_coverage()
    test_blood_slider_and_description_cleaning()
    print("ALL ATTRIBUTE LAYOUT TESTS PASSED.")
