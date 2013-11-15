package org.netxms.ui.eclipse.imagelibrary;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.imagelibrary.messages"; //$NON-NLS-1$
	public static String ImageLibrary_ActionDelete;
	public static String ImageLibrary_ActionEdit;
	public static String ImageLibrary_ActionUpload;
	public static String ImageLibrary_ActionZoomIn;
	public static String ImageLibrary_ActionZoomOut;
	public static String ImageLibrary_LoadError;
	public static String ImageLibrary_ReloadJob;
	public static String ImageLibrary_UpdateError;
	public static String ImageLibrary_UpdateImage;
	public static String ImageLibrary_UpdateJob;
	public static String ImageLibrary_UploadError;
	public static String ImageLibrary_UploadImage;
	public static String ImageLibrary_UploadJob;
	public static String ImagePropertiesDialog_AllFiles;
	public static String ImagePropertiesDialog_Category;
	public static String ImagePropertiesDialog_ImageFile;
	public static String ImagePropertiesDialog_ImageFiles;
	public static String ImagePropertiesDialog_ImageName;
	public static String ImagePropertiesDialog_Title;
	public static String ImagePropertiesDialog_Upload;
	public static String ImageProvider_DecodeError;
	public static String ImageProvider_JobName;
	public static String ImageProvider_ReadError;
	public static String ImageSelectionDialog_Default;
	public static String ImageSelectionDialog_Title;
	public static String ImageSelector_Default;
	public static String ImageSelector_SelectImage;
	public static String LoginListener_JobName;
	public static String OpenLibraryManager_Error;
	public static String OpenLibraryManager_ErrorText;
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
