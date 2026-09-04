//! One hardpoint callout: the fixed side chip, its anchor dot on the weapon, and the leader
//! line between them. Holds no entity references — offsets and prefabs only.
class GRSA_CalloutEntry
{
	string m_sTypeLabel;
	ResourceName m_AttachedPrefab;
	typename m_SlotTypename;
	bool m_bMagazine;
	int m_iStorageSlot = -1;
	vector m_Offset;
	Widget m_wChip;
	GRSA_ItemRowComponent m_Row;
	ImageWidget m_wDot;
	ImageWidget m_wLine;
	float m_fChipX;
	float m_fChipY;
	float m_fAnchorY;
}

//! Gunsmith hardpoint callouts: chips stacked beneath the hardpoint counter, anchor dots pinned to the
//! rendered weapon, and one selected leader line tracking the preview as it orbits. Chips are
//! instanced item rows; dots and lines are code-created solid-color images, so the layer ships no
//! new layout resources.
class GRSA_CalloutLayer
{
	protected static const ResourceName CHIP_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";
	protected static const float CHIP_W = 320;
	protected static const float CHIP_H = 56;
	protected static const float CHIP_GAP = 10;
	protected static const float COLUMN_MARGIN = 48;
	protected static const float COLUMN_TOP = 96;
	protected static const float DOT_SIZE = 6;
	protected static const float LINE_WIDTH = 2;

	protected static const float DOT_SIZE_HIGHLIGHT = 12;

	protected Widget m_wRoot;
	protected GRSA_WeaponStage m_Stage;
	protected ref array<ref GRSA_CalloutEntry> m_aEntries = {};
	protected bool m_bColumnsPlaced;

	//! Chips and leader lines belong to the mouse presentation; the pad rail hides them and keeps
	//! only the anchor dots, highlighting the one whose rail row holds focus.
	protected bool m_bChipsVisible = true;
	protected int m_iHighlight = -1;
	protected int m_iLineSelection = -1;
	protected ref Color m_ColorDot = new Color(0.95, 0.95, 0.97, 1);

	//! Invoked with the clicked hardpoint's index.
	ref ScriptInvoker m_OnSlotClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GRSA_CalloutLayer(Widget calloutRoot, GRSA_WeaponStage stage)
	{
		m_wRoot = calloutRoot;
		m_Stage = stage;
	}

