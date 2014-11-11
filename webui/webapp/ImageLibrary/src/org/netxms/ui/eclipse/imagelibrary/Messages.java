package org.netxms.ui.eclipse.imagelibrary;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.imagelibrary.messages"; //$NON-NLS-1$
	public String ImageLibrary_ActionDelete;
	public String ImageLibrary_ActionEdit;
	public String ImageLibrary_ActionUpload;
	public String ImageLibrary_ActionZoomIn;
	public String ImageLibrary_ActionZoomOut;
	public String ImageLibrary_LoadError;
	public String ImageLibrary_ReloadJob;
	public String ImageLibrary_UpdateError;
	public String ImageLibrary_UpdateImage;
	public String ImageLibrary_UpdateJob;
	public String ImageLibrary_UploadError;
	public String ImageLibrary_UploadImage;
	public String ImageLibrary_UploadJob;
	public String ImagePropertiesDialog_AllFiles;
	public String ImagePropertiesDialog_Category;
   public String ImagePropertiesDialog_Error;
   public String ImagePropertiesDialog_ErrorText;
	public String ImagePropertiesDialog_ImageFile;
	public String ImagePropertiesDialog_ImageFiles;
	public String ImagePropertiesDialog_ImageName;
	public String ImagePropertiesDialog_Title;
	public String ImagePropertiesDialog_Upload;
	public String ImageProvider_DecodeError;
	public String ImageProvider_JobName;
	public String ImageProvider_ReadError;
	public String ImageSelectionDialog_Default;
	public String ImageSelectionDialog_Title;
	public String ImageSelector_Default;
	public String ImageSelector_SelectImage;
	public String LoginListener_JobName;
	public String OpenLibraryManager_Error;
	public String OpenLibraryManager_ErrorText;
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
