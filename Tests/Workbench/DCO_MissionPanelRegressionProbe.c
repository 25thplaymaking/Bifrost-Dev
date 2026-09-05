// Isolated snapshot and reply checks; creates no world entities or widgets.
class DCO_MissionPanelRegressionProbe : DCO_GMMissionPanel
{
	bool m_Open = true;
	int m_ResultCount;
	int m_Passed;
	ref array<string> m_Failures = {};

	override bool IsOpen() { return m_Open; }
	override void OnResult(bool success, string result)
	{
		m_ResultCount++;
		super.OnResult(success, result);
	}

	void Expect(bool condition, string name)
	{
		if (condition) m_Passed++;
		else m_Failures.Insert(name);
	}

	DCO_GMMarkerRecord MakeRecord(int id, string name, int kind = 1, int scope = 1)
	{
		DCO_GMMarkerRecord record = new DCO_GMMarkerRecord();
		record.m_iId = id;
		record.m_sName = name;
		record.m_iKind = kind;
		record.m_iScope = scope;
		return record;
	}

	void Run()
	{
		array<ref DCO_GMMarkerRecord> owner = {};
		owner.Insert(MakeRecord(11, "LZ Alpha"));
		array<DCO_GMMarkerRecord> incoming = {owner[0], null};
		UpdatePositions(incoming);
		Expect(m_Positions.Count() == 1 && m_NamedId == 11, "First snapshot selects the first valid position");
		owner[0].m_sName = "Changed outside panel";
		Expect(SelectedPosition() && SelectedPosition().m_sName == "LZ Alpha", "Displayed record is an independent copy");
		owner.Clear();
		Expect(incoming[0] == null, "Original weak reference expires after service owner clears");
		Expect(SelectedPosition() && SelectedPosition().m_iId == 11, "Panel copy survives original snapshot destruction");

		incoming.Clear();
		owner.Insert(MakeRecord(22, "RP Bravo", 2));
		owner.Insert(MakeRecord(11, "LZ Renamed"));
		owner.Insert(MakeRecord(-1, "Local only", 1, 0));
		owner.Insert(MakeRecord(33, "Ordinary point", 0));
		foreach (DCO_GMMarkerRecord record : owner) incoming.Insert(record);
		UpdatePositions(incoming);
		Expect(m_Positions.Count() == 2, "Local and non-position records are filtered");
		Expect(m_NamedIndex == 1 && SelectedPosition().m_sName == "LZ Renamed", "Selection survives reorder and rename by ID");
		OnAction(5);
		Expect(m_NamedId == 22, "Next wraps to the other position");

		incoming.Clear();
		owner.Clear();
		owner.Insert(MakeRecord(11, "Only remaining position"));
		incoming.Insert(owner[0]);
		UpdatePositions(incoming);
		Expect(SelectedPosition() == null && m_NamedId == 22, "Removed selection does not silently select another destination");
		UpdatePositions(incoming);
		Expect(SelectedPosition() == null, "Repeated snapshots preserve the missing-selection state");
		OnAction(4);
		Expect(SelectedPosition() && m_NamedId == 11, "Previous explicitly selects the remaining destination");
		incoming.Clear();
		owner.Clear();
		UpdatePositions(incoming);
		OnAction(4);
		OnAction(5);
		Expect(m_Positions.IsEmpty() && SelectedPosition() == null, "Empty snapshot and paging remain safe");

		m_Pending = true;
		m_RequestSequence = 42;
		OnRequestResult(41, true, "stale");
		Expect(m_Pending && m_ResultCount == 0, "Stale result cannot complete the active request");
		OnRequestResult(42, true, "matching");
		Expect(!m_Pending && m_ResultCount == 1, "Matching result completes the request");
		OnRequestResult(42, true, "duplicate");
		Expect(m_ResultCount == 1, "Duplicate result is ignored");
		m_Pending = true;
		m_RequestSequence = 43;
		OnRequestResult(42, false, "previous operation");
		Expect(m_Pending && m_ResultCount == 1, "Previous-operation error cannot overwrite a new request");
		m_Open = false;
		OnRequestResult(43, true, "closed");
		Expect(m_Pending && m_ResultCount == 1, "Closed panel ignores replies");
		m_Open = true;
		m_RequestSequence = 44;
		OnRequestResult(43, true, "old editor");
		Expect(m_Pending && m_ResultCount == 1, "Reopened panel ignores earlier editor replies");
		OnRequestResult(44, false, "current rejection");
		Expect(!m_Pending && m_ResultCount == 2, "Matching rejection releases pending state");
	}
}
