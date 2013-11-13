package org.netxms.ui.eclipse.networkmaps;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.networkmaps.messages"; //$NON-NLS-1$
	public String AddGroupBoxDialog_Color;
	public String AddGroupBoxDialog_DialogTitle;
	public String AddGroupBoxDialog_Height;
	public String AddGroupBoxDialog_Title;
	public String AddGroupBoxDialog_Width;
	public String CreateMapGroup_DialogTitle;
	public String CreateMapGroup_JobError;
	public String CreateMapGroup_JobName;
	public String CreateNetworkMap_JobError;
	public String CreateNetworkMap_JobName;
	public String CreateNetworkMapDialog_Custom;
	public String CreateNetworkMapDialog_IpTopology;
	public String CreateNetworkMapDialog_L2Topology;
	public String CreateNetworkMapDialog_MapType;
	public String CreateNetworkMapDialog_Name;
	public String CreateNetworkMapDialog_PleaseEnterName;
	public String CreateNetworkMapDialog_PleaseSelectSeed;
	public String CreateNetworkMapDialog_SeedNode;
	public String CreateNetworkMapDialog_Title;
	public String CreateNetworkMapDialog_Warning;
	public String ExtendedGraphViewer_DownloadTilesError;
	public String ExtendedGraphViewer_DownloadTilesJob;
	public String GeneralMapPreferences_ShowBkgnd;
	public String GeneralMapPreferences_ShowFrame;
	public String GeneralMapPreferences_ShowIcon;
	public String IPNeighbors_PartName;
	public String NetworkMapOpenHandler_Error;
	public String NetworkMapOpenHandler_ErrorText;
	public String OpenMapObject_Error;
	public String OpenMapObject_ErrorText;
	public String ServiceComponents_Error;
	public String ServiceComponents_PartName;
	public String ShowIPNeighbors_Error;
	public String ShowIPNeighbors_ErrorText;
	public String ShowIPRoute_Error;
	public String ShowIPRoute_ErrorOpenView;
	public String ShowIPRoute_InvalidTarget;
	public String ShowLayer2Topology_Error;
	public String ShowLayer2Topology_ErrorText;
	public String ShowServiceComponents_Error;
	public String ShowServiceComponents_ErrorText;
	public String ShowServiceDependency_Error;
	public String ShowServiceDependency_ErrorText;
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


