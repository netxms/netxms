package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.alarmviewer.messages"; //$NON-NLS-1$

	public static String AcknowledgeAlarm_ErrorMessage;
	public static String AcknowledgeAlarm_JobName;
	public static String AcknowledgeAlarm_TaskName;
	public static String AlarmComments_AddCommentJob;
	public static String AlarmComments_AddCommentLink;
	public static String AlarmComments_AddError;
	public static String AlarmComments_Comments;
	public static String AlarmComments_Details;
	public static String AlarmComments_GetComments;
	public static String AlarmComments_GetError;
	public static String AlarmComments_InternalError;
	public static String AlarmCommentsEditor_Edit;
	public static String AlarmCommentsEditor_Unknown;
	public static String AlarmComparator_Unknown;
	public static String AlarmDetails_Column_Message;
	public static String AlarmDetails_Column_Name;
	public static String AlarmDetails_Column_Severity;
	public static String AlarmDetails_Column_Source;
	public static String AlarmDetails_Column_Timestamp;
	public static String AlarmDetails_LastValues;
	public static String AlarmDetails_Overview;
	public static String AlarmDetails_RefreshJobError;
	public static String AlarmDetails_RefreshJobTitle;
	public static String AlarmDetails_RelatedEvents;
	public static String AlarmDetailsProvider_Error;
	public static String AlarmDetailsProvider_ErrorOpeningView;
	public static String AlarmList_AckBy;
	public static String AlarmList_Acknowledge;
	public static String AlarmList_ActionAlarmDetails;
	public static String AlarmList_ActionObjectDetails;
	public static String AlarmList_CannotResoveAlarm;
	public static String AlarmList_ColumnCount;
	public static String AlarmList_ColumnCreated;
	public static String AlarmList_ColumnLastChange;
	public static String AlarmList_ColumnMessage;
	public static String AlarmList_ColumnSeverity;
	public static String AlarmList_ColumnSource;
	public static String AlarmList_ColumnState;
	public static String AlarmList_Comments;
	public static String AlarmList_CopyMsgToClipboard;
	public static String AlarmList_CopyToClipboard;
	public static String AlarmList_Error;
	public static String AlarmList_ErrorText;
	public static String AlarmList_OpenDetailsError;
	public static String AlarmList_Resolve;
	public static String AlarmList_ResolveAlarm;
	public static String AlarmList_Resolving;
	public static String AlarmList_StickyAck;
	public static String AlarmList_SyncJobError;
	public static String AlarmList_SyncJobName;
	public static String AlarmList_Terminate;
	public static String AlarmListLabelProvider_AlarmState_Acknowledged;
	public static String AlarmListLabelProvider_AlarmState_Outstanding;
	public static String AlarmListLabelProvider_AlarmState_Resolved;
	public static String AlarmListLabelProvider_AlarmState_Terminated;
	public static String AlarmNotifier_ToolTip_Header;
	public static String AlarmReminderDialog_Dismiss;
	public static String AlarmReminderDialog_OutstandingAlarms;
	public static String Alarms_Blinking;
	public static String Alarms_ShowPopup;
	public static String Alarms_ShowReminder;
	public static String EditCommentDialog_Comment;
	public static String EditCommentDialog_EditComment;
	public static String ObjectAlarmBrowser_TitlePrefix;
	public static String OpenAlarmBrowser_Error;
	public static String OpenAlarmBrowser_ErrorOpeningView;
	public static String ShowObjectAlarms_Error;
	public static String ShowObjectAlarms_ErrorOpeningView;
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
