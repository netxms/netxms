package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.alarmviewer.messages"; //$NON-NLS-1$
	
	public String AcknowledgeAlarm_ErrorMessage;
	public String AcknowledgeAlarm_JobName;
	public String AcknowledgeAlarm_TaskName;
	public String AlarmComments_AddCommentJob;
	public String AlarmComments_AddCommentLink;
	public String AlarmComments_AddError;
	public String AlarmComments_Comments;
	public String AlarmComments_Details;
	public String AlarmComments_GetComments;
	public String AlarmComments_GetError;
	public String AlarmComments_InternalError;
	public String AlarmCommentsEditor_Edit;
	public String AlarmCommentsEditor_Unknown;
	public String AlarmComparator_Unknown;
	public String AlarmDetailsProvider_Error;
	public String AlarmDetailsProvider_ErrorOpeningView;
	public String AlarmList_AckBy;
	public String AlarmList_Acknowledge;
	public String AlarmList_CannotResoveAlarm;
	public String AlarmList_ColumnCount;
	public String AlarmList_ColumnCreated;
	public String AlarmList_ColumnLastChange;
	public String AlarmList_ColumnMessage;
	public String AlarmList_ColumnSeverity;
	public String AlarmList_ColumnSource;
	public String AlarmList_ColumnState;
	public String AlarmList_Comments;
	public String AlarmList_CopyMsgToClipboard;
	public String AlarmList_CopyToClipboard;
	public String AlarmList_Error;
	public String AlarmList_ErrorText;
	public String AlarmList_Resolve;
	public String AlarmList_ResolveAlarm;
	public String AlarmList_Resolving;
	public String AlarmList_StickyAck;
	public String AlarmList_SyncJobError;
	public String AlarmList_SyncJobName;
	public String AlarmList_Terminate;
	public String AlarmListLabelProvider_AlarmState_Acknowledged;
	public String AlarmListLabelProvider_AlarmState_Outstanding;
	public String AlarmListLabelProvider_AlarmState_Resolved;
	public String AlarmListLabelProvider_AlarmState_Terminated;
	public String AlarmNotifier_ToolTip_Header;
	public String AlarmReminderDialog_Dismiss;
	public String AlarmReminderDialog_OutstandingAlarms;
	public String Alarms_Blinking;
	public String Alarms_ShowPopup;
	public String Alarms_ShowReminder;
	public String EditCommentDialog_Comment;
	public String EditCommentDialog_EditComment;
	public String ObjectAlarmBrowser_TitlePrefix;
	public String OpenAlarmBrowser_Error;
	public String OpenAlarmBrowser_ErrorOpeningView;
	public String ShowObjectAlarms_Error;
	public String ShowObjectAlarms_ErrorOpeningView;
	public String Startup_JobName;
	public String TerminateAlarm_ErrorMessage;
	public String TerminateAlarm_JobTitle;
	public String TerminateAlarm_TaskName;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
	
	/**
	 * Get message class for current locale
	 * 
	 * @return
	 */
	public static Messages get()
	{
		return RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
	}
}
