
// One small handler type serves the previous/value/next controls and the eight segmented choices.
class DCO_ScenarioOptionHandler : ScriptedWidgetEventHandler
{
	protected DCO_ScenarioOptionRow m_Row;
	protected int m_Action;

	void DCO_ScenarioOptionHandler(DCO_ScenarioOptionRow row, int action)
	{
		m_Row = row;
		m_Action = action;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Row)
			return false;
		return m_Row.OnAction(m_Action);
	}
}

class DCO_ScenarioCalendarYearHandler : ScriptedWidgetEventHandler
{
	protected DCO_ScenarioOptionRow m_Row;

	void DCO_ScenarioCalendarYearHandler(DCO_ScenarioOptionRow row)
	{
		m_Row = row;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (!finished || !m_Row)
			return false;
		EditBoxWidget edit = EditBoxWidget.Cast(w);
		if (!edit)
			return false;
		return m_Row.OnCalendarYearChanged(edit.GetText());
	}
}

class DCO_ScenarioTimeValueHandler : ScriptedWidgetEventHandler
{
	protected DCO_ScenarioOptionRow m_Row;

	void DCO_ScenarioTimeValueHandler(DCO_ScenarioOptionRow row)
	{
		m_Row = row;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (!finished || !m_Row)
			return false;
		EditBoxWidget edit = EditBoxWidget.Cast(w);
		if (!edit)
			return false;
		return m_Row.OnTimeValueChanged(edit.GetText());
	}
}

// Bifrost's replacement for the engine AttributePrefab_* family.
class DCO_ScenarioOptionRow
{
	protected static const int MODE_BOOL = 0;
	protected static const int MODE_SLIDER = 1;
	protected static const int MODE_ENUM = 2;
	protected static const int MODE_MULTI = 3;
	protected static const int MODE_DATE = 4;
	protected static const int SEGMENT_ACTION = 100;
	protected static const int MAX_SEGMENTS = 8;
	protected static const int WIDE_SLIDER_STEPS = 20;
	protected static const float CONTROL_WIDTH = 896.0;
	protected static const int CALENDAR_DAY_ACTION = 200;
	protected static const int CALENDAR_PREV_MONTH = -20;
	protected static const int CALENDAR_NEXT_MONTH = 20;
	protected static const int CALENDAR_PREV_YEAR = -21;
	protected static const int CALENDAR_NEXT_YEAR = 21;
	protected static const ResourceName CALENDAR_LAYOUT = "{DCA1E0C000000001}UI/layouts/DCO_GMScenarioCalendar.layout";
	protected static const ResourceName CALENDAR_WEEK_LAYOUT = "{DCA1E0C000000002}UI/layouts/DCO_GMScenarioCalendarWeek.layout";

	protected Widget m_Root;
	protected Widget m_Control;
	protected Widget m_Stepper;
	protected Widget m_Segments;
	protected Widget m_SegmentsTop;
	protected Widget m_SegmentsBottom;
	protected Widget m_SliderControl;
	protected Widget m_CalendarControl;
	protected Widget m_CalendarWeeks;
	protected SizeLayoutWidget m_OptionSize;
	protected SizeLayoutWidget m_ControlSize;
	protected ImageWidget m_Background;
	protected ImageWidget m_Rule;
	protected ImageWidget m_ValuePlate;
	protected ImageWidget m_ValueAccent;
	protected ImageWidget m_SliderFill;
	protected ImageWidget m_SliderTrackPlate;
	protected TextWidget m_Label;
	protected TextWidget m_Description;
	protected TextWidget m_Value;
	protected TextWidget m_SliderValue;
	protected EditBoxWidget m_SliderTimeEdit;
	protected TextWidget m_PrevLabel;
	protected TextWidget m_NextLabel;
	protected TextWidget m_CalendarMonthLabel;
	protected EditBoxWidget m_CalendarYearEdit;
	protected ButtonWidget m_Prev;
	protected ButtonWidget m_ValueButton;
	protected ButtonWidget m_Next;
	protected ButtonWidget m_SliderTrack;
	protected ref DCO_GMSlider m_Slider;
	protected ref array<SizeLayoutWidget> m_SegmentSizes = {};
	protected ref array<ButtonWidget> m_SegmentButtons = {};
	protected ref array<ImageWidget> m_SegmentPlates = {};
	protected ref array<TextWidget> m_SegmentLabels = {};
	protected ref array<ButtonWidget> m_CalendarDayButtons = {};
	protected ref array<ImageWidget> m_CalendarDayPlates = {};
	protected ref array<TextWidget> m_CalendarDayLabels = {};
	protected ref array<int> m_CalendarDays = {};
	protected ref array<ref DCO_ScenarioOptionHandler> m_Handlers = {};
	protected ref DCO_ScenarioCalendarYearHandler m_CalendarYearHandler;
	protected ref DCO_ScenarioTimeValueHandler m_TimeValueHandler;

	protected SCR_BaseEditorAttribute m_Attribute;
	protected SCR_AttributesManagerEditorComponent m_Manager;
	protected DCO_GMScenarioPanel m_Owner;
	protected int m_Mode;
	protected float m_Min;
	protected float m_Max;
	protected float m_Step = 1;
	protected bool m_bUseWideSlider;
	protected bool m_bIsTimeSlider;
	protected ref SCR_BaseEditorAttributeEntrySlider m_SliderData;
	protected ref SCR_EditorAttributeEntryStringArray m_DateMonths;
	protected ref SCR_EditorAttributeEntryIntArray m_DateYears;
	protected bool m_UsesCustomFlags;
	protected bool m_bCalendarInitialized;
	protected bool m_bCalendarRefreshing;
	protected bool m_bSliderValueRefreshing;
	protected bool m_bReadOnlyReview;
	protected int m_CalendarMonth;
	protected int m_CalendarYear;
	protected ref array<string> m_Options = {};
	protected ref array<float> m_OptionValues = {};

	bool Init(Widget root, SCR_BaseEditorAttribute attribute, SCR_AttributesManagerEditorComponent manager, DCO_GMScenarioPanel owner)
	{
		if (!root || !attribute || !manager || !owner)
			return false;

		m_Root = root;
		m_Attribute = attribute;
		m_Manager = manager;
		m_Owner = owner;
		m_bReadOnlyReview = DCO_TriggerReviewEditorAttributeBase.Cast(attribute) != null;
		m_Control = root.FindAnyWidget("DCO_OptionControl");
		m_OptionSize = SizeLayoutWidget.Cast(root.FindAnyWidget("DCO_OptionSize"));
		m_ControlSize = SizeLayoutWidget.Cast(root.FindAnyWidget("DCO_OptionControlSize"));
		m_Stepper = root.FindAnyWidget("DCO_OptionStepper");
		m_Segments = root.FindAnyWidget("DCO_OptionSegments");
		m_SegmentsTop = root.FindAnyWidget("DCO_OptionSegmentsTop");
		m_SegmentsBottom = root.FindAnyWidget("DCO_OptionSegmentsBottom");
		m_SliderControl = root.FindAnyWidget("DCO_OptionSlider");
		m_Background = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionBackground"));
		m_Rule = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionRule"));
		m_ValuePlate = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionValuePlate"));
		m_ValueAccent = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionValueAccent"));
		m_SliderFill = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionSliderFill"));
		m_SliderTrackPlate = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionSliderTrackPlate"));
		m_Label = TextWidget.Cast(root.FindAnyWidget("DCO_OptionLabel"));
		m_Description = TextWidget.Cast(root.FindAnyWidget("DCO_OptionDescription"));
		m_Value = TextWidget.Cast(root.FindAnyWidget("DCO_OptionValue"));
		m_SliderValue = TextWidget.Cast(root.FindAnyWidget("DCO_OptionSliderValue"));
		m_SliderTimeEdit = EditBoxWidget.Cast(root.FindAnyWidget("DCO_OptionSliderTimeEdit"));
		m_PrevLabel = TextWidget.Cast(root.FindAnyWidget("DCO_OptionPrevLabel"));
		m_NextLabel = TextWidget.Cast(root.FindAnyWidget("DCO_OptionNextLabel"));
		m_Prev = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionPrev"));
		m_ValueButton = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionValueButton"));
		m_Next = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionNext"));
		m_SliderTrack = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionSliderTrack"));

		if (!m_Control || !m_OptionSize || !m_ControlSize || !m_Stepper || !m_Segments || !m_SegmentsTop)
			return false;
		if (!m_SegmentsBottom || !m_SliderControl || !m_Label || !m_Description || !m_Value || !m_SliderValue)
			return false;
		if (!m_SliderTimeEdit || !m_Prev || !m_ValueButton || !m_Next || !m_SliderTrack || !m_SliderFill)
			return false;

		BindButton(m_Prev, -1);
		BindButton(m_ValueButton, 0);
		BindButton(m_Next, 1);
		for (int i = 0; i < MAX_SEGMENTS; i++)
		{
			SizeLayoutWidget segmentSize = SizeLayoutWidget.Cast(root.FindAnyWidget("DCO_OptionSeg" + i.ToString() + "Size"));
			ButtonWidget segmentButton = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionSeg" + i.ToString()));
			ImageWidget segmentPlate = ImageWidget.Cast(root.FindAnyWidget("DCO_OptionSeg" + i.ToString() + "Plate"));
			TextWidget segmentLabel = TextWidget.Cast(root.FindAnyWidget("DCO_OptionSeg" + i.ToString() + "Label"));
			m_SegmentSizes.Insert(segmentSize);
			m_SegmentButtons.Insert(segmentButton);
			m_SegmentPlates.Insert(segmentPlate);
			m_SegmentLabels.Insert(segmentLabel);
			if (segmentLabel)
				segmentLabel.SetTextWrapping(true);
			if (segmentButton)
				BindButton(segmentButton, SEGMENT_ACTION + i);
		}

		if (m_Description)
			m_Description.SetTextWrapping(true);
		if (m_bReadOnlyReview)
		{
			// Finalize summaries can wrap. Keep both hosts content-sized so their
			// descriptions cannot paint into the following option row.
			m_OptionSize.EnableHeightOverride(false);
			m_ControlSize.EnableHeightOverride(false);
		}
		VerticalLayoutSlot.SetPadding(m_Root, 0, 0, 0, 7);
		ResolveDataShape();
		m_SliderValue.SetVisible(!m_bIsTimeSlider);
		m_SliderTimeEdit.SetVisible(m_bIsTimeSlider);
		if (m_bIsTimeSlider)
		{
			m_TimeValueHandler = new DCO_ScenarioTimeValueHandler(this);
			m_SliderTimeEdit.AddHandler(m_TimeValueHandler);
		}
		if (m_Mode == MODE_DATE && !InitCalendar())
			return false;
		if (m_bUseWideSlider)
		{
			SCR_BaseEditorAttributeVar sliderVar = m_Attribute.GetVariableOrCopy();
			float initialValue = m_Min;
			if (sliderVar)
				initialValue = sliderVar.GetFloat();
			m_Slider = new DCO_GMSlider();
			m_Slider.Init(root, "DCO_OptionSliderTrack", "DCO_OptionSliderFill", "DCO_OptionSliderValue",
				m_Min, m_Max, initialValue, "");
			m_Slider.ConfigureScale(root, "DCO_OptionSliderThumb", "DCO_OptionSliderTick", "DCO_OptionSliderTickLabel", m_bIsTimeSlider, m_Step);
			m_Slider.GetOnChange().Insert(OnSliderChanged);
			GetGame().GetCallqueue().CallLater(RefreshSliderAfterLayout, 0, false);
		}
		Refresh();
		return true;
	}

	protected bool InitCalendar()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;
		m_CalendarControl = workspace.CreateWidgets(CALENDAR_LAYOUT, m_Control);
		if (!m_CalendarControl)
			return false;

		m_CalendarWeeks = m_CalendarControl.FindAnyWidget("DCO_CalendarWeeks");
		m_CalendarMonthLabel = TextWidget.Cast(m_CalendarControl.FindAnyWidget("DCO_CalendarMonthLabel"));
		m_CalendarYearEdit = EditBoxWidget.Cast(m_CalendarControl.FindAnyWidget("DCO_CalendarYear"));
		if (!m_CalendarWeeks || !m_CalendarMonthLabel || !m_CalendarYearEdit)
			return false;

		BindCalendarButton("DCO_CalendarPrevMonth", CALENDAR_PREV_MONTH);
		BindCalendarButton("DCO_CalendarNextMonth", CALENDAR_NEXT_MONTH);
		BindCalendarButton("DCO_CalendarPrevYear", CALENDAR_PREV_YEAR);
		BindCalendarButton("DCO_CalendarNextYear", CALENDAR_NEXT_YEAR);
		m_CalendarYearHandler = new DCO_ScenarioCalendarYearHandler(this);
		m_CalendarYearEdit.AddHandler(m_CalendarYearHandler);

