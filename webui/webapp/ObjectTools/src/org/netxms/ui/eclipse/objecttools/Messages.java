package org.netxms.ui.eclipse.objecttools;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objecttools.messages"; //$NON-NLS-1$
   public String AccessControl_Add;
   public String AccessControl_Delete;
   public String AccessControl_Label;
   public String BrowserView_Back;
   public String BrowserView_Forward;
   public String BrowserView_PartName_Changed;
   public String BrowserView_PartName_Changing;
   public String BrowserView_Stop;
   public String Columns_Add;
   public String Columns_DefName;
   public String Columns_Delete;
   public String Columns_Edit;
   public String Columns_Format;
   public String Columns_Index;
   public String Columns_Name;
   public String Columns_OID;
   public String CreateNewToolDialog_Name;
   public String CreateNewToolDialog_ToolType;
   public String EditColumnDialog_CreateColumn;
   public String EditColumnDialog_EditColumn;
   public String EditColumnDialog_EnterValidIndex;
   public String EditColumnDialog_FmtFloat;
   public String EditColumnDialog_FmtIfIndex;
   public String EditColumnDialog_FmtInt;
   public String EditColumnDialog_FmtIpAddr;
   public String EditColumnDialog_FmtMacAddr;
   public String EditColumnDialog_FmtString;
   public String EditColumnDialog_Format;
   public String EditColumnDialog_Name;
   public String EditColumnDialog_SNMP_OID;
   public String EditColumnDialog_SubstrIndex;
   public String EditColumnDialog_Warning;
   public String FileViewer_InvalidObjectID;
   public String Filter_AgentNeeded;
   public String Filter_OIDShouldMatch;
   public String Filter_SNMPNeeded;
   public String General_AgentAction;
   public String General_Command;
   public String General_Confirmation;
   public String General_ConfirmationMessage;
   public String General_Description;
   public String General_ExecOptions;
   public String General_FirstColumnValue;
   public String General_GeneratesOutput;
   public String General_Name;
   public String General_OIDSuffix;
   public String General_Operation;
   public String General_Parameter;
   public String General_RegExp;
   public String General_RemoteFileName;
   public String General_RequiresConfirmation;
   public String General_SNMPTableOptions;
   public String General_Title;
   public String General_URL;
   public String General_UseAsIndex;
   public String LocalCommandResults_ClearConsole;
   public String LocalCommandResults_Copy;
   public String LocalCommandResults_JobError;
   public String LocalCommandResults_JobTitle;
   public String LocalCommandResults_Restart;
   public String LocalCommandResults_ScrollLock;
   public String LocalCommandResults_SelectAll;
   public String LocalCommandResults_Terminate;
   public String LocalCommandResults_Terminated;
   public String ObjectToolsAdapterFactory_Error;
   public String ObjectToolsAdapterFactory_LoaderErrorText;
   public String ObjectToolsDynamicMenu_CannotExecuteOnNode;
   public String ObjectToolsDynamicMenu_CannotOpenWebBrowser;
   public String ObjectToolsDynamicMenu_ConfirmExec;
   public String ObjectToolsDynamicMenu_DownloadError;
   public String ObjectToolsDynamicMenu_DownloadFromAgent;
   public String ObjectToolsDynamicMenu_Error;
   public String ObjectToolsDynamicMenu_ErrorOpeningView;
   public String ObjectToolsDynamicMenu_ExecSuccess;
   public String ObjectToolsDynamicMenu_ExecuteOnNode;
   public String ObjectToolsDynamicMenu_ExecuteServerCmd;
   public String ObjectToolsDynamicMenu_HandlerNotDefined;
   public String ObjectToolsDynamicMenu_Information;
   public String ObjectToolsDynamicMenu_MultipleNodes;
   public String ObjectToolsDynamicMenu_ServerCmdExecError;
   public String ObjectToolsDynamicMenu_ServerCommandExecuted;
   public String ObjectToolsDynamicMenu_ToolExecution;
   public String ObjectToolsDynamicMenu_TopLevelLabel;
   public String ObjectToolsEditor_ColDescr;
   public String ObjectToolsEditor_ColId;
   public String ObjectToolsEditor_ColName;
   public String ObjectToolsEditor_ColType;
   public String ObjectToolsEditor_Confirmation;
   public String ObjectToolsEditor_Delete;
   public String ObjectToolsEditor_DeleteConfirmation;
   public String ObjectToolsEditor_JobDelete;
   public String ObjectToolsEditor_JobDeleteError;
   public String ObjectToolsEditor_JobGetConfig;
   public String ObjectToolsEditor_JobGetConfigError;
   public String ObjectToolsEditor_JobNewId;
   public String ObjectToolsEditor_JobNewIdError;
   public String ObjectToolsEditor_JobSave;
   public String ObjectToolsEditor_JobSaveError;
   public String ObjectToolsEditor_New;
   public String ObjectToolsLabelProvider_TypeAction;
   public String ObjectToolsLabelProvider_TypeAgentTable;
   public String ObjectToolsLabelProvider_TypeDownloadFile;
   public String ObjectToolsLabelProvider_TypeInternal;
   public String ObjectToolsLabelProvider_TypeLocalCmd;
   public String ObjectToolsLabelProvider_TypeServerCmd;
   public String ObjectToolsLabelProvider_TypeSNMPTable;
   public String ObjectToolsLabelProvider_TypeURL;
   public String OpenObjectToolsEditor_Error;
   public String OpenObjectToolsEditor_ErrorOpenView;
   public String TableToolResults_InvalidObjectID;
   public String TableToolResults_InvalidToolID;
   public String TableToolResults_JobError;
   public String TableToolResults_JobTitle;
   public String ToolColumnLabelProvider_FmtFloat;
   public String ToolColumnLabelProvider_FmtIfIndex;
   public String ToolColumnLabelProvider_FmtInteger;
   public String ToolColumnLabelProvider_FmtIpAddr;
   public String ToolColumnLabelProvider_FmtMacAddr;
   public String ToolColumnLabelProvider_FmtString;
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

}
