package org.netxms.ui.eclipse.nxsl;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.nxsl.messages"; //$NON-NLS-1$
   public static String CreateScriptDialog_CreateNew;
   public static String CreateScriptDialog_Rename;
   public static String CreateScriptDialog_ScriptName;
   public static String CreateScriptDialog_Warning;
   public static String CreateScriptDialog_WarningEmptyName;
   public static String ExecuteServerScript_Error;
   public static String ExecuteServerScript_ErrorMessage;
   public static String OpenScriptLibrary_Error;
   public static String OpenScriptLibrary_ErrorMsg;
   public static String SaveScriptDialog_Cancel;
   public static String SaveScriptDialog_Discard;
   public static String SaveScriptDialog_Message;
   public static String SaveScriptDialog_Save;
   public static String SaveScriptDialog_SaveAs;
   public static String SaveScriptDialog_Title;
   public static String ScriptEditDialog_Title;
   public static String ScriptEditorView_Error;
   public static String ScriptEditorView_LoadJobError;
   public static String ScriptEditorView_LoadJobTitle;
   public static String ScriptEditorView_PartName;
   public static String ScriptEditorView_Save;
   public static String ScriptEditorView_SaveErrorMessage;
   public static String ScriptEditorView_SaveJobError;
   public static String ScriptEditorView_SaveJobTitle;
   public static String ScriptExecutor_Clear;
   public static String ScriptExecutor_ClearOutput;
   public static String ScriptExecutor_Error;
   public static String ScriptExecutor_Execute;
   public static String ScriptExecutor_JobError_Create;
   public static String ScriptExecutor_JobError_Execute;
   public static String ScriptExecutor_JobError_ReadList;
   public static String ScriptExecutor_JobError_Update;
   public static String ScriptExecutor_JobName_Create;
   public static String ScriptExecutor_JobName_Execute;
   public static String ScriptExecutor_JobName_ReadList;
   public static String ScriptExecutor_JobName_Update;
   public static String ScriptExecutor_LibScript;
   public static String ScriptExecutor_Noname;
   public static String ScriptExecutor_Output;
   public static String ScriptExecutor_PartName;
   public static String ScriptExecutor_Reload;
   public static String ScriptExecutor_Save;
   public static String ScriptExecutor_SaveAs;
   public static String ScriptExecutor_SaveError;
   public static String ScriptExecutor_Source;
   public static String ScriptLibrary_ColumnId;
   public static String ScriptLibrary_ColumnName;
   public static String ScriptLibrary_Confirmation;
   public static String ScriptLibrary_ConfirmationText;
   public static String ScriptLibrary_CreateJobError;
   public static String ScriptLibrary_CreateJobTitle;
   public static String ScriptLibrary_Delete;
   public static String ScriptLibrary_DeleteJobError;
   public static String ScriptLibrary_DeleteJobTitle;
   public static String ScriptLibrary_Edit;
   public static String ScriptLibrary_EditScriptError;
   public static String ScriptLibrary_Error;
   public static String ScriptLibrary_LoadJobError;
   public static String ScriptLibrary_LoadJobTitle;
   public static String ScriptLibrary_New;
   public static String ScriptLibrary_Rename;
   public static String ScriptLibrary_RenameJobError;
   public static String ScriptLibrary_RenameJobTitle;
   public static String SelectScriptDialog_AvailableScripts;
   public static String SelectScriptDialog_JobError;
   public static String SelectScriptDialog_JobTitle;
   public static String SelectScriptDialog_Title;
   public static String SelectScriptDialog_Warning;
   public static String SelectScriptDialog_WarningEmptySelection;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
