package org.netxms.ui.eclipse.osm;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.osm.messages"; //$NON-NLS-1$
	public String AbstractGeolocationView_ZoomIn;
	public String AbstractGeolocationView_ZoomOut;
	public String GeoMapViewer_DownloadError;
	public String GeoMapViewer_DownloadJob_Title;
	public String GeoMapViewer_LoadMissingJob_Title;
	public String LocationMap_InitError1;
	public String LocationMap_InitError2;
	public String LocationMap_PartNamePrefix;
	public String OpenLocationMap_Error;
	public String OpenLocationMap_ErrorText;
	public String OpenWorldMap_Error;
	public String OpenWorldMap_ErrorText;
	public String WorldMap_JobError;
	public String WorldMap_JobTitle;
	public String WorldMap_PlaceObject;
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

