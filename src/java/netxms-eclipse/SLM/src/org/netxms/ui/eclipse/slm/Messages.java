package org.netxms.ui.eclipse.slm;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.slm.messages"; //$NON-NLS-1$
	public static String CreateBusinessService_JobError;
	public static String CreateBusinessService_JobTitle;
	public static String CreateBusinessServiceDialog_Name;
	public static String CreateBusinessServiceDialog_Title;
	public static String CreateBusinessServiceDialog_Warning;
	public static String CreateBusinessServiceDialog_WarningText;
	public static String CreateNodeLink_JobError;
	public static String CreateNodeLink_JobTitle;
	public static String CreateNodeLinkDialog_Name;
	public static String CreateNodeLinkDialog_Node;
	public static String CreateNodeLinkDialog_Title;
	public static String CreateNodeLinkDialog_Warning;
	public static String CreateNodeLinkDialog_WarningText;
	public static String CreateServiceCheck_JobError;
	public static String CreateServiceCheck_JobTitle;
	public static String CreateServiceCheckDialog_Name;
	public static String CreateServiceCheckDialog_Title;
	public static String CreateServiceCheckDialog_Warning;
	public static String CreateServiceCheckDialog_WarningText;
	public static String CreateServiceCheckTemplate_JobError;
	public static String CreateServiceCheckTemplate_JobTitle;
	public static String ServiceAvailability_Down;
	public static String ServiceAvailability_Downtime;
	public static String ServiceAvailability_InitError;
	public static String ServiceAvailability_InternalError;
	public static String ServiceAvailability_PartName;
	public static String ServiceAvailability_ThisMonth;
	public static String ServiceAvailability_ThisWeek;
	public static String ServiceAvailability_Today;
	public static String ServiceAvailability_Up;
	public static String ServiceAvailability_Uptime;
	public static String ServiceAvailability_UptimeDowntime;
	public static String ServiceCheckScript_CheckScript;
	public static String ServiceCheckScript_JobError;
	public static String ServiceCheckScript_JobTitle;
	public static String ShowAvailabilityChart_Error;
	public static String ShowAvailabilityChart_ErrorText;
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
