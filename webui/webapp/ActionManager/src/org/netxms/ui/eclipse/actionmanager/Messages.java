package org.netxms.ui.eclipse.actionmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.actionmanager.messages"; //$NON-NLS-1$
	public String ActionLabelProvider_ActionTypeExecute;
	public String ActionLabelProvider_ActionTypeForward;
	public String ActionLabelProvider_ActionTypeMail;
	public String ActionLabelProvider_ActionTypeRemoteExec;
	public String ActionLabelProvider_ActionTypeSMS;
	public String ActionLabelProvider_Unknown;
	public String ActionManager_ActionDelete;
	public String ActionManager_ActionNew;
	public String ActionManager_ActionProperties;
	public String ActionManager_ColumnData;
	public String ActionManager_ColumnName;
	public String ActionManager_ColumnRcpt;
	public String ActionManager_ColumnSubj;
	public String ActionManager_ColumnType;
	public String ActionManager_Confirmation;
	public String ActionManager_ConfirmDelete;
	public String ActionManager_CreateJobError;
	public String ActionManager_CreateJobName;
	public String ActionManager_DeleteJobError;
	public String ActionManager_DeleteJobName;
	public String ActionManager_LoadJobError;
	public String ActionManager_LoadJobName;
	public String ActionManager_UiUpdateJobName;
	public String ActionManager_UodateJobError;
	public String ActionManager_UpdateJobName;
	public String EditActionDlg_Action;
	public String EditActionDlg_ActionDisabled;
	public String EditActionDlg_Command;
	public String EditActionDlg_CreateAction;
	public String EditActionDlg_EditAction;
	public String EditActionDlg_ExecCommandOnNode;
	public String EditActionDlg_ExecCommandOnServer;
	public String EditActionDlg_ExecuteScript;
	public String EditActionDlg_ForwardEvent;
	public String EditActionDlg_MailSubject;
	public String EditActionDlg_MessageText;
	public String EditActionDlg_Name;
	public String EditActionDlg_Options;
	public String EditActionDlg_PhoneNumber;
	public String EditActionDlg_Recipient;
	public String EditActionDlg_RemoteHost;
	public String EditActionDlg_RemoteServer;
	public String EditActionDlg_ScriptName;
	public String EditActionDlg_SendSMS;
	public String EditActionDlg_SenMail;
	public String EditActionDlg_Type;
	public String OpenActionManager_Error;
	public String OpenActionManager_ErrorOpeningView;
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