		for (int weekIndex = 0; weekIndex < 6; weekIndex++)
		{
			Widget week = workspace.CreateWidgets(CALENDAR_WEEK_LAYOUT, m_CalendarWeeks);
			if (!week)
				return false;
			for (int column = 0; column < 7; column++)
			{
				int cell = weekIndex * 7 + column;
				ButtonWidget button = ButtonWidget.Cast(week.FindAnyWidget("DCO_CalendarDay" + column.ToString()));
				ImageWidget plate = ImageWidget.Cast(week.FindAnyWidget("DCO_CalendarDay" + column.ToString() + "Plate"));
				TextWidget label = TextWidget.Cast(week.FindAnyWidget("DCO_CalendarDay" + column.ToString() + "Label"));
				if (!button || !plate || !label)
					return false;
				m_CalendarDayButtons.Insert(button);
				m_CalendarDayPlates.Insert(plate);
				m_CalendarDayLabels.Insert(label);
				m_CalendarDays.Insert(0);
				BindButton(button, CALENDAR_DAY_ACTION + cell);
			}
		}

		m_OptionSize.EnableHeightOverride(true);
		m_OptionSize.SetHeightOverride(340);
		m_ControlSize.EnableHeightOverride(true);
		m_ControlSize.SetHeightOverride(244);
		return m_CalendarDayButtons.Count() == 42;
	}

	protected void BindCalendarButton(string name, int action)
	{
		ButtonWidget button = ButtonWidget.Cast(m_CalendarControl.FindAnyWidget(name));
		if (button)
			BindButton(button, action);
	}

	protected void BindButton(ButtonWidget button, int action)
	{
		if (!button)
			return;
		DCO_ScenarioOptionHandler handler = new DCO_ScenarioOptionHandler(this, action);
		button.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	protected void ResolveDataShape()
	{
		string layout = m_Attribute.GetLayout();
		if (layout.Contains("Checkbox"))
			m_Mode = MODE_BOOL;
		else if (layout.Contains("MultiSelection"))
			m_Mode = MODE_MULTI;
		else if (layout.Contains("Slider"))
			m_Mode = MODE_SLIDER;
		else if (layout.Contains("Date.layout"))
			m_Mode = MODE_DATE;
		else
			m_Mode = MODE_ENUM;

		array<ref SCR_BaseEditorAttributeEntry> entries = {};
		m_Attribute.GetEntries(entries);
		foreach (SCR_BaseEditorAttributeEntry entry : entries)
		{
			SCR_BaseEditorAttributeEntryTimeSlider timeSliderData = SCR_BaseEditorAttributeEntryTimeSlider.Cast(entry);
			if (timeSliderData)
			{
				m_bIsTimeSlider = true;
				continue;
			}

			SCR_BaseEditorAttributeEntrySlider sliderData = SCR_BaseEditorAttributeEntrySlider.Cast(entry);
			if (sliderData)
			{
				m_SliderData = sliderData;
				m_SliderData.GetSliderMinMaxStep(m_Min, m_Max, m_Step);
				continue;
			}

			SCR_BaseEditorAttributeFloatStringValues valueData = SCR_BaseEditorAttributeFloatStringValues.Cast(entry);
			if (valueData)
			{
				for (int v = 0; v < valueData.GetValueCount(); v++)
				{
					SCR_EditorAttributeFloatStringValueHolder holder = valueData.GetValuesEntry(v);
					if (!holder)
						continue;
					string optionName = holder.GetName();
					if (optionName.IsEmpty())
						optionName = holder.GetDescription();
					m_Options.Insert(optionName);
					m_OptionValues.Insert(holder.GetFloatValue());
				}
				continue;
			}

			SCR_EditorAttributePresetMultiSelectEntry multiPreset = SCR_EditorAttributePresetMultiSelectEntry.Cast(entry);
			if (multiPreset)
			{
				m_UsesCustomFlags = multiPreset.GetUsesCustomFlags();
				continue;
			}

			SCR_EditorAttributeEntryStringArray monthData = SCR_EditorAttributeEntryStringArray.Cast(entry);
			if (monthData)
			{
				m_DateMonths = monthData;
				continue;
			}

			SCR_EditorAttributeEntryIntArray yearData = SCR_EditorAttributeEntryIntArray.Cast(entry);
			if (yearData)
			{
				m_DateYears = yearData;
				continue;
			}

			if (m_Mode == MODE_ENUM)
			{
				SCR_BaseEditorAttributeEntryText textData = SCR_BaseEditorAttributeEntryText.Cast(entry);
				if (textData)
				{
					m_Options.Insert(textData.GetText());
					m_OptionValues.Insert(m_OptionValues.Count());
					continue;
				}

				SCR_BaseEditorAttributeEntryUIInfo infoData = SCR_BaseEditorAttributeEntryUIInfo.Cast(entry);
				if (infoData && infoData.GetInfo())
				{
					m_Options.Insert(infoData.GetInfo().GetName());
					m_OptionValues.Insert(infoData.GetValue());
				}
			}
		}

		if (m_Mode == MODE_SLIDER && m_SliderData && m_Step > 0)
			m_bUseWideSlider = ((m_Max - m_Min) / m_Step) > WIDE_SLIDER_STEPS;
	}

	void Refresh()
	{
		if (!m_Attribute || !m_Root)
			return;

		DCO_GMTheme theme = DCO_GMTheme.Get();
		SCR_EditorAttributeUIInfo info = m_Attribute.GetUIInfo();
		if (info)
		{
			m_Label.SetText(info.GetName());
			m_Description.SetText(GetReadableDescription(info.GetDescription()));
		}
		m_Label.SetColor(theme.m_TextColor);
		m_Description.SetColor(Color.FromRGBA(210, 216, 219, 255));
		m_Value.SetColor(theme.m_TextColor);
		m_SliderValue.SetColor(theme.m_TextColor);
		m_SliderTimeEdit.SetColor(theme.m_TextColor);

		if (m_Background)
			m_Background.SetColor(Color.FromRGBA(11, 14, 15, 232));
		if (m_Rule)
		{
			m_Rule.SetColor(theme.m_AccentColor);
			m_Rule.SetOpacity(0.22);
		}
		if (m_ValueAccent)
			m_ValueAccent.SetColor(theme.m_AccentColor);
		if (m_SliderFill)
			m_SliderFill.SetColor(theme.m_AccentColor);
		if (m_SliderTrackPlate)
			m_SliderTrackPlate.SetColor(theme.m_TrackColor);
		if (m_PrevLabel)
			m_PrevLabel.SetColor(theme.m_AccentColor);
		if (m_NextLabel)
			m_NextLabel.SetColor(theme.m_AccentColor);

		bool enabled = m_Attribute.IsEnabled() && !m_bReadOnlyReview;
		if (enabled)
			m_Root.SetOpacity(1.0);
		else if (m_bReadOnlyReview)
			m_Root.SetOpacity(1.0);
		else
			m_Root.SetOpacity(0.38);
		m_Prev.SetEnabled(enabled);
		m_ValueButton.SetEnabled(enabled);
		m_Next.SetEnabled(enabled);
		m_SliderTrack.SetEnabled(enabled);
		m_SliderTimeEdit.SetEnabled(enabled && m_bIsTimeSlider);

		if (m_bReadOnlyReview)
			RefreshReview();
		else if (m_Mode == MODE_DATE)
			RefreshCalendar(enabled);
		else if (m_bUseWideSlider)
			RefreshWideSlider(enabled);
		else if (m_Mode == MODE_BOOL)
			RefreshBool(enabled);
		else if (m_Mode == MODE_MULTI)
			RefreshMulti(enabled);
		else if (m_Mode == MODE_ENUM && m_Options.Count() > 0 && m_Options.Count() <= MAX_SEGMENTS)
			RefreshEnumSegments(enabled);
		else
			RefreshStepper(enabled);
	}

	protected void SetControlMode(bool segments, bool slider, bool calendar = false)
	{
		m_Segments.SetVisible(segments);
		m_SliderControl.SetVisible(slider);
		m_Stepper.SetVisible(!segments && !slider && !calendar);
		if (m_CalendarControl)
			m_CalendarControl.SetVisible(calendar);
	}

	protected void ConfigureSegments(int count, bool enabled)
	{
		SetControlMode(true, false);
		int topCount = Math.Min(count, 4);
		int bottomCount = Math.Max(count - 4, 0);
		m_SegmentsTop.SetVisible(topCount > 0);
		m_SegmentsBottom.SetVisible(bottomCount > 0);

		float topWidth = CONTROL_WIDTH;
		float bottomWidth = CONTROL_WIDTH;
		if (topCount > 0)
			topWidth = topWidth / topCount;
		if (bottomCount > 0)
			bottomWidth = bottomWidth / bottomCount;
		for (int i = 0; i < m_SegmentButtons.Count(); i++)
		{
			bool visible = i < count;
			if (m_SegmentSizes[i])
			{
				float segmentWidth = topWidth;
				if (i >= 4)
					segmentWidth = bottomWidth;
				m_SegmentSizes[i].SetVisible(visible);
				m_SegmentSizes[i].EnableWidthOverride(true);
				m_SegmentSizes[i].SetWidthOverride(segmentWidth);
			}
			if (m_SegmentButtons[i])
			{
				m_SegmentButtons[i].SetVisible(visible);
				m_SegmentButtons[i].SetEnabled(enabled && visible);
			}
		}
	}

	protected void SetSegment(int index, string text, bool selected)
	{
		if (!m_SegmentButtons.IsIndexValid(index) || !m_SegmentLabels[index] || !m_SegmentPlates[index])
			return;
		m_SegmentLabels[index].SetText(text);
		if (selected)
		{
			Color accent = DCO_GMTheme.Get().m_AccentColor;
			m_SegmentPlates[index].SetColor(accent);
			m_SegmentLabels[index].SetColor(GetTextColorForBackground(accent));
		}
		else
		{
			m_SegmentPlates[index].SetColor(Color.FromRGBA(27, 32, 34, 255));
			m_SegmentLabels[index].SetColor(DCO_GMTheme.Get().m_TextColor);
		}
	}

	protected void RefreshBool(bool enabled)
	{
		ConfigureSegments(2, enabled);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		bool value = var && var.GetBool();
		SetSegment(0, "OFF", !value);
		SetSegment(1, "ON", value);
	}

	protected void RefreshEnumSegments(bool enabled)
	{
		ConfigureSegments(m_Options.Count(), enabled);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		int selected = 0;
		if (var)
			selected = var.GetInt();
		for (int i = 0; i < m_Options.Count(); i++)
			SetSegment(i, m_Options[i], i == selected);
	}

	protected bool IsMultiSelected(int option, vector flags)
	{
		if (m_UsesCustomFlags && m_OptionValues.IsIndexValid(option))
			return (((int)flags[0]) & ((int)m_OptionValues[option])) != 0;
		int vectorIndex = option / 31;
		int flagIndex = option % 31;
		return (((int)flags[vectorIndex]) & (1 << flagIndex)) != 0;
	}

	protected void RefreshMulti(bool enabled)
	{
		int count = Math.Min(m_Options.Count(), MAX_SEGMENTS);
		ConfigureSegments(count, enabled);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		vector flags;
		if (var)
			flags = var.GetVector();
		for (int i = 0; i < count; i++)
			SetSegment(i, m_Options[i], IsMultiSelected(i, flags));
	}

	protected void RefreshCalendar(bool enabled)
	{
		SetControlMode(false, false, true);
		if (!m_CalendarControl)
			return;
		m_CalendarControl.SetEnabled(enabled);

		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		SCR_DateEditorAttribute dateAttribute = SCR_DateEditorAttribute.Cast(m_Attribute);
		if (!var || !dateAttribute)
			return;

		vector selectedDate = var.GetVector();
		int selectedDay = ((int)selectedDate[0]) + 1;
		int selectedMonth = (int)selectedDate[1];
		int selectedYear = dateAttribute.DCO_GetYearAtIndex((int)selectedDate[2]);
		if (!m_bCalendarInitialized)
		{
			m_CalendarMonth = selectedMonth;
			m_CalendarYear = selectedYear;
			m_bCalendarInitialized = true;
		}

		string monthName = (m_CalendarMonth + 1).ToString();
		if (m_DateMonths && m_CalendarMonth >= 0 && m_CalendarMonth < m_DateMonths.GetCount())
			monthName = m_DateMonths.GetEntry(m_CalendarMonth);
		m_CalendarMonthLabel.SetText(monthName);
		m_CalendarMonthLabel.SetColor(DCO_GMTheme.Get().m_TextColor);
		m_bCalendarRefreshing = true;
		m_CalendarYearEdit.SetText(m_CalendarYear.ToString());
		m_bCalendarRefreshing = false;

		ImageWidget yearAccent = ImageWidget.Cast(m_CalendarControl.FindAnyWidget("DCO_CalendarYearAccent"));
		if (yearAccent)
			yearAccent.SetColor(DCO_GMTheme.Get().m_AccentColor);

		int firstCell = FirstWeekdayMonday(m_CalendarYear, m_CalendarMonth + 1, 1);
		int dayCount = DaysInCalendarMonth(m_CalendarYear, m_CalendarMonth + 1);
		for (int cell = 0; cell < m_CalendarDayButtons.Count(); cell++)
		{
			int day = cell - firstCell + 1;
			bool visible = day >= 1 && day <= dayCount;
			if (visible)
				m_CalendarDays[cell] = day;
			else
				m_CalendarDays[cell] = 0;
			m_CalendarDayButtons[cell].SetVisible(visible);
			m_CalendarDayButtons[cell].SetEnabled(enabled && visible);
			if (!visible)
				continue;

			m_CalendarDayLabels[cell].SetText(day.ToString());
			bool selected = day == selectedDay && m_CalendarMonth == selectedMonth && m_CalendarYear == selectedYear;
			if (selected)
			{
				Color accent = DCO_GMTheme.Get().m_AccentColor;
				m_CalendarDayPlates[cell].SetColor(accent);
				m_CalendarDayLabels[cell].SetColor(GetTextColorForBackground(accent));
			}
			else
			{
				m_CalendarDayPlates[cell].SetColor(Color.FromRGBA(27, 32, 34, 255));
				m_CalendarDayLabels[cell].SetColor(DCO_GMTheme.Get().m_TextColor);
			}
		}
	}

	protected int DaysInCalendarMonth(int year, int month)
	{
		for (int day = 31; day >= 28; day--)
		{
			if (IsCalendarDateValid(year, month, day))
				return day;
		}
		return 28;
	}

	protected bool IsCalendarDateValid(int year, int month, int day)
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
		if (!timeManager)
			return false;
		return timeManager.CheckValidDate(year, month, day);
	}

	// Gregorian weekday, returned Monday=0 through Sunday=6.
	protected int FirstWeekdayMonday(int year, int month, int day)
	{
		int adjustedYear = year;
		int adjustedMonth = month;
		if (adjustedMonth < 3)
		{
			adjustedMonth += 12;
			adjustedYear--;
		}
		int centuryYear = adjustedYear % 100;
		int century = adjustedYear / 100;
		int zeller = (day + (13 * (adjustedMonth + 1)) / 5 + centuryYear + centuryYear / 4 + century / 4 + 5 * century) % 7;
		return (zeller + 5) % 7;
	}

	protected void NavigateCalendar(int monthDelta, int yearDelta)
	{
		int year = Math.Clamp(m_CalendarYear + yearDelta, 1, 9999);
		int month = m_CalendarMonth + monthDelta;
		while (month < 0 && year > 1)
		{
			month += 12;
			year--;
		}
		while (month > 11 && year < 9999)
		{
			month -= 12;
			year++;
		}
		m_CalendarYear = Math.Clamp(year, 1, 9999);
		m_CalendarMonth = Math.Clamp(month, 0, 11);
		RefreshCalendar(m_Attribute.IsEnabled());
	}

	bool OnCalendarYearChanged(string value)
	{
		if (m_bCalendarRefreshing)
			return false;
		int year = value.ToInt();
		if (year < 1 || year > 9999 || year.ToString() != value)
		{
			RefreshCalendar(m_Attribute.IsEnabled());
			return false;
		}
		m_CalendarYear = year;

		SCR_BaseEditorAttributeVar current = m_Attribute.GetVariableOrCopy();
		if (!current)
		{
			RefreshCalendar(m_Attribute.IsEnabled());
			return false;
		}
		vector currentDate = current.GetVector();
		int day = ((int)currentDate[0]) + 1;
		day = Math.Min(day, DaysInCalendarMonth(m_CalendarYear, m_CalendarMonth + 1));
		return ApplyCalendarDate(day);
	}

	protected bool SelectCalendarDay(int cell)
	{
		if (!m_CalendarDays.IsIndexValid(cell) || m_CalendarDays[cell] <= 0 || !m_Attribute.IsEnabled())
			return false;
		return ApplyCalendarDate(m_CalendarDays[cell]);
	}

	protected bool ApplyCalendarDate(int day)
	{
		SCR_DateEditorAttribute dateAttribute = SCR_DateEditorAttribute.Cast(m_Attribute);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariable(true);
		if (!dateAttribute || !var)
			return false;
		int yearIndex = dateAttribute.DCO_GetYearIndex(m_CalendarYear);
		if (yearIndex < 0 || !IsCalendarDateValid(m_CalendarYear, m_CalendarMonth + 1, day))
			return false;

		var.SetVector(Vector(day - 1, m_CalendarMonth, yearIndex));
		m_Attribute.UpdateInterlinkedVariables(var, m_Manager);
		m_Attribute.PreviewVariable(true, m_Manager);
		RefreshCalendar(true);
		m_Owner.OnAttributeChanged();
		return true;
	}

	protected void RefreshStepper(bool enabled)
	{
		SetControlMode(false, false);
		m_Prev.SetEnabled(enabled);
		m_Next.SetEnabled(enabled);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		if (!var)
		{
			m_Value.SetText("MIXED");
			return;
		}

		if (m_Mode == MODE_SLIDER)
		{
			m_PrevLabel.SetText("-");
			m_NextLabel.SetText("+");
			m_Value.SetText(FormatSlider(var.GetFloat()));
		}
		else if (m_Mode == MODE_DATE)
		{
			m_PrevLabel.SetText("<");
			m_NextLabel.SetText(">");
			m_Value.SetText(FormatDate(var.GetVector()));
		}
		else
		{
			m_PrevLabel.SetText("<");
			m_NextLabel.SetText(">");
			int index = var.GetInt();
			if (m_Options.IsIndexValid(index))
				m_Value.SetText(m_Options[index]);
			else
				m_Value.SetText(index.ToString());
		}
	}

	protected void RefreshWideSlider(bool enabled)
	{
		SetControlMode(false, true);
		m_SliderTrack.SetEnabled(enabled);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariableOrCopy();
		if (!var)
		{
			SetSliderReadout("MIXED");
			return;
		}

		float value = SnapSliderValue(var.GetFloat());
		if (m_Slider)
			m_Slider.SetValue(value);
		SetSliderReadout(FormatSlider(value));
	}

	// The custom row is created before its horizontal layout has a measured width.
	protected void RefreshSliderAfterLayout()
	{
		if (!m_Slider || !m_SliderValue)
			return;
		m_Slider.Refresh();
		SetSliderReadout(FormatSlider(m_Slider.GetValue()));
	}

	protected float SnapSliderValue(float value)
	{
		if (m_Step <= 0)
			return Math.Clamp(value, m_Min, m_Max);
		float snapped = m_Min + Math.Round((value - m_Min) / m_Step) * m_Step;
		return Math.Clamp(snapped, m_Min, m_Max);
	}

	protected void OnSliderChanged(float value)
	{
		if (!m_bUseWideSlider || !m_Attribute || !m_Attribute.IsEnabled())
			return;
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariable(true);
		if (!var)
			return;

		float snapped = SnapSliderValue(value);
		if (m_Slider)
			m_Slider.SetValue(snapped);
		SetSliderReadout(FormatSlider(snapped));
		if (var.GetFloat() != snapped)
		{
			var.SetFloat(snapped);
			m_Attribute.UpdateInterlinkedVariables(var, m_Manager);
			m_Attribute.PreviewVariable(true, m_Manager);
		}

		if (m_Slider && !m_Slider.IsDragging())
			m_Owner.OnAttributeChanged();
	}

	protected void SetSliderReadout(string value)
	{
		m_bSliderValueRefreshing = true;
		m_SliderValue.SetText(value);
		m_SliderTimeEdit.SetText(value);
		m_bSliderValueRefreshing = false;
	}

	bool OnTimeValueChanged(string value)
	{
		if (m_bSliderValueRefreshing || !m_bIsTimeSlider || !m_Attribute || !m_Attribute.IsEnabled())
			return false;

		array<string> parts = {};
		value.Split(":", parts, true);
		if (parts.Count() != 2 || parts[0].Length() != 2 || parts[1].Length() != 2)
		{
			SetSliderReadout(FormatSlider(m_Slider.GetValue()));
			return false;
		}

		int hours = parts[0].ToInt();
		int minutes = parts[1].ToInt();
		if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || TwoDigits(hours) != parts[0] || TwoDigits(minutes) != parts[1])
		{
			SetSliderReadout(FormatSlider(m_Slider.GetValue()));
			return false;
		}

		OnSliderChanged(hours * 3600 + minutes * 60);
		return true;
	}

	void CommitPendingEdits()
	{
		if (m_bIsTimeSlider && m_SliderTimeEdit)
			OnTimeValueChanged(m_SliderTimeEdit.GetText());
		if (m_Mode == MODE_DATE && m_CalendarYearEdit)
			OnCalendarYearChanged(m_CalendarYearEdit.GetText());
	}

	protected string FormatSlider(float value)
	{
		if (m_bIsTimeSlider)
			return FormatTimeOfDay(value);

		if (!m_SliderData)
			return value.ToString();
		string raw = m_SliderData.GetText(value);
		string format = m_SliderData.GetSliderValueFormating();
		if (!format.IsEmpty() && format.Contains("%1"))
			return string.Format(format, raw);
		return raw;
	}

	// The authoritative daytime attribute stores seconds.
	protected string FormatTimeOfDay(float seconds)
	{
		int totalMinutes = Math.Round(seconds / 60.0);
		totalMinutes = totalMinutes % 1440;
		if (totalMinutes < 0)
			totalMinutes += 1440;
		int hours = totalMinutes / 60;
		int minutes = totalMinutes % 60;
		return TwoDigits(hours) + ":" + TwoDigits(minutes);
	}

	protected string TwoDigits(int value)
	{
		if (value < 10)
			return "0" + value.ToString();
		return value.ToString();
	}

	// engine's date description is dynamic rich text.
	protected string GetReadableDescription(string fallback)
	{
		if (SCR_TimePresetsEditorAttribute.Cast(m_Attribute))
			return "Choose a daylight preset. It updates the exact time below and previews immediately; changes apply when the settings are saved and closed.";
		if (m_bIsTimeSlider)
			return "Drag the bar or type an HH:MM time on the 24-hour clock. Values use 15-minute steps, preview immediately, and apply when the settings are saved and closed.";
		if (SCR_DateEditorAttribute.Cast(m_Attribute))
			return "Choose a day from the calendar, use the arrows to change month or year, or type any year from 1 to 9999. Sunrise, sunset, and moon phase update automatically.";
		return fallback;
	}

	// Selected controls use the accent as their background.
	protected Color GetTextColorForBackground(Color background)
	{
		if (!background)
			return Color.FromRGBA(238, 242, 244, 255);
		float luminance = background.R() * 0.299 + background.G() * 0.587 + background.B() * 0.114;
		if (luminance >= 0.52)
			return Color.FromRGBA(0, 0, 0, 255);
		return Color.FromRGBA(255, 255, 255, 255);
	}

	protected string FormatDate(vector date)
	{
		int day = ((int)date[0]) + 1;
		int month = (int)date[1];
		int yearIndex = (int)date[2];
		string monthText = (month + 1).ToString();
		if (m_DateMonths && month >= 0 && month < m_DateMonths.GetCount())
			monthText = m_DateMonths.GetEntry(month);
		string yearText = yearIndex.ToString();
		if (m_DateYears && yearIndex >= 0 && yearIndex < m_DateYears.GetCount())
			yearText = m_DateYears.GetEntry(yearIndex).ToString();
		return day.ToString() + " " + monthText + " " + yearText;
	}

	bool OnAction(int action)
	{
		if (!m_Attribute || !m_Attribute.IsEnabled())
			return false;
		if (action >= CALENDAR_DAY_ACTION)
			return SelectCalendarDay(action - CALENDAR_DAY_ACTION);
		if (action == CALENDAR_PREV_MONTH)
		{
			NavigateCalendar(-1, 0);
			return true;
		}
		if (action == CALENDAR_NEXT_MONTH)
		{
			NavigateCalendar(1, 0);
			return true;
		}
		if (action == CALENDAR_PREV_YEAR)
		{
			NavigateCalendar(0, -1);
			return true;
		}
		if (action == CALENDAR_NEXT_YEAR)
		{
			NavigateCalendar(0, 1);
			return true;
		}
		if (action == 0 && m_Mode == MODE_ENUM && m_Options.Count() > MAX_SEGMENTS)
			return m_Owner.ShowOptionPicker(this, m_ValueButton);
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariable(true);
		if (!var)
			return false;

		if (action >= SEGMENT_ACTION)
		{
			int index = action - SEGMENT_ACTION;
			if (m_Mode == MODE_BOOL)
				var.SetBool(index == 1);
			else if (m_Mode == MODE_MULTI)
				ToggleMulti(var, index);
			else
				var.SetInt(index);
		}
		else if (action != 0)
		{
			if (m_Mode == MODE_SLIDER)
			{
				float value = Math.Clamp(var.GetFloat() + m_Step * action, m_Min, m_Max);
				var.SetFloat(value);
			}
			else if (m_Mode == MODE_DATE)
			{
				var.SetVector(ShiftDate(var.GetVector(), action));
			}
			else if (m_Options.Count() > 0)
			{
				int value = var.GetInt() + action;
				if (value < 0)
					value = m_Options.Count() - 1;
				else if (value >= m_Options.Count())
					value = 0;
				var.SetInt(value);
			}
		}
		else
		{
			return false;
		}

		m_Attribute.UpdateInterlinkedVariables(var, m_Manager);
		m_Attribute.PreviewVariable(true, m_Manager);
		Refresh();
		m_Owner.OnAttributeChanged();
		return true;
	}

	protected void RefreshReview()
	{
		ConfigureSegments(1, false);
		string summary = "Review unavailable";
		if (!m_Options.IsEmpty())
			summary = m_Options[0];
		SetSegment(0, summary, true);
	}

	int GetOptionCount()
	{
		return m_Options.Count();
	}

	string GetOptionName(int index)
	{
		if (!m_Options.IsIndexValid(index))
			return "";
		return m_Options[index];
	}

	bool SelectOption(int index)
	{
		if (!m_Attribute || !m_Attribute.IsEnabled() || !m_Options.IsIndexValid(index))
			return false;
		SCR_BaseEditorAttributeVar var = m_Attribute.GetVariable(true);
		if (!var)
			return false;
		var.SetInt(index);
		m_Attribute.UpdateInterlinkedVariables(var, m_Manager);
		m_Attribute.PreviewVariable(true, m_Manager);
		Refresh();
		m_Owner.OnAttributeChanged();
		return true;
	}

	protected void ToggleMulti(SCR_BaseEditorAttributeVar var, int option)
	{
		if (!m_Options.IsIndexValid(option))
			return;
		vector flags = var.GetVector();
		if (m_UsesCustomFlags && m_OptionValues.IsIndexValid(option))
		{
			int customValue = (int)m_OptionValues[option];
			int currentCustomFlags = (int)flags[0];
			if ((currentCustomFlags & customValue) != 0)
				currentCustomFlags &= ~customValue;
			else
				currentCustomFlags |= customValue;
			flags[0] = currentCustomFlags;
		}
		else
		{
			int vectorIndex = option / 31;
			int flagIndex = option % 31;
			int flag = 1 << flagIndex;
			int currentFlags = (int)flags[vectorIndex];
			if ((currentFlags & flag) != 0)
				currentFlags &= ~flag;
			else
				currentFlags |= flag;
			flags[vectorIndex] = currentFlags;
		}
		var.SetVector(flags);
	}

	protected vector ShiftDate(vector date, int direction)
	{
		int dayIndex = (int)date[0];
		int monthIndex = (int)date[1];
		int yearIndex = (int)date[2];
		int monthCount = 12;
		int yearCount = 1;
		if (m_DateMonths)
			monthCount = m_DateMonths.GetCount();
		if (m_DateYears)
			yearCount = m_DateYears.GetCount();

		dayIndex += direction;
		if (direction > 0 && !IsValidDate(dayIndex, monthIndex, yearIndex))
		{
			dayIndex = 0;
			monthIndex++;
			if (monthIndex >= monthCount)
			{
				monthIndex = 0;
				yearIndex = (yearIndex + 1) % yearCount;
			}
		}
		else if (direction < 0 && dayIndex < 0)
		{
			monthIndex--;
			if (monthIndex < 0)
			{
				monthIndex = monthCount - 1;
				yearIndex--;
				if (yearIndex < 0)
					yearIndex = yearCount - 1;
			}
			dayIndex = 30;
			while (dayIndex > 27 && !IsValidDate(dayIndex, monthIndex, yearIndex))
				dayIndex--;
		}
		return Vector(dayIndex, monthIndex, yearIndex);
	}

	protected bool IsValidDate(int dayIndex, int monthIndex, int yearIndex)
	{
		if (!m_DateYears || yearIndex < 0 || yearIndex >= m_DateYears.GetCount())
			return dayIndex < 31;
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return dayIndex < 31;
		TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
		if (!timeManager)
			return dayIndex < 31;
		return timeManager.CheckValidDate(m_DateYears.GetEntry(yearIndex), monthIndex + 1, dayIndex + 1);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(RefreshSliderAfterLayout);
		if (m_SliderTimeEdit && m_TimeValueHandler)
			m_SliderTimeEdit.RemoveHandler(m_TimeValueHandler);
		m_TimeValueHandler = null;
		if (m_CalendarYearEdit && m_CalendarYearHandler)
			m_CalendarYearEdit.RemoveHandler(m_CalendarYearHandler);
		m_CalendarYearHandler = null;
		if (m_Slider)
		{
			m_Slider.GetOnChange().Remove(OnSliderChanged);
			m_Slider.Shutdown();
			m_Slider = null;
		}
		m_Handlers.Clear();
		m_SegmentSizes.Clear();
		m_SegmentButtons.Clear();
		m_SegmentPlates.Clear();
		m_SegmentLabels.Clear();
		m_CalendarDayButtons.Clear();
		m_CalendarDayPlates.Clear();
		m_CalendarDayLabels.Clear();
		m_CalendarDays.Clear();
		m_Attribute = null;
		m_Manager = null;
		m_Owner = null;
		m_SegmentsTop = null;
		m_SegmentsBottom = null;
		m_CalendarYearEdit = null;
		m_SliderTimeEdit = null;
		m_CalendarMonthLabel = null;
		m_CalendarWeeks = null;
		m_CalendarControl = null;
		m_Control = null;
		m_OptionSize = null;
		m_ControlSize = null;
		m_Root = null;
	}
}

