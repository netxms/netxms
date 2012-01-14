package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.alarmviewer.messages"; //$NON-NLS-1$
	public static String AcknowledgeAlarm_ErrorMessage;
	public static String AcknowledgeAlarm_JobName;
	public static String AcknowledgeAlarm_TaskName;
	public static String AlarmComparator_Unknown;
	public static String AlarmList_ColumnCount;
	public static String AlarmList_ColumnCreated;
	public static String AlarmList_ColumnLastChange;
	public static String AlarmList_ColumnMessage;
	public static String AlarmList_ColumnSeverity;
	public static String AlarmList_ColumnSource;
	public static String AlarmList_ColumnState;
	public static String AlarmList_CopyMsgToClipboard;
	public static String AlarmList_CopyToClipboard;
	public static String AlarmList_SyncJobError;
	public static String AlarmList_SyncJobName;
	public static String AlarmListLabelProvider_AlarmState_Acknowledged;
	public static String AlarmListLabelProvider_AlarmState_Outstanding;
	public static String AlarmListLabelProvider_AlarmState_Terminated;
	public static String AlarmNotifier_ToolTip_Header;
	public static String OpenAlarmBrowser_Error;
	public static String OpenAlarmBrowser_ErrorOpeningView;
	public static String Startup_JobName;
	public static String TerminateAlarm_ErrorMessage;
	public static String TerminateAlarm_JobTitle;
	public static String TerminateAlarm_TaskName;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
