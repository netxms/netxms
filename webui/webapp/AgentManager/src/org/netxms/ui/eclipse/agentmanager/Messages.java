package org.netxms.ui.eclipse.agentmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.agentmanager.messages"; //$NON-NLS-1$
	public String AgentConfigEditorView_Error;
	public String AgentConfigEditorView_PartName;
	public String AgentConfigEditorView_Save;
	public String AgentConfigEditorView_SaveError;
	public String DeploymentStatusLabelProvider_Completed;
	public String DeploymentStatusLabelProvider_Failed;
	public String DeploymentStatusLabelProvider_Init;
	public String DeploymentStatusLabelProvider_Installing;
	public String DeploymentStatusLabelProvider_Pending;
	public String DeploymentStatusLabelProvider_Unknown;
	public String DeploymentStatusLabelProvider_Uploading;
	public String OpenAgentConfig_Error;
	public String OpenAgentConfig_GetConfig;
	public String OpenAgentConfig_OpenError;
	public String OpenAgentConfig_OpenErrorPrefix;
	public String OpenPackageManager_Error;
	public String OpenPackageManager_ErrorOpenView;
	public String PackageDeploymentMonitor_ColumnMessage;
	public String PackageDeploymentMonitor_ColumnNode;
	public String PackageDeploymentMonitor_ColumnStatus;
	public String PackageManager_ColumnDescription;
	public String PackageManager_ColumnFile;
	public String PackageManager_ColumnID;
	public String PackageManager_ColumnName;
	public String PackageManager_ColumnPlatform;
	public String PackageManager_ColumnVersion;
	public String PackageManager_ConfirmDeleteText;
	public String PackageManager_ConfirmDeleteTitle;
	public String PackageManager_DBUnlockError;
	public String PackageManager_DeletePackages;
	public String PackageManager_DeployAction;
	public String PackageManager_DeployAgentPackage;
	public String PackageManager_DepStartError;
	public String PackageManager_Error;
	public String PackageManager_ErrorOpenView;
	public String PackageManager_FileTypeAll;
	public String PackageManager_FileTypePackage;
	public String PackageManager_Information;
	public String PackageManager_InstallAction;
	public String PackageManager_InstallError;
	public String PackageManager_InstallPackage;
	public String PackageManager_LoadPkgList;
	public String PackageManager_OpenDatabase;
	public String PackageManager_OpenError;
	public String PackageManager_PkgDeleteError;
	public String PackageManager_PkgDepCompleted;
	public String PackageManager_PkgFileOpenError;
	public String PackageManager_PkgListLoadError;
	public String PackageManager_RemoveAction;
	public String PackageManager_SelectFile;
	public String PackageManager_UnlockDatabase;
	public String PackageManager_UploadPackage;
	public String SaveConfigDialog_Cancel;
	public String SaveConfigDialog_Discard;
	public String SaveConfigDialog_ModifiedMessage;
	public String SaveConfigDialog_Save;
	public String SaveConfigDialog_SaveApply;
	public String SaveConfigDialog_UnsavedChanges;
   public String OpenAgentConfigManager_Error;
   public String OpenAgentConfigManager_ErrorMessage;
   public String PackageDeploymentMonitor_RestartFailedInstallation;
   public String SaveStoredConfigDialog_SaveWarning;
   public String ScreenshotView_AllFiles;
   public String ScreenshotView_CannotCreateFile;
   public String ScreenshotView_CannotSaveImage;
   public String ScreenshotView_CopyToClipboard;
   public String ScreenshotView_Error;
   public String ScreenshotView_JobError;
   public String ScreenshotView_JobTitle;
   public String ScreenshotView_PartTitle;
   public String ScreenshotView_PngFiles;
   public String ScreenshotView_Save;
   public String ScreenshotView_SaveScreenshot;
   public String ServerStoredAgentConfigEditorView_ConfigFile;
   public String ServerStoredAgentConfigEditorView_Delete;
   public String ServerStoredAgentConfigEditorView_Filter;
   public String ServerStoredAgentConfigEditorView_JobError_Delete;
   public String ServerStoredAgentConfigEditorView_JobError_GetContent;
   public String ServerStoredAgentConfigEditorView_JobError_GetList;
   public String ServerStoredAgentConfigEditorView_JobError_MoveDown;
   public String ServerStoredAgentConfigEditorView_JobError_MoveUp;
   public String ServerStoredAgentConfigEditorView_JobError_Save;
   public String ServerStoredAgentConfigEditorView_JobTitle_CreateNew;
   public String ServerStoredAgentConfigEditorView_JobTitle_GetContent;
   public String ServerStoredAgentConfigEditorView_MoveDown;
   public String ServerStoredAgentConfigEditorView_MoveUp;
   public String ServerStoredAgentConfigEditorView_Name;
   public String ServerStoredAgentConfigEditorView_Noname;
   public String TakeScreenshot_Error;
   public String TakeScreenshot_ErrorOpeningView;
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