// Three compact profile-backed scenario snapshots.
class DCO_ScenarioPresetValue
{
	string m_sKey;
	int m_iKind;
	bool m_bBool;
	int m_iInt;
	float m_fFloat;
	vector m_vVector;
}

class DCO_ScenarioPresetSlot
{
	bool m_bSaved;
	ref array<ref DCO_ScenarioPresetValue> m_aValues = {};
}

class DCO_ScenarioPresetStore
{
	int m_iVersion = 1;
	ref array<ref DCO_ScenarioPresetSlot> m_aSlots = {};
}

class DCO_ScenarioButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMScenarioPanel m_Owner;

	void DCO_ScenarioButtonHandler(DCO_GMScenarioPanel owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(w);
		return false;
	}
}

// A Bifrost-owned properties session is modal. Consume every mouse phase so
// world selection and the panels below cannot receive a click through it.
class DCO_ScenarioBackdropHandler : ScriptedWidgetEventHandler
{
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return true;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return true;
	}
}

// Give every layout-defined category button its own stable category index.
class DCO_ScenarioCategoryHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMScenarioPanel m_Owner;
	protected int m_Index;

	void DCO_ScenarioCategoryHandler(DCO_GMScenarioPanel owner, int index)
	{
		m_Owner = owner;
		m_Index = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner)
			return false;
		return m_Owner.SelectCategory(m_Index);
	}
}

