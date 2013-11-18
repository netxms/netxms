package org.netxms.ui.eclipse.nxsl;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.nxsl.messages"; //$NON-NLS-1$
   public String CreateScriptDialog_CreateNew;
   public String CreateScriptDialog_Rename;
   public String CreateScriptDialog_ScriptName;
   public String CreateScriptDialog_Warning;
   public String CreateScriptDialog_WarningEmptyName;
   public String OpenScriptLibrary_Error;
   public String OpenScriptLibrary_ErrorMsg;
   public String ScriptEditorView_Error;
   public String ScriptEditorView_LoadJobError;
   public String ScriptEditorView_LoadJobTitle;
   public String ScriptEditorView_PartName;
   public String ScriptEditorView_Save;
   public String ScriptEditorView_SaveErrorMessage;
   public String ScriptEditorView_SaveJobError;
   public String ScriptEditorView_SaveJobTitle;
   public String ScriptLibrary_ColumnId;
   public String ScriptLibrary_ColumnName;
   public String ScriptLibrary_Confirmation;
   public String ScriptLibrary_ConfirmationText;
   public String ScriptLibrary_CreateJobError;
   public String ScriptLibrary_CreateJobTitle;
   public String ScriptLibrary_Delete;
   public String ScriptLibrary_DeleteJobError;
   public String ScriptLibrary_DeleteJobTitle;
   public String ScriptLibrary_Edit;
   public String ScriptLibrary_EditScriptError;
   public String ScriptLibrary_Error;
   public String ScriptLibrary_LoadJobError;
   public String ScriptLibrary_LoadJobTitle;
   public String ScriptLibrary_New;
   public String ScriptLibrary_Rename;
   public String ScriptLibrary_RenameJobError;
   public String ScriptLibrary_RenameJobTitle;
   public String SelectScriptDialog_AvailableScripts;
   public String SelectScriptDialog_JobError;
   public String SelectScriptDialog_JobTitle;
   public String SelectScriptDialog_Title;
   public String SelectScriptDialog_Warning;
   public String SelectScriptDialog_WarningEmptySelection;
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
