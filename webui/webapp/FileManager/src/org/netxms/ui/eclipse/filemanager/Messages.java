package org.netxms.ui.eclipse.filemanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.filemanager.messages"; //$NON-NLS-1$
	public static String LocalFileSelector_AllFiles;
	public static String LocalFileSelector_None;
	public static String LocalFileSelector_SelectFile;
	public static String LocalFileSelector_Tooltip;
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
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