class DCO_ScenarioScrollHandler : ScriptedWidgetEventHandler
{
	protected ScrollLayoutWidget m_Scroll;

	void DCO_ScenarioScrollHandler(ScrollLayoutWidget scroll)
	{
		m_Scroll = scroll;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_Scroll)
			return false;
		float sx, sy;
		m_Scroll.GetSliderPos(sx, sy);
		sy = Math.Clamp(sy - wheel * 0.08, 0.0, 1.0);	// wheel up -> scroll up.
		m_Scroll.SetSliderPos(sx, sy);
		return true;	// consume so the editor camera doesn't zoom.
	}
}

class DCO_GMScenarioPanel
{
	protected static const ResourceName OPTION_LAYOUT = "{2A2733736B074188}UI/layouts/DCO_GMScenarioOption.layout";
	protected static const string PRESET_FILE = "$profile:BifrostScenarioPresets.json";
	protected static const int PRESET_SLOT_COUNT = 3;
	protected static const int PRESET_BOOL = 0;
	protected static const int PRESET_FLOAT = 1;
	protected static const int PRESET_INT = 2;
	protected static const int PRESET_VECTOR = 3;
	protected static const int OPTION_PAGE_SIZE = 16;
	protected static const int OPTION_PREVIOUS = 900001;
	protected static const int OPTION_NEXT = 900002;
	protected static const ResourceName TRIGGER_SETUP_CATEGORY = "{5DC0DCB10F1ACE01}Configs/Editor/AttributeCategories/DCO_TriggerSetup.conf";
	protected static const ResourceName TRIGGER_UNITS_CATEGORY = "{5DC0DCB20F1ACE01}Configs/Editor/AttributeCategories/DCO_TriggerUnits.conf";
	protected static const ResourceName TRIGGER_FINALIZE_CATEGORY = "{5DC0DCB30F1ACE01}Configs/Editor/AttributeCategories/DCO_TriggerFinalize.conf";

	protected Widget m_wRoot;
	protected Widget m_wBackdrop;
	protected Widget m_wPanel;
	protected Widget m_wContent;
	protected Widget m_wPlaceholder;
	protected TextWidget m_wTitle;
	protected TextWidget m_wCloseLabel;
	protected ScrollLayoutWidget m_wScroll;
	protected SizeLayoutWidget m_wSizeBox;
	protected ButtonWidget m_btnCog;
	protected ButtonWidget m_btnClose;
	protected Widget m_wPresetBar;
	protected Widget m_wPresetMenu;
	protected ButtonWidget m_btnPresetSelect;
	protected ButtonWidget m_btnPresetLoad;
	protected ButtonWidget m_btnPresetSave;
	protected TextWidget m_wPresetSelectLabel;
	protected ref array<ButtonWidget> m_PresetSlotBtns = {};
	protected ref array<TextWidget> m_PresetSlotLabels = {};
	protected ref DCO_ScenarioButtonHandler m_Handler;
	protected ref DCO_ScenarioBackdropHandler m_BackdropHandler;
	protected ref DCO_ScenarioScrollHandler m_ScrollHandler;
	protected DCO_GMContextMenu m_Menu;
	protected ref ScriptInvoker m_OptionMenuCallback = new ScriptInvoker();
	protected DCO_ScenarioOptionRow m_OptionPickerRow;
	protected Widget m_OptionPickerAnchor;
	protected int m_OptionPickerPage;
	protected SCR_AttributesManagerEditorComponent m_Manager;
	protected bool m_bOpen;
	protected bool m_bEditing;	// true between StartEditing and ConfirmEditing.
	protected bool m_bSubscribed;	// guard so we hook GetOnAttributesStart only once.
	protected bool m_bCogSession;
	protected ref array<SCR_BaseEditorAttribute> m_aSessionAttributes;
	protected ref array<ref DCO_ScenarioOptionRow> m_Rows = {};
	protected bool m_bHasContinuousFire;	// current session contains the Loiter Continuous Fire row.
	protected bool m_bContinuousFire;	// live manager value used to hide/show the Rounds row.
	protected bool m_bConditionalRefreshQueued;
	protected bool m_bCategoryRefreshQueued;
	protected ref DCO_ScenarioPresetStore m_PresetStore;
	protected int m_iPresetSlot;

	protected int m_ActiveCat;
	protected int m_CategoryCount;
	protected int m_CategoryPage;
	protected int m_TimeDateOpenAttempts;
	protected bool m_bTriggerSession;
	protected int m_iTriggerFinalizeCategory = -1;
	protected int m_iTriggerAction;
	protected int m_iTriggerTimerMode;
	protected int m_iTriggerOwnerMode;
	protected int m_iTriggerLinkedUnitMode;
	protected int m_iTriggerLinkedCount;
	protected bool m_bTriggerRepeat;
	protected string m_sTriggerActiveStep = "SETUP";
	protected ref array<ButtonWidget> m_CatTabBtns = {};
	protected ref array<TextWidget> m_CatTabLabels = {};
	protected ref array<ref DCO_ScenarioCategoryHandler> m_CatHandlers = {};

