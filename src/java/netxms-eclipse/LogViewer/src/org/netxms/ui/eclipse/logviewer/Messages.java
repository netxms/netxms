package org.netxms.ui.eclipse.logviewer;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.logviewer.messages"; //$NON-NLS-1$
	private static final ResourceBundle RESOURCE_BUNDLE = ResourceBundle.getBundle(BUNDLE_NAME);
	public static String LogViewer_10;
	public static String LogViewer_2;
	public static String LogViewer_7;
	public static String LogViewer_8;
	public static String LogViewer_9;
	public static String LogViewer_ActionClearFilter;
	public static String LogViewer_ActionCopy;
	public static String LogViewer_ActionExec;
	public static String LogViewer_ActionGetMoreData;
	public static String LogViewer_ActionShowFilter;
	public static String LogViewer_AlarmLog;
	public static String LogViewer_AuditLog;
	public static String LogViewer_EventLog;
	public static String LogViewer_GetDataJob;
	public static String LogViewer_QueryError;
	public static String LogViewer_QueryJob;
	public static String LogViewer_QueryJobError;
	public static String LogViewer_RefreshError;
	public static String LogViewer_RefreshJob;
	public static String LogViewer_SnmpTrapLog;
	public static String LogViewer_syslog;
	public static String OpenAlarmLog_Error;
	public static String OpenAlarmLog_ErrorText;
	public static String OpenAuditLog_Error;
	public static String OpenAuditLog_ErrorText;
	public static String OpenEventLog_Error;
	public static String OpenEventLog_ErrorText;
	public static String OpenSnmpTrapLog_Error;
	public static String OpenSnmpTrapLog_ErrorText;
	public static String OpenSyslog_Error;
	public static String OpenSyslog_ErrorText;
	public static String OrderingListLabelProvider_No;
	public static String OrderingListLabelProvider_Yes;
	public static String SeverityConditionEditor_Above;
	public static String SeverityConditionEditor_Below;
	public static String SeverityConditionEditor_Is;
	public static String SeverityConditionEditor_IsNot;
	public static String UserConditionEditor_Is;
	public static String UserConditionEditor_IsNot;
	public static String UserConditionEditor_None;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}

	public static String getString(String key)
	{
		try
		{
			return RESOURCE_BUNDLE.getString(key);
		}
		catch(MissingResourceException e)
		{
			return '!' + key + '!';
		}
	}
}
