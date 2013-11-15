package org.netxms.ui.eclipse.eventmanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.eventmanager.messages"; //$NON-NLS-1$
	public static String EditEventTemplateDialog_Description;
	public static String EditEventTemplateDialog_EventCode;
	public static String EditEventTemplateDialog_EventName;
	public static String EditEventTemplateDialog_Message;
	public static String EditEventTemplateDialog_Severity;
	public static String EditEventTemplateDialog_TitleCreate;
	public static String EditEventTemplateDialog_TitleEdit;
	public static String EditEventTemplateDialog_WriteToLog;
	public static String EventConfigurator_ColCode;
	public static String EventConfigurator_ColDescription;
	public static String EventConfigurator_ColFlags;
	public static String EventConfigurator_ColMessage;
	public static String EventConfigurator_ColName;
	public static String EventConfigurator_ColSeverity;
	public static String EventConfigurator_CreateJob_Error;
	public static String EventConfigurator_CreateJob_Title;
	public static String EventConfigurator_Delete;
	public static String EventConfigurator_DeleteConfirmation_Plural;
	public static String EventConfigurator_DeleteConfirmation_Singular;
	public static String EventConfigurator_DeleteConfirmationTitle;
	public static String EventConfigurator_DeleteJob_Error;
	public static String EventConfigurator_DeleteJob_Title;
	public static String EventConfigurator_NewEvent;
	public static String EventConfigurator_OpenJob_Error;
	public static String EventConfigurator_OpenJob_Title;
	public static String EventConfigurator_Properties;
	public static String EventConfigurator_UpdateJob_Error;
	public static String EventConfigurator_UpdateJob_Title;
	public static String EventLabelProvider_Unknown;
	public static String EventMonitor_ColEvent;
	public static String EventMonitor_ColMessage;
	public static String EventMonitor_ColSeverity;
	public static String EventMonitor_ColSource;
	public static String EventMonitor_ColTimestamp;
	public static String EventMonitor_ShowStatusColors;
	public static String EventMonitor_ShowStatusIcons;
	public static String EventSelectionDialog_Code;
	public static String EventSelectionDialog_Filter;
	public static String EventSelectionDialog_Name;
	public static String EventSelectionDialog_Title;
	public static String EventSelector_None;
	public static String EventSelector_Severity;
	public static String EventSelector_Tooltip;
	public static String EventSelector_Unknown;
	public static String OpenEventConfigurator_Error;
	public static String OpenEventConfigurator_ErrorText;
	public static String OpenEventMonitor_Error;
	public static String OpenEventMonitor_ErrorText;
	public static String OpenSyslogMonitor_Error;
	public static String OpenSyslogMonitor_ErrorText;
	public static String OpenSyslogMonitor_JobError;
	public static String OpenSyslogMonitor_JobTitle;
	public static String SyslogLabelProvider_FacAuth;
	public static String SyslogLabelProvider_FacClock;
	public static String SyslogLabelProvider_FacCron;
	public static String SyslogLabelProvider_FacFTPD;
	public static String SyslogLabelProvider_FacKernel;
	public static String SyslogLabelProvider_FacLocal0;
	public static String SyslogLabelProvider_FacLocal1;
	public static String SyslogLabelProvider_FacLocal2;
	public static String SyslogLabelProvider_FacLocal3;
	public static String SyslogLabelProvider_FacLocal4;
	public static String SyslogLabelProvider_FacLocal5;
	public static String SyslogLabelProvider_FacLocal6;
	public static String SyslogLabelProvider_FacLocal7;
	public static String SyslogLabelProvider_FacLogAlert;
	public static String SyslogLabelProvider_FacLogAudit;
	public static String SyslogLabelProvider_FacLpr;
	public static String SyslogLabelProvider_FacMail;
	public static String SyslogLabelProvider_FacNews;
	public static String SyslogLabelProvider_FacNTP;
	public static String SyslogLabelProvider_FacSecurity;
	public static String SyslogLabelProvider_FacSyslog;
	public static String SyslogLabelProvider_FacSystem;
	public static String SyslogLabelProvider_FacUser;
	public static String SyslogLabelProvider_FacUUCP;
	public static String SyslogLabelProvider_SevAlert;
	public static String SyslogLabelProvider_SevCritical;
	public static String SyslogLabelProvider_SevDebug;
	public static String SyslogLabelProvider_SevEmergency;
	public static String SyslogLabelProvider_SevError;
	public static String SyslogLabelProvider_SevInfo;
	public static String SyslogLabelProvider_SevNotice;
	public static String SyslogLabelProvider_SevWarning;
	public static String SyslogLabelProvider_Unknown;
	public static String SyslogMonitor_ColFacility;
	public static String SyslogMonitor_ColHostName;
	public static String SyslogMonitor_ColMessage;
	public static String SyslogMonitor_ColSeverity;
	public static String SyslogMonitor_ColSource;
	public static String SyslogMonitor_ColTag;
	public static String SyslogMonitor_ColTimestamp;
	public static String SyslogMonitor_ShowStatusColors;
	public static String SyslogMonitor_ShowStatusIcons;
	public static String SyslogMonitor_SubscribeJob_Error;
	public static String SyslogMonitor_SubscribeJob_Title;
	public static String SyslogMonitor_UnsubscribeJob_Error;
	public static String SyslogMonitor_UnsubscribeJob_Title;
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