	void Init(Widget root, DCO_GMContextMenu menu)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Menu = menu;
		m_OptionMenuCallback.Insert(OnOptionPickerAction);
		m_Handler = new DCO_ScenarioButtonHandler(this);

		m_wBackdrop    = root.FindAnyWidget("DCO_ScenarioBackdrop");
		m_wPanel       = root.FindAnyWidget("DCO_ScenarioPanel");
		m_wContent     = root.FindAnyWidget("DCO_ScenarioContent");
		m_wPlaceholder = root.FindAnyWidget("DCO_ScenarioPlaceholder");
		m_wTitle       = TextWidget.Cast(root.FindAnyWidget("DCO_ScenarioTitle"));
		m_wCloseLabel  = TextWidget.Cast(root.FindAnyWidget("DCO_ScenarioClose_Label"));
		m_wScroll      = ScrollLayoutWidget.Cast(root.FindAnyWidget("DCO_ScenarioScroll"));
		m_wSizeBox     = SizeLayoutWidget.Cast(root.FindAnyWidget("DCO_ScenarioScrollHost"));
		m_wPresetBar   = root.FindAnyWidget("DCO_ScenarioPresetBar");
		m_wPresetMenu  = root.FindAnyWidget("DCO_ScenarioPresetMenu");
		m_btnPresetSelect = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetSelect"));
		m_btnPresetLoad   = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetLoad"));
		m_btnPresetSave   = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetSave"));
		m_wPresetSelectLabel = TextWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetSelect_Label"));
		if (m_wSizeBox)
		{
			// CLAMP the scroll viewport to a fixed height.
			m_wSizeBox.EnableHeightOverride(true);
			m_wSizeBox.SetHeightOverride(544);
		}
		if (m_wScroll)
			m_wScroll.SetDefaultStyle();	// apply the native scrollbar style.
		m_btnCog       = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioCog"));
		m_btnClose     = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioClose"));
		Widget grip = root.FindAnyWidget("DCO_ScenarioDrag");
		if (grip)
			grip.SetVisible(false);
		grip = root.FindAnyWidget("DCO_ScenarioResize");
		if (grip)
			grip.SetVisible(false);

		if (m_btnCog)
			m_btnCog.AddHandler(m_Handler);
		if (m_btnClose)
			m_btnClose.AddHandler(m_Handler);
		if (m_btnPresetSelect)
			m_btnPresetSelect.AddHandler(m_Handler);
		if (m_btnPresetLoad)
			m_btnPresetLoad.AddHandler(m_Handler);
		if (m_btnPresetSave)
			m_btnPresetSave.AddHandler(m_Handler);
		for (int presetIndex = 0; presetIndex < PRESET_SLOT_COUNT; presetIndex++)
		{
			ButtonWidget slotButton = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetSlot" + presetIndex.ToString()));
			TextWidget slotLabel = TextWidget.Cast(root.FindAnyWidget("DCO_ScenarioPresetSlot" + presetIndex.ToString() + "_Label"));
			m_PresetSlotBtns.Insert(slotButton);
			m_PresetSlotLabels.Insert(slotLabel);
			if (slotButton)
				slotButton.AddHandler(m_Handler);
		}
		EnsurePresetStore();
		UpdatePresetControls();

		// Category navigation is layout-defined so it uses the same proven input path as the rest of the GM shell.
		for (int i = 0; i < 8; i++)
		{
			ButtonWidget categoryButton = ButtonWidget.Cast(root.FindAnyWidget("DCO_ScenarioCat" + i.ToString()));
			TextWidget categoryLabel = TextWidget.Cast(root.FindAnyWidget("DCO_ScenarioCat" + i.ToString() + "_Label"));
			m_CatTabBtns.Insert(categoryButton);
			m_CatTabLabels.Insert(categoryLabel);
			if (!categoryButton)
				continue;

			DCO_ScenarioCategoryHandler categoryHandler = new DCO_ScenarioCategoryHandler(this, i);
			categoryButton.AddHandler(categoryHandler);
			categoryButton.SetVisible(false);
			m_CatHandlers.Insert(categoryHandler);
		}

		// Wheel-scroll: attach to the scroll widget + the panel root so the wheel works anywhere over the panel.
		if (m_wScroll)
		{
			m_ScrollHandler = new DCO_ScenarioScrollHandler(m_wScroll);
			m_wScroll.AddHandler(m_ScrollHandler);
			if (m_wContent)
				m_wContent.AddHandler(m_ScrollHandler);
			if (m_wPanel)
				m_wPanel.AddHandler(m_ScrollHandler);
		}

		if (m_wPanel)
			m_wPanel.SetVisible(false);	// hidden until the cog is clicked or an entity edit-properties fires.
		if (m_wBackdrop)
		{
			m_BackdropHandler = new DCO_ScenarioBackdropHandler();
			m_wBackdrop.AddHandler(m_BackdropHandler);
			m_wBackdrop.SetVisible(false);
		}
		if (m_wPresetBar)
			m_wPresetBar.SetVisible(false);
		if (m_wPresetMenu)
			m_wPresetMenu.SetVisible(false);
		m_bOpen = false;

