package org.netxms.ui.eclipse.slm;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.slm.messages"; //$NON-NLS-1$
	public String CreateBusinessService_JobErrorPrefix;
	public String CreateBusinessService_JobErrorSuffix;
	public String CreateBusinessService_JobTitle;
	public String CreateBusinessServiceDialog_Name;
	public String CreateBusinessServiceDialog_Title;
	public String CreateBusinessServiceDialog_Warning;
	public String CreateBusinessServiceDialog_WarningText;
	public String CreateNodeLink_JobErrorPrefix;
	public String CreateNodeLink_JobErrorSuffix;
	public String CreateNodeLink_JobTitle;
	public String CreateNodeLinkDialog_Name;
	public String CreateNodeLinkDialog_Node;
	public String CreateNodeLinkDialog_Title;
	public String CreateNodeLinkDialog_Warning;
	public String CreateNodeLinkDialog_WarningText;
	public String CreateServiceCheck_JobErrorPrefix;
	public String CreateServiceCheck_JobErrorSuffix;
	public String CreateServiceCheck_JobTitle;
	public String CreateServiceCheckDialog_Name;
	public String CreateServiceCheckDialog_Title;
	public String CreateServiceCheckDialog_Warning;
	public String CreateServiceCheckDialog_WarningText;
	public String CreateServiceCheckTemplate_JobErrorPrefix;
	public String CreateServiceCheckTemplate_JobErrorSuffix;
	public String CreateServiceCheckTemplate_JobTitle;
	public String ServiceAvailability_Down;
	public String ServiceAvailability_Downtime;
	public String ServiceAvailability_InitErrorPart1;
	public String ServiceAvailability_InitErrorPart2;
	public String ServiceAvailability_InternalError;
	public String ServiceAvailability_PartNamePrefix;
	public String ServiceAvailability_ThisMonth;
	public String ServiceAvailability_ThisWeek;
	public String ServiceAvailability_Today;
	public String ServiceAvailability_Up;
	public String ServiceAvailability_Uptime;
	public String ServiceAvailability_UptimeDowntime;
	public String ServiceCheckScript_CheckScript;
	public String ServiceCheckScript_JobError;
	public String ServiceCheckScript_JobTitle;
	public String ShowAvailabilityChart_Error;
	public String ShowAvailabilityChart_ErrorText;
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


