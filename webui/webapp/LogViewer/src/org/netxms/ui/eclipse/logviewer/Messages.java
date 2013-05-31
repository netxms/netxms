package org.netxms.ui.eclipse.logviewer;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.logviewer.messages"; //$NON-NLS-1$
	private static final ResourceBundle RESOURCE_BUNDLE = ResourceBundle.getBundle(BUNDLE_NAME);
	public static String AlarmHDStateConditionEditor_Is;
	public static String AlarmHDStateConditionEditor_IsNot;
	public static String AlarmStateConditionEditor_Is;
	public static String AlarmStateConditionEditor_IsNot;
	public static String ColumnFilterEditor_And;
	public static String ColumnFilterEditor_AndCondition;
	public static String ColumnFilterEditor_Or;
	public static String ColumnFilterEditor_OrCondition;
	public static String EventConditionEditor_Is;
	public static String EventConditionEditor_IsNot;
	public static String EventConditionEditor_None;
	public static String FilterBuilder_Add;
	public static String FilterBuilder_AddColumn;
	public static String FilterBuilder_ClearFilter;
	public static String FilterBuilder_Close;
	public static String FilterBuilder_Column;
	public static String FilterBuilder_Condition;
	public static String FilterBuilder_Descending;
	public static String FilterBuilder_Execute;
	public static String FilterBuilder_Filter;
	public static String FilterBuilder_FormTitle;
	public static String FilterBuilder_Ordering;
	public static String FilterBuilder_Remove;
	public static String IntegerConditionEditor_And;
	public static String IntegerConditionEditor_Between;
	public static String IntegerConditionEditor_Equal;
	public static String IntegerConditionEditor_NotEqual;
	public static String LogLabelProvider_Acknowledged;
	public static String LogLabelProvider_Closed;
	public static String LogLabelProvider_Error;
	public static String LogLabelProvider_Ignored;
	public static String LogLabelProvider_Open;
	public static String LogLabelProvider_Outstanding;
	public static String LogLabelProvider_Resolved;
	public static String LogLabelProvider_Terminated;
	public static String LogViewer_ActionClearFilter;
	public static String LogViewer_ActionCopy;
	public static String LogViewer_ActionExec;
	public static String LogViewer_ActionGetMoreData;
	public static String LogViewer_ActionShowFilter;
	public static String LogViewer_GetDataJob;
	public static String LogViewer_OpenLogError;
	public static String LogViewer_OpenLogJobName;
	public static String LogViewer_QueryError;
	public static String LogViewer_QueryJob;
	public static String LogViewer_QueryJobError;
	public static String LogViewer_RefreshError;
	public static String LogViewer_RefreshJob;
	public static String ObjectConditionEditor_Is;
	public static String ObjectConditionEditor_IsNot;
	public static String ObjectConditionEditor_None;
	public static String ObjectConditionEditor_NotWithin;
	public static String ObjectConditionEditor_Within;
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
	public static String TextConditionEditor_Like;
	public static String TextConditionEditor_NotLike;
	public static String TimestampConditionEditor_After;
	public static String TimestampConditionEditor_And;
	public static String TimestampConditionEditor_Before;
	public static String TimestampConditionEditor_Between;
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

	/**
	 * This is intentional mix between old and new approach. getString
	 * used to read log display name based on log name. 
	 */
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