		EnsureSubscribed();
	}

	bool ShowOptionPicker(DCO_ScenarioOptionRow row, Widget anchor)
	{
		if (!m_Menu || !row || !anchor || row.GetOptionCount() <= 0)
			return false;
		m_OptionPickerRow = row;
		m_OptionPickerAnchor = anchor;
		m_OptionPickerPage = 0;
		ShowOptionPickerPage();
		return true;
	}

	protected int OptionPickerPageCount()
	{
		if (!m_OptionPickerRow)
			return 1;
		return Math.Max(1, (m_OptionPickerRow.GetOptionCount() + OPTION_PAGE_SIZE - 1) / OPTION_PAGE_SIZE);
	}

	protected void ShowOptionPickerPage()
	{
		if (!m_Menu || !m_OptionPickerRow || !m_OptionPickerAnchor)
			return;
		array<string> labels = {};
		array<int> ids = {};
		int first = m_OptionPickerPage * OPTION_PAGE_SIZE;
		int end = Math.Min(first + OPTION_PAGE_SIZE, m_OptionPickerRow.GetOptionCount());
		for (int i = first; i < end; i++)
		{
			labels.Insert(m_OptionPickerRow.GetOptionName(i));
			ids.Insert(i);
		}
		int pageCount = OptionPickerPageCount();
		if (pageCount > 1)
		{
			labels.Insert(string.Format("PREVIOUS  ·  %1/%2", m_OptionPickerPage + 1, pageCount));
			ids.Insert(OPTION_PREVIOUS);
			labels.Insert(string.Format("NEXT  ·  %1/%2", m_OptionPickerPage + 1, pageCount));
			ids.Insert(OPTION_NEXT);
		}
		m_Menu.ShowAdjacent(labels, ids, m_OptionPickerAnchor, m_wPanel, "SELECT VALUE", m_OptionMenuCallback, null);
	}

	protected void OnOptionPickerAction(int actionId, SCR_EditableEntityComponent entity)
	{
		if (!m_OptionPickerRow)
			return;
		if (actionId == OPTION_PREVIOUS)
		{
			m_OptionPickerPage--;
			if (m_OptionPickerPage < 0)
				m_OptionPickerPage = OptionPickerPageCount() - 1;
			ShowOptionPickerPage();
			return;
		}
		if (actionId == OPTION_NEXT)
		{
			m_OptionPickerPage = (m_OptionPickerPage + 1) % OptionPickerPageCount();
			ShowOptionPickerPage();
			return;
		}
		m_OptionPickerRow.SelectOption(actionId);
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnCog)
		{
			if (m_bOpen)
				RequestClose();
			else
				SetOpen(true);
			return true;
		}
		if (w == m_btnClose)
		{
			RequestClose();
			return true;
		}
		if (w == m_btnPresetSelect && m_bCogSession)
		{
			if (m_wPresetMenu)
				m_wPresetMenu.SetVisible(!m_wPresetMenu.IsVisible());
			return true;
		}
		if (w == m_btnPresetLoad && m_bCogSession)
		{
			LoadSelectedPreset();
			return true;
		}
		if (w == m_btnPresetSave && m_bCogSession)
		{
			SaveSelectedPreset();
			return true;
		}
		for (int presetIndex = 0; presetIndex < m_PresetSlotBtns.Count(); presetIndex++)
		{
			if (w != m_PresetSlotBtns[presetIndex])
				continue;
			m_iPresetSlot = presetIndex;
			if (m_wPresetMenu)
				m_wPresetMenu.SetVisible(false);
			UpdatePresetControls();
			return true;
		}
		return false;
	}

	bool SelectCategory(int index)
	{
		if (!m_bOpen || !m_bEditing || !m_aSessionAttributes)
			return false;
		if (index < 0 || index >= m_CatTabBtns.Count())
			return false;

		int pageSize = CategoryPageSize();
		if (m_CategoryCount > m_CatTabBtns.Count() && index == m_CatTabBtns.Count() - 1)
		{
			m_CategoryPage = (m_CategoryPage + 1) % CategoryPageCount();
			m_ActiveCat = m_CategoryPage * pageSize;
			QueueCategoryRefresh();
			return true;
		}

		int categoryIndex = m_CategoryPage * pageSize + index;
		if (categoryIndex < 0 || categoryIndex >= m_CategoryCount)
			return false;
		if (m_ActiveCat == categoryIndex)
			return true;

		m_ActiveCat = categoryIndex;
		QueueCategoryRefresh();
		return true;
	}

	protected void QueueCategoryRefresh()
	{
		if (m_bCategoryRefreshQueued)
			return;
		m_bCategoryRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RenderSelectedCategory, 0, false);
	}

	protected int CategoryPageSize()
	{
		if (m_CategoryCount > m_CatTabBtns.Count())
			return m_CatTabBtns.Count() - 1;
		return m_CatTabBtns.Count();
	}

	protected int CategoryPageCount()
	{
		int pageSize = CategoryPageSize();
		if (pageSize <= 0)
			return 1;
		return Math.Max(1, (m_CategoryCount + pageSize - 1) / pageSize);
	}

	// Entry point used by the live 24-hour clock in the top bar.
	void OpenTimeAndDate()
	{
		SetOpen(true);
		m_TimeDateOpenAttempts = 0;
		GetGame().GetCallqueue().CallLater(SelectTimeAndDateCategory, 0, false);
	}

	protected void SelectTimeAndDateCategory()
	{
		m_TimeDateOpenAttempts++;
		for (int i = 0; i < m_CatTabLabels.Count(); i++)
		{
			TextWidget label = m_CatTabLabels[i];
			if (!label)
				continue;
			string text = label.GetText();
			text.ToLower();
			if (text.Contains("time and date"))
			{
				SelectCategory(i);
				return;
			}
		}
		if (m_TimeDateOpenAttempts < 10)
		{
			if (m_CategoryCount > m_CatTabBtns.Count())
				SelectCategory(m_CatTabBtns.Count() - 1);
			GetGame().GetCallqueue().CallLater(SelectTimeAndDateCategory, 100, false);
		}
	}

	protected void RenderSelectedCategory()
	{
		m_bCategoryRefreshQueued = false;
		if (!m_bOpen || !m_bEditing || !m_aSessionAttributes)
			return;

		RenderAttributes(m_aSessionAttributes, true);
	}

	bool CloseForBack()
	{
		if (!m_bOpen)
			return false;
		if (m_wPresetMenu && m_wPresetMenu.IsVisible())
		{
			m_wPresetMenu.SetVisible(false);
			return true;
		}
		RequestClose();
		return true;
	}

	protected void RequestClose()
	{
		foreach (DCO_ScenarioOptionRow row : m_Rows)
		{
			if (row)
				row.CommitPendingEdits();
		}

		// A trigger cannot be applied accidentally from the middle of the wizard.
		// The first close request moves to the live review; the second confirms it.
		if (m_bTriggerSession && m_iTriggerFinalizeCategory >= 0 && m_ActiveCat != m_iTriggerFinalizeCategory)
		{
			m_ActiveCat = m_iTriggerFinalizeCategory;
			m_CategoryPage = 0;
			RenderAttributes(m_aSessionAttributes, true);
			return;
		}
		SetOpen(false);
	}

	// The native dialog remains the compatibility fallback for attribute layouts
	// Bifrost cannot faithfully render. Fully supported sessions are handed off
	// after the native menu completes its own opening lifecycle.
	bool CanOwnPropertySession()
	{
		if (!m_bOpen || !m_bEditing || !m_aSessionAttributes || m_aSessionAttributes.IsEmpty())
			return false;
		foreach (SCR_BaseEditorAttribute attribute : m_aSessionAttributes)
		{
			if (attribute && !SupportsLayout(attribute))
				return false;
		}
		return true;
	}

	protected void SetOpen(bool open)
	{
		bool wasOpen = m_bOpen;
		if (!open && wasOpen)
			DCO_GMUIController.ReleaseMenuFocus();
		m_bOpen = open;
		if (m_wBackdrop)
			m_wBackdrop.SetVisible(open);
		if (m_wPanel)
			m_wPanel.SetVisible(open);

		if (open)
		{
			m_bTriggerSession = false;
			m_iTriggerFinalizeCategory = -1;
			DCO_GMUIController.SetPropertyOverlaysSuppressed(false);
			m_bCogSession = true;	// cog => the GLOBAL/scenario set.
			if (m_wTitle)
				m_wTitle.SetText("SCENARIO SETTINGS");
			ApplyDefaultGeometry();	// always open at the known-good size/pos.
			UpdatePresetControls();
			BeginEditing();
		}
		else
		{
			if (m_wPresetMenu)
				m_wPresetMenu.SetVisible(false);
			foreach (DCO_ScenarioOptionRow row : m_Rows)
			{
				if (row)
					row.CommitPendingEdits();
			}
			EndEditing(true);
		}
	}

	protected void ApplyDefaultGeometry()
	{
		if (!m_wPanel)
			return;
		float panelWidth = 980;
		float panelHeight = 760;
		if (m_bTriggerSession)
		{
			panelWidth = 1120;
			panelHeight = 860;
		}
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			float viewportWidth = workspace.DPIUnscale(workspace.GetWidth());
			float viewportHeight = workspace.DPIUnscale(workspace.GetHeight());
			if (viewportWidth > 0)
				panelWidth = Math.Min(panelWidth, Math.Max(640, viewportWidth - 32));
			if (viewportHeight > 0)
				panelHeight = Math.Min(panelHeight, Math.Max(560, viewportHeight - 32));
		}
		FrameSlot.SetAnchor(m_wPanel, 0.5, 0.5);
		FrameSlot.SetAlignment(m_wPanel, 0.5, 0.5);
		FrameSlot.SetSize(m_wPanel, panelWidth, panelHeight);
		FrameSlot.SetPos(m_wPanel, 0, 0);
		if (m_wSizeBox)
		{
			m_wSizeBox.EnableHeightOverride(true);
			m_wSizeBox.SetHeightOverride(Math.Max(360, panelHeight - 216));
		}
	}

	protected SCR_AttributesManagerEditorComponent GetManager()
	{
		if (!m_Manager)
			m_Manager = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		return m_Manager;
	}

	protected void EnsureSubscribed()
	{
		if (m_bSubscribed)
			return;
		SCR_AttributesManagerEditorComponent mgr = GetManager();
		if (!mgr)
			return;
		mgr.GetOnAttributesStart().Insert(OnAttributesStart);
		m_bSubscribed = true;
	}

	protected void BeginEditing()
	{
		SCR_AttributesManagerEditorComponent mgr = GetManager();
		if (!mgr)
		{
			Print("[DCO-GM] scenario: attributes manager unavailable", LogLevel.WARNING);
			return;
		}
		EnsureSubscribed();
		m_ActiveCat = 0;
		m_CategoryPage = 0;
		m_aSessionAttributes = null;
		m_bEditing = true;
		mgr.StartEditing(GetGame().GetGameMode());	// game-mode entity = the GLOBAL/scenario attribute set.
	}

	protected void EndEditing(bool confirm)
	{
		SCR_AttributesManagerEditorComponent mgr = GetManager();
		bool wasEditing = m_bEditing;

		// Mark the old session closed before asking the manager to finish it.
		m_bEditing = false;
		m_bCogSession = false;
		UpdatePresetControls();
		if (mgr && wasEditing)
		{
			if (confirm)
				mgr.ConfirmEditing();
			else
				mgr.CancelEditing();
		}

		if (!m_bEditing)
		{
			ClearContent();
			m_aSessionAttributes = null;
		}
		DCO_GMUIController.SetPropertyOverlaysSuppressed(false);
		m_bTriggerSession = false;
		m_iTriggerFinalizeCategory = -1;
		UpdateTriggerChrome();
	}

	protected void OnAttributesStart(array<SCR_BaseEditorAttribute> attributes)
	{
		// The stock manager asks the server for the authoritative list before this
		// callback. If even one layout is outside Bifrost's renderer, keep the
		// native dialog as the sole owner instead of exposing a partial duplicate.
		if (!CanRenderAttributeSession(attributes))
		{
			DCO_GMUIController.SetPropertyOverlaysSuppressed(false);
			m_bTriggerSession = false;
			m_iTriggerFinalizeCategory = -1;
			m_bOpen = false;
			m_bEditing = false;
			m_bCogSession = false;
			m_aSessionAttributes = null;
			if (m_wBackdrop)
				m_wBackdrop.SetVisible(false);
			if (m_wPanel)
				m_wPanel.SetVisible(false);
			ClearContent();
			return;
		}

		bool newSession = !m_bEditing || !m_aSessionAttributes;
		if (newSession)
		{
			m_ActiveCat = 0;
			m_CategoryPage = 0;
		}
		m_bTriggerSession = IsTriggerAttributeSession(attributes);
		DCO_GMUIController.SetPropertyOverlaysSuppressed(m_bTriggerSession);
		m_bEditing = true;
		if (!m_bOpen)
		{
			m_bOpen = true;
			m_bEditing = true;
			if (m_wTitle && !m_bCogSession)
				m_wTitle.SetText("PROPERTIES");
			if (m_wPanel)
				m_wPanel.SetVisible(true);
			if (m_wBackdrop)
				m_wBackdrop.SetVisible(true);
			ApplyDefaultGeometry();
		}
		UpdatePresetControls();
		// The callback owns the authoritative list for this new editing session.
		RenderAttributes(attributes);
	}

	protected bool IsTriggerAttributeSession(array<SCR_BaseEditorAttribute> attributes)
	{
		if (!attributes)
			return false;
		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			if (DCO_TriggerAttributeBase.Cast(attribute) || DCO_TriggerEnabledEditorAttribute.Cast(attribute))
				return true;
		}
		return false;
	}

	protected bool CanRenderAttributeSession(array<SCR_BaseEditorAttribute> attributes)
	{
		if (!attributes || attributes.IsEmpty())
			return false;
		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			if (attribute && !SupportsLayout(attribute))
			{
				Print(string.Format("[DCO-GM] native properties retained for unsupported layout: %1", attribute.GetLayout()), LogLevel.WARNING);
				return false;
			}
		}
		return true;
	}

	protected void RenderAttributes(array<SCR_BaseEditorAttribute> sessionAttributes, bool resetScroll = true)
	{
		if (!m_wContent)
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		SCR_AttributesManagerEditorComponent mgr = GetManager();
		if (!workspace || !mgr)
			return;

		float savedScrollX = 0;
		float savedScrollY = 0;
		if (!resetScroll && m_wScroll)
			m_wScroll.GetSliderPos(savedScrollX, savedScrollY);

		// A ScrollLayout keeps its content offset when its children are deleted.
		if (resetScroll)
			ResetScrollToTop();
		ClearContent();

		// Keep a complete session snapshot.
		array<SCR_BaseEditorAttribute> attributes = {};
		if (sessionAttributes)
		{
			foreach (SCR_BaseEditorAttribute sessionAttribute : sessionAttributes)
				attributes.Insert(sessionAttribute);
		}
		else
		{
			mgr.GetEditedAttributes(attributes);
		}
		m_aSessionAttributes = attributes;
		UpdatePresetControls();
		UpdateConditionalState(attributes);

		array<ResourceName> categoryConfigs = {};
		array<ref SCR_EditorAttributeCategory> categories = {};
		BuildOrderedCategories(attributes, categoryConfigs, categories);
		if (m_bTriggerSession)
		{
			m_iTriggerFinalizeCategory = categoryConfigs.Find(TRIGGER_FINALIZE_CATEGORY);
			UpdateTriggerReviewSummaries(attributes);
		}

		bool hasOther = false;
		foreach (SCR_BaseEditorAttribute ua : attributes)
		{
			if (ua && categoryConfigs.Find(ua.GetCategoryConfig()) == -1 && ShouldRenderAttribute(ua))
			{
				hasOther = true;
				break;
			}
		}
		int tabCount = categoryConfigs.Count();
		if (hasOther)
			tabCount++;
		if (m_ActiveCat >= tabCount)
			m_ActiveCat = 0;
		if (m_bTriggerSession && m_ActiveCat < categoryConfigs.Count())
		{
			ResourceName activeTriggerCategory = categoryConfigs[m_ActiveCat];
			m_sTriggerActiveStep = "SETUP";
			if (activeTriggerCategory == TRIGGER_UNITS_CATEGORY)
				m_sTriggerActiveStep = "UNITS";
			else if (activeTriggerCategory == TRIGGER_FINALIZE_CATEGORY)
				m_sTriggerActiveStep = "FINALIZE";
		}

		Color accent = DCO_GMTheme.Get().m_AccentColor;
		UpdateCategoryNavigation(categoryConfigs, categories, hasOther, accent);
		UpdateTriggerChrome();

		if (m_ActiveCat < categoryConfigs.Count())
		{
			ResourceName activeCfg = categoryConfigs[m_ActiveCat];
			foreach (SCR_BaseEditorAttribute attribute : attributes)
			{
				if (!attribute || attribute.GetCategoryConfig() != activeCfg)
					continue;
				if (!ShouldRenderAttribute(attribute))
					continue;
				RenderOneAttribute(workspace, attribute);
			}
		}
		else
		{
			foreach (SCR_BaseEditorAttribute attribute : attributes)
			{
				if (!attribute || categoryConfigs.Find(attribute.GetCategoryConfig()) != -1)
					continue;
				if (!ShouldRenderAttribute(attribute))
					continue;
				RenderOneAttribute(workspace, attribute);
			}
		}

		if (resetScroll)
		{
			ResetScrollToTop();
			GetGame().GetCallqueue().CallLater(ResetScrollToTop, 0, false);	// repeat after the rebuilt layout has measured itself.
		}
		else
		{
			RestoreScroll(savedScrollX, savedScrollY);
			GetGame().GetCallqueue().CallLater(RestoreScroll, 0, false, savedScrollX, savedScrollY);
		}

	}

	// Refresh the persistent settings-header buttons.
	protected void UpdateCategoryNavigation(array<ResourceName> categoryConfigs, array<ref SCR_EditorAttributeCategory> categories, bool hasOther, Color accent)
	{
		int tabCount = categoryConfigs.Count();
		if (hasOther)
			tabCount++;
		m_CategoryCount = tabCount;
		m_CategoryPage = Math.Clamp(m_CategoryPage, 0, CategoryPageCount() - 1);
		int pageSize = CategoryPageSize();
		int pageCount = CategoryPageCount();

		for (int i = 0; i < m_CatTabBtns.Count(); i++)
		{
			ButtonWidget b = m_CatTabBtns[i];
			TextWidget t = m_CatTabLabels[i];
			bool more = m_CategoryCount > m_CatTabBtns.Count() && i == m_CatTabBtns.Count() - 1;
			int categoryIndex = m_CategoryPage * pageSize + i;
			bool visible = more || categoryIndex < m_CategoryCount;
			if (b)
				b.SetVisible(visible);
			if (!visible || !t)
				continue;

			if (more)
			{
				t.SetText(string.Format("MORE %1/%2", m_CategoryPage + 1, pageCount));
				t.SetExactFontSize(13);
				t.SetColor(DCO_GMTheme.Get().m_AccentColor);
				continue;
			}

			string name = "OTHER";
			if (categoryIndex < categoryConfigs.Count())
			{
				name = "ATTRIBUTES";
				SCR_UIInfo info = categories[categoryIndex].GetInfo();
				if (info)
					name = info.GetName();
			}
			t.SetText(name);
			if (categoryIndex == m_ActiveCat)
			{
				t.SetExactFontSize(14);
				if (accent)
					t.SetColor(accent);
			}
			else
			{
				t.SetExactFontSize(13);
				t.SetColor(DCO_GMTheme.Get().m_MutedColor);
			}
		}
	}

	protected void ResetScrollToTop()
	{
		if (m_wScroll)
			m_wScroll.SetSliderPos(0, 0);
	}

	protected void RestoreScroll(float x, float y)
	{
		if (m_wScroll)
			m_wScroll.SetSliderPos(x, y);
	}

	protected void UpdateConditionalState(notnull array<SCR_BaseEditorAttribute> attributes)
	{
		m_bHasContinuousFire = false;
		m_bContinuousFire = false;
		m_iTriggerAction = EDCO_TriggerAction.NOTIFY;
		m_iTriggerTimerMode = EDCO_TriggerTimerMode.IMMEDIATE;
		m_iTriggerOwnerMode = EDCO_TriggerOwnerMode.AREA_FILTER;
		m_iTriggerLinkedUnitMode = EDCO_TriggerLinkedUnitMode.LINK_ONLY;
		m_bTriggerRepeat = false;
		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			DCO_FxContinuousFireEditorAttribute continuous = DCO_FxContinuousFireEditorAttribute.Cast(attribute);
			if (continuous)
			{
				SCR_BaseEditorAttributeVar continuousValue = continuous.GetVariable();
				if (continuousValue)
				{
					m_bHasContinuousFire = true;
					m_bContinuousFire = continuousValue.GetBool();
				}
			}
			if (!m_bTriggerSession)
				continue;
			SCR_BaseEditorAttributeVar value = attribute.GetVariableOrCopy();
			if (!value)
				continue;
			if (DCO_TriggerActionEditorAttribute.Cast(attribute))
				m_iTriggerAction = value.GetInt();
			else if (DCO_TriggerTimerModeEditorAttribute.Cast(attribute))
				m_iTriggerTimerMode = value.GetInt();
			else if (DCO_TriggerOwnerModeEditorAttribute.Cast(attribute))
				m_iTriggerOwnerMode = value.GetInt();
			else if (DCO_TriggerLinkedUnitModeEditorAttribute.Cast(attribute))
				m_iTriggerLinkedUnitMode = value.GetInt();
			else if (DCO_TriggerRepeatEditorAttribute.Cast(attribute))
				m_bTriggerRepeat = value.GetBool();
		}
	}

	protected bool ShouldRenderAttribute(SCR_BaseEditorAttribute attribute)
	{
		if (!SupportsLayout(attribute))
			return false;
		if (m_bHasContinuousFire && m_bContinuousFire && DCO_FxExplosionGunrunRoundsEditorAttribute.Cast(attribute))
			return false;
		if (!m_bTriggerSession)
			return true;
		if ((DCO_TriggerTimerMinEditorAttribute.Cast(attribute) || DCO_TriggerTimerMidEditorAttribute.Cast(attribute)
			|| DCO_TriggerTimerMaxEditorAttribute.Cast(attribute)) && m_iTriggerTimerMode == EDCO_TriggerTimerMode.IMMEDIATE)
			return false;
		if (DCO_TriggerCooldownEditorAttribute.Cast(attribute) && !m_bTriggerRepeat)
			return false;
		if ((DCO_TriggerConditionEditorAttribute.Cast(attribute) || DCO_TriggerCountEditorAttribute.Cast(attribute))
			&& m_iTriggerOwnerMode != EDCO_TriggerOwnerMode.AREA_FILTER)
			return false;
		if ((DCO_TriggerSpawnFactionEditorAttribute.Cast(attribute) || DCO_TriggerSpawnGroupEditorAttribute.Cast(attribute))
			&& m_iTriggerAction != EDCO_TriggerAction.SPAWN_GROUP)
			return false;
		if (DCO_TriggerPairIdEditorAttribute.Cast(attribute) && m_iTriggerAction != EDCO_TriggerAction.SPRING_AMBUSH)
			return false;
		if (DCO_TriggerFxRadiusEditorAttribute.Cast(attribute) && m_iTriggerAction != EDCO_TriggerAction.FIRE_FX)
			return false;
		return true;
	}

	protected string GetAttributeEntryLabel(SCR_BaseEditorAttribute attribute, int selected, string fallback)
	{
		if (!attribute)
			return fallback;
		array<ref SCR_BaseEditorAttributeEntry> entries = {};
		attribute.GetEntries(entries);
		int index;
		foreach (SCR_BaseEditorAttributeEntry entry : entries)
		{
			SCR_BaseEditorAttributeEntryText textEntry = SCR_BaseEditorAttributeEntryText.Cast(entry);
			if (textEntry)
			{
				if (index == selected)
					return textEntry.GetText();
				index++;
				continue;
			}
			SCR_BaseEditorAttributeEntryUIInfo infoEntry = SCR_BaseEditorAttributeEntryUIInfo.Cast(entry);
			if (infoEntry && infoEntry.GetInfo())
			{
				if (index == selected)
					return infoEntry.GetInfo().GetName();
				index++;
			}
		}
		return fallback;
	}

	protected void UpdateTriggerReviewSummaries(notnull array<SCR_BaseEditorAttribute> attributes)
	{
		int shape;
		int activation;
		int ownerMode;
		int condition;
		int action;
		int timerMode;
		int linkedMode;
		int spawnGroup;
		float radius = 25;
		float radiusZ = 25;
		float height;
		float count = 1;
		float cooldown = 10;
		float checkInterval = 2;
		float timerMin;
		float timerMid;
		float timerMax;
		float pairId;
		float fxPairRadius = 50;
		bool repeat;
		SCR_BaseEditorAttribute shapeAttribute;
		SCR_BaseEditorAttribute activationAttribute;
		SCR_BaseEditorAttribute ownerAttribute;
		SCR_BaseEditorAttribute conditionAttribute;
		SCR_BaseEditorAttribute actionAttribute;
		SCR_BaseEditorAttribute timerAttribute;
		SCR_BaseEditorAttribute linkedModeAttribute;
		SCR_BaseEditorAttribute spawnGroupAttribute;
		DCO_TriggerReviewAreaEditorAttribute areaReview;
		DCO_TriggerReviewActivationEditorAttribute activationReview;
		DCO_TriggerReviewResponseEditorAttribute responseReview;

		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			if (!attribute)
				continue;
			SCR_BaseEditorAttributeVar value = attribute.GetVariableOrCopy();
			if (DCO_TriggerReviewAreaEditorAttribute.Cast(attribute))
			{
				areaReview = DCO_TriggerReviewAreaEditorAttribute.Cast(attribute);
				continue;
			}
			if (DCO_TriggerReviewActivationEditorAttribute.Cast(attribute))
			{
				activationReview = DCO_TriggerReviewActivationEditorAttribute.Cast(attribute);
				continue;
			}
			if (DCO_TriggerReviewResponseEditorAttribute.Cast(attribute))
			{
				responseReview = DCO_TriggerReviewResponseEditorAttribute.Cast(attribute);
				if (value)
					m_iTriggerLinkedCount = Math.Max(0, value.GetInt());
				continue;
			}
			if (!value)
				continue;
			if (DCO_TriggerShapeEditorAttribute.Cast(attribute))
			{
				shapeAttribute = attribute;
				shape = value.GetInt();
			}
			else if (DCO_TriggerRadiusEditorAttribute.Cast(attribute))
				radius = value.GetFloat();
			else if (DCO_TriggerRadiusZEditorAttribute.Cast(attribute))
				radiusZ = value.GetFloat();
			else if (DCO_TriggerHeightEditorAttribute.Cast(attribute))
				height = value.GetFloat();
			else if (DCO_TriggerIntervalEditorAttribute.Cast(attribute))
				checkInterval = value.GetFloat();
			else if (DCO_TriggerTimerModeEditorAttribute.Cast(attribute))
			{
				timerAttribute = attribute;
				timerMode = value.GetInt();
			}
			else if (DCO_TriggerTimerMaxEditorAttribute.Cast(attribute))
				timerMax = value.GetFloat();
			else if (DCO_TriggerTimerMidEditorAttribute.Cast(attribute))
				timerMid = value.GetFloat();
			else if (DCO_TriggerTimerMinEditorAttribute.Cast(attribute))
				timerMin = value.GetFloat();
			else if (DCO_TriggerActivationEditorAttribute.Cast(attribute))
			{
				activationAttribute = attribute;
				activation = value.GetInt();
			}
			else if (DCO_TriggerOwnerModeEditorAttribute.Cast(attribute))
			{
				ownerAttribute = attribute;
				ownerMode = value.GetInt();
			}
			else if (DCO_TriggerConditionEditorAttribute.Cast(attribute))
			{
				conditionAttribute = attribute;
				condition = value.GetInt();
			}
			else if (DCO_TriggerCountEditorAttribute.Cast(attribute))
				count = value.GetFloat();
			else if (DCO_TriggerRepeatEditorAttribute.Cast(attribute))
				repeat = value.GetBool();
			else if (DCO_TriggerCooldownEditorAttribute.Cast(attribute))
				cooldown = value.GetFloat();
			else if (DCO_TriggerActionEditorAttribute.Cast(attribute))
			{
				actionAttribute = attribute;
				action = value.GetInt();
			}
			else if (DCO_TriggerLinkedUnitModeEditorAttribute.Cast(attribute))
			{
				linkedModeAttribute = attribute;
				linkedMode = value.GetInt();
			}
			else if (DCO_TriggerSpawnGroupEditorAttribute.Cast(attribute))
			{
				spawnGroupAttribute = attribute;
				spawnGroup = value.GetInt();
			}
			else if (DCO_TriggerPairIdEditorAttribute.Cast(attribute))
				pairId = value.GetFloat();
			else if (DCO_TriggerFxRadiusEditorAttribute.Cast(attribute))
				fxPairRadius = value.GetFloat();
		}

		string shapeName = GetAttributeEntryLabel(shapeAttribute, shape, "Ellipse");
		string heightName = "unlimited height";
		if (height > 0)
			heightName = height.ToString(-1, 0) + " m high";
		string timingName = GetAttributeEntryLabel(timerAttribute, timerMode, "Immediate");
		if (timerMode != EDCO_TriggerTimerMode.IMMEDIATE)
			timingName += string.Format(" %1/%2/%3 s", timerMin.ToString(-1, 0), timerMid.ToString(-1, 0), timerMax.ToString(-1, 0));
		if (areaReview)
			areaReview.DCO_SetSummary(string.Format("%1 · %2 x %3 m · %4 · %5 · checks %6 s", shapeName,
				(radius * 2).ToString(-1, 0), (radiusZ * 2).ToString(-1, 0), heightName, timingName, checkInterval.ToString(-1, 1)));

		string ownerName = GetAttributeEntryLabel(ownerAttribute, ownerMode, "Area filter");
		string activationName = GetAttributeEntryLabel(activationAttribute, activation, "Present");
		string subjectName = GetAttributeEntryLabel(conditionAttribute, condition, "Anyone");
		string repeatName = "Once";
		if (repeat)
			repeatName = "Repeat · " + cooldown.ToString(-1, 0) + " s re-arm";
		if (activationReview)
		{
			if (ownerMode == EDCO_TriggerOwnerMode.AREA_FILTER)
				activationReview.DCO_SetSummary(string.Format("%1 · %2 · %3 required · %4", subjectName, activationName, count.ToString(-1, 0), repeatName));
			else
				activationReview.DCO_SetSummary(string.Format("%1 · %2 · %3", ownerName, activationName, repeatName));
		}

		string actionName = GetAttributeEntryLabel(actionAttribute, action, "Notify everyone");
		if (action == EDCO_TriggerAction.SPAWN_GROUP)
			actionName = "Spawn: " + GetAttributeEntryLabel(spawnGroupAttribute, spawnGroup, "selected group");
		else if (action == EDCO_TriggerAction.SPRING_AMBUSH)
			actionName += " · pair " + pairId.ToString(-1, 0);
		else if (action == EDCO_TriggerAction.FIRE_FX)
			actionName += " · " + fxPairRadius.ToString(-1, 0) + " m pair range";
		string linkedModeName = GetAttributeEntryLabel(linkedModeAttribute, linkedMode, "Linked only");
		string linkedCountName = m_iTriggerLinkedCount.ToString() + " linked group";
		if (m_iTriggerLinkedCount != 1)
			linkedCountName += "s";
		if (responseReview)
			responseReview.DCO_SetSummary(string.Format("%1 · %2 · %3", actionName, linkedCountName, linkedModeName));
	}

	protected void UpdateTriggerChrome()
	{
		if (m_bTriggerSession)
		{
			if (m_wTitle)
				m_wTitle.SetText("TRIGGER SETUP · " + m_sTriggerActiveStep);
			if (m_wCloseLabel)
			{
				if (m_iTriggerFinalizeCategory >= 0 && m_ActiveCat == m_iTriggerFinalizeCategory)
					m_wCloseLabel.SetText("APPLY & CLOSE");
				else
					m_wCloseLabel.SetText("REVIEW & FINALIZE");
			}
			return;
		}
		if (m_wTitle)
		{
			if (m_bCogSession)
				m_wTitle.SetText("SCENARIO SETTINGS");
			else
				m_wTitle.SetText("PROPERTIES");
		}
		if (m_wCloseLabel)
			m_wCloseLabel.SetText("CLOSE");
	}

	protected bool SupportsLayout(SCR_BaseEditorAttribute attribute)
	{
		if (!attribute)
			return false;
		// These native custom layouts are still single-value preset selectors.
		if (SCR_TimePresetsEditorAttribute.Cast(attribute) || SCR_GameOverTypeEditorAttribute.Cast(attribute))
			return true;
		string layout = attribute.GetLayout();
		return layout.Contains("Checkbox") || layout.Contains("MultiSelection") || layout.Contains("Slider")
			|| layout.Contains("Date.layout") || layout.Contains("ButtonBox_Selection") || layout.Contains("Spinbox");
	}

	protected void BuildOrderedCategories(notnull array<SCR_BaseEditorAttribute> attributes, notnull array<ResourceName> categoryConfigs, notnull array<ref SCR_EditorAttributeCategory> categories)
	{
		array<int> priorities = {};
		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			if (!attribute || !ShouldRenderAttribute(attribute))
				continue;
			ResourceName categoryConfig = attribute.GetCategoryConfig();
			if (categoryConfigs.Find(categoryConfig) != -1)
				continue;

			Resource categoryContainer = BaseContainerTools.LoadContainer(categoryConfig);
			if (!categoryContainer)
				continue;
			SCR_EditorAttributeCategory category = SCR_EditorAttributeCategory.Cast(BaseContainerTools.CreateInstanceFromContainer(categoryContainer.GetResource().ToBaseContainer()));
			if (!category)
				continue;

			int priority = category.GetPriority();
			int index = 0;
			int count = categoryConfigs.Count();
			bool inserted = false;
			for (int c = 0; c < count; c++)
			{
				if (priority > priorities[c])
				{
					index = c;
					inserted = true;
					break;
				}
			}
			if (inserted)
			{
				categoryConfigs.InsertAt(categoryConfig, index);
				categories.InsertAt(category, index);
				priorities.InsertAt(priority, index);
			}
			else
			{
				categoryConfigs.Insert(categoryConfig);
				categories.Insert(category);
				priorities.Insert(priority);
			}
		}
	}

	// Create one Bifrost-owned row.
	protected bool RenderOneAttribute(WorkspaceWidget workspace, SCR_BaseEditorAttribute attribute)
	{
		Widget attributeWidget = workspace.CreateWidgets(OPTION_LAYOUT, m_wContent);
		if (!attributeWidget)
		{
			Print("[DCO-GM] scenario: failed to create Bifrost option row", LogLevel.WARNING);
			return false;
		}

		DCO_ScenarioOptionRow row = new DCO_ScenarioOptionRow();
		if (!row.Init(attributeWidget, attribute, GetManager(), this))
		{
			row.Shutdown();
			delete attributeWidget;
			Print("[DCO-GM] scenario: incomplete Bifrost option row", LogLevel.WARNING);
			return false;
		}
		m_Rows.Insert(row);
		return true;
	}

	void OnAttributeChanged()
	{
		if (m_bConditionalRefreshQueued)
			return;
		m_bConditionalRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshConditionalRows, 0, false);
	}

	protected void RefreshConditionalRows()
	{
		m_bConditionalRefreshQueued = false;
		if (!m_bOpen || !m_bEditing || !m_aSessionAttributes)
			return;

		UpdateConditionalState(m_aSessionAttributes);
		RenderAttributes(m_aSessionAttributes, false);
	}

	protected void EnsurePresetStore()
	{
		if (!m_PresetStore)
		{
			m_PresetStore = new DCO_ScenarioPresetStore();
			JsonLoadContext load = new JsonLoadContext();
			if (load.LoadFromFile(PRESET_FILE))
			{
				if (!load.ReadValue("", m_PresetStore) || !m_PresetStore)
				{
					Print("[DCO-GM] scenario preset store unreadable - starting empty", LogLevel.WARNING);
					m_PresetStore = new DCO_ScenarioPresetStore();
				}
			}
		}

		if (!m_PresetStore.m_aSlots)
			m_PresetStore.m_aSlots = {};
		while (m_PresetStore.m_aSlots.Count() < PRESET_SLOT_COUNT)
			m_PresetStore.m_aSlots.Insert(new DCO_ScenarioPresetSlot());
		for (int i = 0; i < PRESET_SLOT_COUNT; i++)
		{
			if (!m_PresetStore.m_aSlots[i])
				m_PresetStore.m_aSlots[i] = new DCO_ScenarioPresetSlot();
			if (!m_PresetStore.m_aSlots[i].m_aValues)
				m_PresetStore.m_aSlots[i].m_aValues = {};
		}
	}

	protected bool PersistPresetStore()
	{
		EnsurePresetStore();
		JsonSaveContext save = new JsonSaveContext();
		if (!save.WriteValue("", m_PresetStore) || !save.SaveToFile(PRESET_FILE))
		{
			Print("[DCO-GM] scenario preset SAVE FAILED", LogLevel.WARNING);
			return false;
		}
		return true;
	}

	protected void UpdatePresetControls()
	{
		EnsurePresetStore();
		bool visible = m_bOpen && m_bCogSession;
		if (m_wPresetBar)
			m_wPresetBar.SetVisible(visible);
		if (!visible && m_wPresetMenu)
			m_wPresetMenu.SetVisible(false);

		bool hasSession = visible && m_bEditing && m_aSessionAttributes && !m_aSessionAttributes.IsEmpty();
		DCO_ScenarioPresetSlot selected = m_PresetStore.m_aSlots[m_iPresetSlot];
		if (m_btnPresetLoad)
			m_btnPresetLoad.SetEnabled(hasSession && selected.m_bSaved);
		if (m_btnPresetSave)
			m_btnPresetSave.SetEnabled(hasSession);
		if (m_wPresetSelectLabel)
			m_wPresetSelectLabel.SetText("PRESET " + (m_iPresetSlot + 1).ToString());

		for (int i = 0; i < m_PresetSlotLabels.Count(); i++)
		{
			TextWidget label = m_PresetSlotLabels[i];
			if (!label)
				continue;
			string text = "SLOT " + (i + 1).ToString();
			if (!m_PresetStore.m_aSlots[i].m_bSaved)
				text += " (EMPTY)";
			label.SetText(text);
			if (i == m_iPresetSlot)
				label.SetColor(DCO_GMTheme.Get().m_AccentColor);
			else
				label.SetColor(DCO_GMTheme.Get().m_TextColor);
		}
	}

	protected int GetPresetKind(SCR_BaseEditorAttribute attribute)
	{
		string layout = attribute.GetLayout();
		if (layout.Contains("Checkbox"))
			return PRESET_BOOL;
		if (layout.Contains("MultiSelection") || layout.Contains("Date.layout"))
			return PRESET_VECTOR;
		if (layout.Contains("Slider"))
			return PRESET_FLOAT;
		return PRESET_INT;
	}

	protected string GetPresetKey(SCR_BaseEditorAttribute attribute, int attributeIndex)
	{
		string typeName = attribute.Type().ToString();
		int occurrence;
		for (int i = 0; i < attributeIndex; i++)
		{
			SCR_BaseEditorAttribute previous = m_aSessionAttributes[i];
			if (previous && previous.Type().ToString() == typeName)
				occurrence++;
		}
		return typeName + "#" + occurrence.ToString();
	}

	protected DCO_ScenarioPresetValue FindPresetValue(DCO_ScenarioPresetSlot slot, string key)
	{
		foreach (DCO_ScenarioPresetValue value : slot.m_aValues)
		{
			if (value && value.m_sKey == key)
				return value;
		}
		return null;
	}

	protected void SaveSelectedPreset()
	{
		if (!m_bCogSession || !m_bEditing || !m_aSessionAttributes)
			return;
		EnsurePresetStore();
		DCO_ScenarioPresetSlot slot = m_PresetStore.m_aSlots[m_iPresetSlot];
		slot.m_aValues.Clear();

		for (int i = 0; i < m_aSessionAttributes.Count(); i++)
		{
			SCR_BaseEditorAttribute attribute = m_aSessionAttributes[i];
			if (!attribute || !SupportsLayout(attribute))
				continue;
			SCR_BaseEditorAttributeVar var = attribute.GetVariableOrCopy();
			if (!var)
				continue;

			DCO_ScenarioPresetValue value = new DCO_ScenarioPresetValue();
			value.m_sKey = GetPresetKey(attribute, i);
			value.m_iKind = GetPresetKind(attribute);
			switch (value.m_iKind)
			{
				case PRESET_BOOL:   value.m_bBool = var.GetBool(); break;
				case PRESET_FLOAT:  value.m_fFloat = var.GetFloat(); break;
				case PRESET_VECTOR: value.m_vVector = var.GetVector(); break;
				default:            value.m_iInt = var.GetInt(); break;
			}
			slot.m_aValues.Insert(value);
		}

		slot.m_bSaved = !slot.m_aValues.IsEmpty();
		if (PersistPresetStore())
			Print(string.Format("[DCO-GM] scenario preset %1 saved (%2 attributes)", m_iPresetSlot + 1, slot.m_aValues.Count()), LogLevel.NORMAL);
		UpdatePresetControls();
	}

	protected void LoadSelectedPreset()
	{
		if (!m_bCogSession || !m_bEditing || !m_aSessionAttributes)
			return;
		EnsurePresetStore();
		DCO_ScenarioPresetSlot slot = m_PresetStore.m_aSlots[m_iPresetSlot];
		if (!slot.m_bSaved || slot.m_aValues.IsEmpty())
			return;

		SCR_AttributesManagerEditorComponent manager = GetManager();
		if (!manager)
			return;
		int applied;
		for (int i = 0; i < m_aSessionAttributes.Count(); i++)
		{
			SCR_BaseEditorAttribute attribute = m_aSessionAttributes[i];
			if (!attribute || !SupportsLayout(attribute))
				continue;
			DCO_ScenarioPresetValue saved = FindPresetValue(slot, GetPresetKey(attribute, i));
			if (!saved || saved.m_iKind != GetPresetKind(attribute))
				continue;
			SCR_BaseEditorAttributeVar var = attribute.GetVariable(true);
			if (!var)
				continue;

			switch (saved.m_iKind)
			{
				case PRESET_BOOL:   var.SetBool(saved.m_bBool); break;
				case PRESET_FLOAT:  var.SetFloat(saved.m_fFloat); break;
				case PRESET_VECTOR: var.SetVector(saved.m_vVector); break;
				default:            var.SetInt(saved.m_iInt); break;
			}
			attribute.UpdateInterlinkedVariables(var, manager);
			attribute.PreviewVariable(true, manager);
			applied++;
		}

		RenderAttributes(m_aSessionAttributes, false);
		Print(string.Format("[DCO-GM] scenario preset %1 loaded (%2 attributes previewed)", m_iPresetSlot + 1, applied), LogLevel.NORMAL);
	}

	protected void ClearContent()
	{
		if (!m_wContent)
			return;
		foreach (DCO_ScenarioOptionRow row : m_Rows)
		{
			if (row)
				row.Shutdown();
		}
		m_Rows.Clear();
		m_wPlaceholder = null;	// it lives inside the content holder; it's deleted with the first clear.
		while (m_wContent.GetChildren())
			delete m_wContent.GetChildren();
	}

	void Shutdown()
	{
		if (m_bEditing)
			EndEditing(false);	// abandon an open session cleanly on teardown.
		DCO_GMUIController.SetPropertyOverlaysSuppressed(false);
		if (m_Manager && m_bSubscribed)
			m_Manager.GetOnAttributesStart().Remove(OnAttributesStart);
		m_bSubscribed = false;
		m_OptionMenuCallback.Remove(OnOptionPickerAction);
		m_OptionPickerRow = null;
		m_OptionPickerAnchor = null;
		m_Menu = null;
		m_Manager = null;
		m_ScrollHandler = null;
		m_wScroll = null;
		m_btnCog = null;
		m_btnClose = null;
		m_wPresetBar = null;
		m_wPresetMenu = null;
		m_btnPresetSelect = null;
		m_btnPresetLoad = null;
		m_btnPresetSave = null;
		m_wPresetSelectLabel = null;
		m_wPanel = null;
		m_wBackdrop = null;
		m_BackdropHandler = null;
		m_wContent = null;
		m_wPlaceholder = null;
		m_aSessionAttributes = null;
		m_Rows.Clear();
		m_CatTabBtns.Clear();
		m_CatTabLabels.Clear();
		m_CatHandlers.Clear();
		m_PresetSlotBtns.Clear();
		m_PresetSlotLabels.Clear();
		m_PresetStore = null;
		GetGame().GetCallqueue().Remove(RenderSelectedCategory);
		GetGame().GetCallqueue().Remove(SelectTimeAndDateCategory);
		m_bCategoryRefreshQueued = false;
		m_bConditionalRefreshQueued = false;
		m_wTitle = null;
		m_wCloseLabel = null;
		m_Handler = null;
		m_wRoot = null;
	}
}
