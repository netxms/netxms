package org.netxms.ui.eclipse.filemanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.filemanager.messages"; //$NON-NLS-1$
	public String LocalFileSelector_AllFiles;
	public String LocalFileSelector_None;
	public String LocalFileSelector_SelectFile;
	public String LocalFileSelector_Tooltip;
	public String SelectServerFileDialog_ColModTime;
	public String SelectServerFileDialog_ColName;
	public String SelectServerFileDialog_ColSize;
	public String SelectServerFileDialog_JobError;
	public String SelectServerFileDialog_JobTitle;
	public String SelectServerFileDialog_Title;
	public String SelectServerFileDialog_Warning;
	public String SelectServerFileDialog_WarningText;
	public String ServerFileSelector_None;
	public String ServerFileSelector_Tooltip;
	public String StartClientToServerFileUploadDialog_LocalFile;
	public String StartClientToServerFileUploadDialog_RemoteFileName;
	public String StartClientToServerFileUploadDialog_Title;
	public String StartClientToServerFileUploadDialog_Warning;
	public String StartClientToServerFileUploadDialog_WarningText;
	public String StartServerToAgentFileUploadDialog_CreateJobOnHold;
	public String StartServerToAgentFileUploadDialog_RemoteFileName;
	public String StartServerToAgentFileUploadDialog_ServerFile;
	public String StartServerToAgentFileUploadDialog_Title;
	public String StartServerToAgentFileUploadDialog_Warning;
	public String StartServerToAgentFileUploadDialog_WarningText;
	public String UploadFileToAgent_JobError;
	public String UploadFileToAgent_JobTitle;
	public String UploadFileToServer_JobError;
	public String UploadFileToServer_JobTitle;
	public String UploadFileToServer_TaskNamePrefix;
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