	//------------------------------------------------------------------------------------------------
	bool GetOffset(int index, out vector offset)
	{
		if (index < 0 || index >= m_aEntries.Count())
			return false;

		offset = m_aEntries[index].m_Offset;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_CalloutEntry GetEntry(int index)
	{
		if (index < 0 || index >= m_aEntries.Count())
			return null;

		return m_aEntries[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Entry index of the callout currently holding the given item, -1 when none does.
	int FindEntryByAttachment(ResourceName attachedPrefab)
	{
		if (attachedPrefab.IsEmpty())
			return -1;

		foreach (int i, GRSA_CalloutEntry entry : m_aEntries)
		{
			if (entry.m_AttachedPrefab == attachedPrefab)
				return i;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	int GetTotalCount()
	{
		return m_aEntries.Count();
	}

	//------------------------------------------------------------------------------------------------
	void SetChipsVisible(bool visible)
	{
		m_bChipsVisible = visible;
		foreach (GRSA_CalloutEntry entry : m_aEntries)
		{
			if (entry.m_wChip)
				entry.m_wChip.SetVisible(visible && m_bColumnsPlaced);
			if (!visible && entry.m_wLine)
				entry.m_wLine.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Emphasizes one anchor dot (the pad rail's focused row); -1 clears.
	void SetHighlight(int index)
	{
		m_iHighlight = index;
	}

	//------------------------------------------------------------------------------------------------
	//! Shows the selected hardpoint's leader on the next projection pass; -1 clears every leader.
	void SetLineSelection(int index)
	{
		if (index < 0 || index >= m_aEntries.Count())
			index = -1;

		m_iLineSelection = index;
		foreach (GRSA_CalloutEntry entry : m_aEntries)
		{
			if (entry.m_wLine)
				entry.m_wLine.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	int GetMountedCount()
	{
		int mounted = 0;
		foreach (GRSA_CalloutEntry entry : m_aEntries)
		{
			if (!entry.m_AttachedPrefab.IsEmpty())
				mounted++;
		}
		return mounted;
	}

	//------------------------------------------------------------------------------------------------
	//! Enumerates the staged weapon's hardpoints and builds one callout per visible slot. Column
	//! placement waits for the first valid projection, the render needs a frame to warm up.
	void Build(notnull IEntity weaponEntity, SCR_Faction faction)
	{
		Clear();
		if (!m_wRoot || !m_Stage)
			return;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int count = storage.GetSlotsCount();
		int anonymousSlotIndex;
		for (int i = 0; i < count; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			AttachmentSlotComponent attachment = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (attachment && !attachment.ShouldShowInInspection())
				continue;

			GRSA_CalloutEntry entry = new GRSA_CalloutEntry();
			entry.m_sTypeLabel = SlotLabel(slot, anonymousSlotIndex);
			entry.m_iStorageSlot = i;
			entry.m_Offset = slot.GetInspectionWidgetOffset();

			if (attachment && attachment.GetAttachmentSlotType())
				entry.m_SlotTypename = attachment.GetAttachmentSlotType().Type();
			else if (BaseMuzzleComponent.Cast(slot.GetParentContainer()))
				entry.m_bMagazine = true;

			IEntity attached = slot.GetAttachedEntity();
			if (attached)
				entry.m_AttachedPrefab = SCR_ResourceNameUtils.GetPrefabName(attached);

			entry.m_wChip = workspace.CreateWidgets(CHIP_LAYOUT, m_wRoot);
			if (entry.m_wChip)
			{
				FrameSlot.SetSize(entry.m_wChip, CHIP_W, CHIP_H);
				entry.m_wChip.SetVisible(false);
				Widget border = entry.m_wChip.FindAnyWidget("CalloutBorder");
				if (border)
					border.SetVisible(true);

				GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(entry.m_wChip.FindHandler(GRSA_ItemRowComponent));
				if (row)
				{
					string state = "EMPTY";
					if (!entry.m_AttachedPrefab.IsEmpty())
						state = GRSA_CatalogService.GetDisplayName(entry.m_AttachedPrefab, faction);
					row.SetSlotDisplay(entry.m_sTypeLabel, state, entry.m_AttachedPrefab);
					row.m_OnEntryClicked.Insert(OnChipClicked);
					entry.m_Row = row;
				}
			}

			entry.m_wDot = CreateSolidImage(workspace, 0.95, 0.95, 0.97, 1);
			if (entry.m_wDot)
				FrameSlot.SetSize(entry.m_wDot, DOT_SIZE, DOT_SIZE);

			entry.m_wLine = CreateSolidImage(workspace, 1, 1, 1, 0.5);

			m_aEntries.Insert(entry);
		}

		m_bColumnsPlaced = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnChipClicked(GRSA_ItemRowComponent row)
	{
		foreach (int i, GRSA_CalloutEntry entry : m_aEntries)
		{
			if (entry.m_Row == row)
			{
				m_OnSlotClicked.Invoke(i);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Per-frame: project every anchor and keep the dots plus selected leader tracking the render.
	//! Chips stay fixed in their columns once placed.
	void Reposition()
	{
		if (!m_Stage || m_aEntries.IsEmpty())
			return;

		if (!m_bColumnsPlaced)
		{
			if (!TryPlaceColumns())
				return;
		}

		foreach (int i, GRSA_CalloutEntry entry : m_aEntries)
		{
			vector anchor;
			bool ok = m_Stage.ProjectPoint(entry.m_Offset, anchor);
			if (!ok)
			{
				if (entry.m_wDot)
					entry.m_wDot.SetVisible(false);
				if (entry.m_wLine)
					entry.m_wLine.SetVisible(false);
				continue;
			}

			if (entry.m_wDot)
			{
				entry.m_wDot.SetVisible(true);
				FrameSlot.SetPos(entry.m_wDot, anchor[0], anchor[1]);

				if (i == m_iHighlight)
				{
					FrameSlot.SetSize(entry.m_wDot, DOT_SIZE_HIGHLIGHT, DOT_SIZE_HIGHLIGHT);
					entry.m_wDot.SetColor(GRSA_Theme.Separator());
				}
				else
				{
					FrameSlot.SetSize(entry.m_wDot, DOT_SIZE, DOT_SIZE);
					entry.m_wDot.SetColor(m_ColorDot);
				}
			}

			if (entry.m_wLine && entry.m_wChip && m_bChipsVisible && i == m_iLineSelection)
			{
				float chipEdgeX = entry.m_fChipX;
				float chipEdgeY = entry.m_fChipY + CHIP_H * 0.5;

				float dx = anchor[0] - chipEdgeX;
				float dy = anchor[1] - chipEdgeY;
				float length = Math.Sqrt(dx * dx + dy * dy);
				if (length >= 1)
				{
					entry.m_wLine.SetVisible(true);
					FrameSlot.SetSize(entry.m_wLine, length, LINE_WIDTH);
					FrameSlot.SetPos(entry.m_wLine, (chipEdgeX + anchor[0]) * 0.5, (chipEdgeY + anchor[1]) * 0.5);
					entry.m_wLine.SetRotation(Math.Atan2(dy, dx) * Math.RAD2DEG);
				}
				else
				{
					entry.m_wLine.SetVisible(false);
				}
			}
			else if (entry.m_wLine)
			{
				entry.m_wLine.SetVisible(false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		foreach (GRSA_CalloutEntry entry : m_aEntries)
		{
			if (entry.m_wChip)
				entry.m_wChip.RemoveFromHierarchy();
			if (entry.m_wDot)
				entry.m_wDot.RemoveFromHierarchy();
			if (entry.m_wLine)
				entry.m_wLine.RemoveFromHierarchy();
		}
		m_aEntries.Clear();
		m_bColumnsPlaced = false;
		m_iHighlight = -1;
		m_iLineSelection = -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Stacks every callout beneath the right-aligned hardpoint counter. Runs once per build,
	//! on the first valid projection.
	protected bool TryPlaceColumns()
	{
		float sizeX, sizeY;
		m_wRoot.GetScreenSize(sizeX, sizeY);
		if (sizeX <= 0)
			return false;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float rootW = workspace.DPIUnscale(sizeX);

		array<ref GRSA_CalloutEntry> column = {};
		foreach (GRSA_CalloutEntry entry : m_aEntries)
		{
			vector anchor;
			if (!m_Stage.ProjectPoint(entry.m_Offset, anchor))
				return false;

			entry.m_fAnchorY = anchor[1];
			column.Insert(entry);
		}

		SortByAnchor(column);
		float rightX = rootW - COLUMN_MARGIN - CHIP_W;
		StackColumn(column, rightX);
		m_bColumnsPlaced = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Chips stack in the same vertical order as their anchors, so the leader lines fan out
	//! without crossing each other.
	protected void SortByAnchor(notnull array<ref GRSA_CalloutEntry> column)
	{
		for (int i = 1; i < column.Count(); ++i)
		{
			ref GRSA_CalloutEntry current = column[i];
			int j = i - 1;
			while (j >= 0 && column[j].m_fAnchorY > current.m_fAnchorY)
			{
				column[j + 1] = column[j];
				j--;
			}
			column[j + 1] = current;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void StackColumn(notnull array<ref GRSA_CalloutEntry> column, float chipX)
	{
		float nextY = COLUMN_TOP;
		foreach (GRSA_CalloutEntry entry : column)
		{
			entry.m_fChipX = chipX;
			entry.m_fChipY = nextY;
			nextY += CHIP_H + CHIP_GAP;

			if (entry.m_wChip)
			{
				FrameSlot.SetPos(entry.m_wChip, entry.m_fChipX, entry.m_fChipY);
				entry.m_wChip.SetVisible(m_bChipsVisible);
				if (entry.m_Row)
					entry.m_Row.SetThumbnail(entry.m_AttachedPrefab);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected ImageWidget CreateSolidImage(notnull WorkspaceWidget workspace, float r, float g, float b, float a)
	{
		Widget w = workspace.CreateWidget(WidgetType.ImageWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.NOFOCUS, null, 0, m_wRoot);
		if (!w)
			return null;

		ImageWidget image = ImageWidget.Cast(w);
		if (!image)
			return null;

		FrameSlot.SetAlignment(image, 0.5, 0.5);
		image.SetColor(new Color(r, g, b, a));
		image.SetVisible(false);
		return image;
	}

	//------------------------------------------------------------------------------------------------
	protected string SlotLabel(notnull InventoryStorageSlot slot, inout int anonymousSlotIndex)
	{
		AttachmentSlotComponent attachment = AttachmentSlotComponent.Cast(slot.GetParentContainer());
		if (attachment)
		{
			if (attachment.GetAttachmentSlotType())
			{
				string typeName = ReadableTypeName(attachment.GetAttachmentSlotType().Type().ToString());
				string canonical = CanonicalSlotLabel(typeName);
				if (!canonical.IsEmpty())
					return canonical;

				string pretty = PrettifyTypeName(typeName);
				if (!IsUnreadableSlotLabel(typeName, pretty))
					return pretty;
			}

			return NextAttachmentLabel(anonymousSlotIndex);
		}

		if (BaseMuzzleComponent.Cast(slot.GetParentContainer()))
			return "MAGAZINE";

		return NextAttachmentLabel(anonymousSlotIndex);
	}

	//------------------------------------------------------------------------------------------------
	protected string ReadableTypeName(string typeName)
	{
		int closingBrace = typeName.LastIndexOf("}");
		if (closingBrace < 0)
			return typeName;
		if (closingBrace + 1 >= typeName.Length())
			return string.Empty;

		return typeName.Substring(closingBrace + 1, typeName.Length() - closingBrace - 1);
	}

	//------------------------------------------------------------------------------------------------
	protected string CanonicalSlotLabel(string typeName)
	{
		string normalized = typeName;
		normalized.ToLower();

		if (normalized.Contains("optic") || normalized.Contains("scope"))
			return "OPTICS";
		if (normalized.Contains("bayonet"))
			return "BAYONET";
		if (normalized.Contains("magazine"))
			return "MAGAZINE";
		if (normalized.Contains("muzzle") || normalized.Contains("suppressor") || normalized.Contains("silencer"))
			return "MUZZLE";

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected string NextAttachmentLabel(inout int anonymousSlotIndex)
	{
		anonymousSlotIndex++;
		return string.Format("ATTACHMENT %1", anonymousSlotIndex);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsUnreadableSlotLabel(string typeName, string pretty)
	{
		if (pretty.IsEmpty() || pretty == "ATTACHMENT" || pretty == "MOUNT" || pretty == "SLOT" || pretty == "TYPE" || pretty == "GUID" || pretty == "UNKNOWN")
			return true;

		string compact = typeName;
		compact.Replace("Attachment", "");
		compact.Replace("attachment", "");
		compact.Replace("GUID", "");
		compact.Replace("Guid", "");
		compact.Replace("guid", "");
		compact.Replace("{", "");
		compact.Replace("}", "");
		compact.Replace("-", "");
		compact.Replace("_", "");
		if (compact.Length() < 16)
			return false;

		for (int i = 0; i < compact.Length(); ++i)
		{
			if (!IsHexCharacter(compact.Get(i)))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsHexCharacter(string ch)
	{
		ch.ToLower();
		return ch == "0" || ch == "1" || ch == "2" || ch == "3" || ch == "4" || ch == "5" || ch == "6" || ch == "7" || ch == "8" || ch == "9"
			|| ch == "a" || ch == "b" || ch == "c" || ch == "d" || ch == "e" || ch == "f";
	}

	//------------------------------------------------------------------------------------------------
	//! "AttachmentUnderBarrelRIS" reads as "UNDER BARREL RIS" on the chip.
	protected string PrettifyTypeName(string typeName)
	{
		if (typeName.StartsWith("Attachment"))
			typeName = typeName.Substring(10, typeName.Length() - 10);

		string pretty;
		for (int i = 0; i < typeName.Length(); ++i)
		{
			string ch = typeName.Get(i);
			string upper = ch;
			upper.ToUpper();
			string lower = ch;
			lower.ToLower();

			if (i > 0 && ch == upper && upper != lower)
				pretty += " ";
			pretty += ch;
		}

		pretty.ToUpper();
		return pretty;
	}
}
