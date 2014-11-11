package org.netxms.ui.eclipse.filemanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.filemanager.messages"; //$NON-NLS-1$
	public static String AgentFileManager_AllFiles;
   public static String AgentFileManager_ColDate;
   public static String AgentFileManager_ColName;
   public static String AgentFileManager_ColSize;
   public static String AgentFileManager_ColType;
   public static String AgentFileManager_CreateFolder;
   public static String AgentFileManager_CreatingFolder;
   public static String AgentFileManager_Delete;
   public static String AgentFileManager_DirectoryReadError;
   public static String AgentFileManager_Download;
   public static String AgentFileManager_DownloadFileFromAgent;
   public static String AgentFileManager_DownloadJobError;
   public static String AgentFileManager_DownloadJobTitle;
   public static String AgentFileManager_Error;
   public static String AgentFileManager_FileDownloadError;
   public static String AgentFileManager_FolderCreationError;
   public static String AgentFileManager_MoveError;
   public static String AgentFileManager_MoveFile;
   public static String AgentFileManager_OpenViewError;
   public static String AgentFileManager_PartTitle;
   public static String AgentFileManager_RefreshFolder;
   public static String AgentFileManager_Rename;
   public static String AgentFileManager_RenameError;
   public static String AgentFileManager_Show;
   public static String AgentFileManager_StartDownloadDialogTitle;
   public static String AgentFileManager_UploadFile;
   public static String AgentFileManager_UploadFileJobTitle;
   public static String AgentFileManager_UploadFolder;
   public static String AgentFileManager_UploadFolderJobTitle;
   public static String CreateFolderDialog_Label;
   public static String CreateFolderDialog_Title;
   public static String GetServerFileList_ErrorMessageFileView;
   public static String GetServerFileList_ErrorMessageFileViewTitle;
   public static String LocalFileSelector_AllFiles;
	public static String LocalFileSelector_None;
	public static String LocalFileSelector_SelectFile;
	public static String LocalFileSelector_Tooltip;
   public static String OpenFileManager_Error;
   public static String OpenFileManager_ErrorText;
	public static String RenameFileDialog_NewName;
   public static String RenameFileDialog_OldName;
   public static String RenameFileDialog_Title;
   public static String RenameFileDialog_Warning;
   public static String RenameFileDialog_WarningMessage;
   public static String SelectServerFileDialog_ColModTime;
	public static String SelectServerFileDialog_ColName;
	public static String SelectServerFileDialog_ColSize;
	public static String SelectServerFileDialog_JobError;
	public static String SelectServerFileDialog_JobTitle;
	public static String SelectServerFileDialog_Title;
	public static String SelectServerFileDialog_Warning;
	public static String SelectServerFileDialog_WarningText;
	public static String ServerFileSelector_None;
	public static String ServerFileSelector_Tooltip;
	public static String StartClientToAgentFolderUploadDialog_Title;
   public static String StartClientToServerFileUploadDialog_LocalFile;
	public static String StartClientToServerFileUploadDialog_RemoteFileName;
	public static String StartClientToServerFileUploadDialog_Title;
	public static String StartClientToServerFileUploadDialog_Warning;
	public static String StartClientToServerFileUploadDialog_WarningText;
	public static String StartServerToAgentFileUploadDialog_CreateJobOnHold;
	public static String StartServerToAgentFileUploadDialog_RemoteFileName;
	public static String StartServerToAgentFileUploadDialog_ServerFile;
	public static String StartServerToAgentFileUploadDialog_Title;
	public static String StartServerToAgentFileUploadDialog_Warning;
	public static String StartServerToAgentFileUploadDialog_WarningText;
	public static String UploadFileToAgent_JobError;
	public static String UploadFileToAgent_JobTitle;
	public static String UploadFileToServer_JobError;
	public static String UploadFileToServer_JobTitle;
	public static String UploadFileToServer_TaskNamePrefix;
	public static String ViewAgentFilesProvider_JobError;
   public static String ViewAgentFilesProvider_JobTitle;
   public static String ViewAgentFilesProvider_Loading;
   public static String ViewServerFile_DeletAck;
   public static String ViewServerFile_DeleteConfirmation;
   public static String ViewServerFile_DeleteFileOnServerAction;
   public static String ViewServerFile_DeletFileFromServerJob;
   public static String ViewServerFile_ErrorDeleteFileJob;
   public static String ViewServerFile_FileName;
   public static String ViewServerFile_FileSize;
   public static String ViewServerFile_FileType;
   public static String ViewServerFile_ModificationDate;
   public static String ViewServerFile_ShowFilterAction;
   public static String ViewServerFile_UploadFileOnServerAction;
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
