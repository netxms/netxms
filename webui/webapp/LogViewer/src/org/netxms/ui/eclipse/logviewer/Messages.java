package org.netxms.ui.eclipse.logviewer;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.logviewer.messages"; //$NON-NLS-1$
	private static final ResourceBundle RESOURCE_BUNDLE = ResourceBundle.getBundle(BUNDLE_NAME);
	public String AlarmHDStateConditionEditor_Is;
	public String AlarmHDStateConditionEditor_IsNot;
	public String AlarmStateConditionEditor_Is;
	public String AlarmStateConditionEditor_IsNot;
	public String ColumnFilterEditor_And;
	public String ColumnFilterEditor_AndCondition;
	public String ColumnFilterEditor_Or;
	public String ColumnFilterEditor_OrCondition;
	public String EventConditionEditor_Is;
	public String EventConditionEditor_IsNot;
	public String EventConditionEditor_None;
	public String FilterBuilder_Add;
	public String FilterBuilder_AddColumn;
	public String FilterBuilder_ClearFilter;
	public String FilterBuilder_Close;
	public String FilterBuilder_Column;
	public String FilterBuilder_Condition;
	public String FilterBuilder_Descending;
	public String FilterBuilder_Execute;
	public String FilterBuilder_Filter;
	public String FilterBuilder_FormTitle;
	public String FilterBuilder_Ordering;
	public String FilterBuilder_Remove;
	public String IntegerConditionEditor_And;
	public String IntegerConditionEditor_Between;
	public String IntegerConditionEditor_Equal;
	public String IntegerConditionEditor_NotEqual;
	public String LogLabelProvider_Acknowledged;
	public String LogLabelProvider_Closed;
	public String LogLabelProvider_Error;
	public String LogLabelProvider_Ignored;
	public String LogLabelProvider_Open;
	public String LogLabelProvider_Outstanding;
	public String LogLabelProvider_Resolved;
	public String LogLabelProvider_Terminated;
	public String LogViewer_ActionClearFilter;
	public String LogViewer_ActionCopy;
	public String LogViewer_ActionExec;
	public String LogViewer_ActionGetMoreData;
	public String LogViewer_ActionShowFilter;
	public String LogViewer_GetDataJob;
	public String LogViewer_OpenLogError;
	public String LogViewer_OpenLogJobName;
	public String LogViewer_QueryError;
	public String LogViewer_QueryJob;
	public String LogViewer_QueryJobError;
	public String LogViewer_RefreshError;
	public String LogViewer_RefreshJob;
	public String ObjectConditionEditor_Is;
	public String ObjectConditionEditor_IsNot;
	public String ObjectConditionEditor_None;
	public String ObjectConditionEditor_NotWithin;
	public String ObjectConditionEditor_Within;
	public String OpenAlarmLog_Error;
	public String OpenAlarmLog_ErrorText;
	public String OpenAuditLog_Error;
	public String OpenAuditLog_ErrorText;
	public String OpenEventLog_Error;
	public String OpenEventLog_ErrorText;
	public String OpenSnmpTrapLog_Error;
	public String OpenSnmpTrapLog_ErrorText;
	public String OpenSyslog_Error;
	public String OpenSyslog_ErrorText;
	public String OrderingListLabelProvider_No;
	public String OrderingListLabelProvider_Yes;
	public String SeverityConditionEditor_Above;
	public String SeverityConditionEditor_Below;
	public String SeverityConditionEditor_Is;
	public String SeverityConditionEditor_IsNot;
	public String TextConditionEditor_Like;
	public String TextConditionEditor_NotLike;
	public String TimestampConditionEditor_After;
	public String TimestampConditionEditor_And;
	public String TimestampConditionEditor_Before;
	public String TimestampConditionEditor_Between;
	public String UserConditionEditor_Is;
	public String UserConditionEditor_IsNot;
	public String UserConditionEditor_None;
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

	
	/**
	 * Get message class for current locale
	 * 
	 * @return
	 */
	public static Messages get()
	{
		return RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
	}

	
	/**
	 * Get message class for current locale
	 * 
	 * @return
	 */
	public static Messages get(Display display)
	{
		CallHelper r = new CallHelper();
		display.syncExec(r);
		return r.messages;
	}
	
	/**
	 * Helper class to call RWT.NLS.getISO8859_1Encoded from non-UI thread
	 */
	private static class CallHelper implements Runnable
	{
		Messages messages;
		
		@Override
		public void run()
		{
			messages = RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
		}
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

	
	/**
	 * Get message class for current locale
	 * 
	 * @return
	 */
	public static Messages get(Display display)
	{
		CallHelper r = new CallHelper();
		display.syncExec(r);
		return r.messages;
	}
	
	/**
	 * Helper class to call RWT.NLS.getISO8859_1Encoded from non-UI thread
	 */
	private static class CallHelper implements Runnable
	{
		Messages messages;
		
		@Override
		public void run()
		{
			messages = RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
		}
	}
}


