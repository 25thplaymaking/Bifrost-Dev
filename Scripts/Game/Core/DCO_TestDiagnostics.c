// Bounded, observational tracing for the acceptance-test candidate.
class DCO_TestDiagnostics
{
 static const bool ENABLED = true;
 protected static const int MAX_EVENTS = 10000;
 protected static int s_Count;

 static void Event(string name, string detail = "", bool suspect = false)
 {
  if (!ENABLED || s_Count >= MAX_EVENTS) return;
  s_Count++;
  if (detail.Length() > 800) detail = detail.Substring(0, 800);
  string role = "client";
  if (Replication.IsServer()) role = "authority";
  string line = string.Format("[BF-DIAG] #%1 tick=%2 role=%3 %4 %5", s_Count, System.GetTickCount(), role, name, detail);
  if (suspect) Print(line, LogLevel.WARNING);
  else Print(line, LogLevel.NORMAL);
  if (s_Count == MAX_EVENTS) Print("[BF-DIAG] TRACE_LIMIT reached; further diagnostic events are suppressed for this script session.", LogLevel.WARNING);
 }

 static string WidgetState(Widget widget)
 {
  if (!widget) return "missing";
  return string.Format("%1:visible=%2:hierarchy=%3:enabled=%4", widget.GetName(), widget.IsVisible(), widget.IsVisibleInHierarchy(), widget.IsEnabled());
 }

 static BIA_ApplyReport ApplyResult(BIA_ApplyReport report, int playerId)
 {
  if (report)
   Event("kit.apply.end", string.Format("player=%1 status=%2 applied=%3 skipped=%4", playerId, report.m_eStatus, report.m_iApplied, report.m_iSkipped), report.m_eStatus != BIA_EApplyStatus.SUCCESS);
  else Event("kit.apply.end", "missing report", true);
  return report;
 }
}
