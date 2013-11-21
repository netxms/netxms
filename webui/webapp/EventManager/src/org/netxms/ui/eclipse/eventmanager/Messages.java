package org.netxms.ui.eclipse.eventmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.eventmanager.messages"; //$NON-NLS-1$
	public String EditEventTemplateDialog_Description;
	public String EditEventTemplateDialog_EventCode;
	public String EditEventTemplateDialog_EventName;
	public String EditEventTemplateDialog_Message;
	public String EditEventTemplateDialog_Severity;
	public String EditEventTemplateDialog_TitleCreate;
	public String EditEventTemplateDialog_TitleEdit;
	public String EditEventTemplateDialog_WriteToLog;
	public String EventConfigurator_ColCode;
	public String EventConfigurator_ColDescription;
	public String EventConfigurator_ColFlags;
	public String EventConfigurator_ColMessage;
	public String EventConfigurator_ColName;
	public String EventConfigurator_ColSeverity;
	public String EventConfigurator_CreateJob_Error;
	public String EventConfigurator_CreateJob_Title;
	public String EventConfigurator_Delete;
	public String EventConfigurator_DeleteConfirmation_Plural;
	public String EventConfigurator_DeleteConfirmation_Singular;
	public String EventConfigurator_DeleteConfirmationTitle;
	public String EventConfigurator_DeleteJob_Error;
	public String EventConfigurator_DeleteJob_Title;
	public String EventConfigurator_NewEvent;
	public String EventConfigurator_OpenJob_Error;
	public String EventConfigurator_OpenJob_Title;
	public String EventConfigurator_Properties;
	public String EventConfigurator_UpdateJob_Error;
	public String EventConfigurator_UpdateJob_Title;
	public String EventLabelProvider_Unknown;
	public String EventMonitor_ColEvent;
	public String EventMonitor_ColMessage;
	public String EventMonitor_ColSeverity;
	public String EventMonitor_ColSource;
	public String EventMonitor_ColTimestamp;
	public String EventMonitor_ShowStatusColors;
	public String EventMonitor_ShowStatusIcons;
	public String EventSelectionDialog_Code;
	public String EventSelectionDialog_Filter;
	public String EventSelectionDialog_Name;
	public String EventSelectionDialog_Title;
	public String EventSelector_None;
	public String EventSelector_Severity;
	public String EventSelector_Tooltip;
	public String EventSelector_Unknown;
	public String OpenEventConfigurator_Error;
	public String OpenEventConfigurator_ErrorText;
	public String OpenEventMonitor_Error;
	public String OpenEventMonitor_ErrorText;
	public String OpenSyslogMonitor_Error;
	public String OpenSyslogMonitor_ErrorText;
	public String OpenSyslogMonitor_JobError;
	public String OpenSyslogMonitor_JobTitle;
	public String SyslogLabelProvider_FacAuth;
	public String SyslogLabelProvider_FacClock;
	public String SyslogLabelProvider_FacCron;
	public String SyslogLabelProvider_FacFTPD;
	public String SyslogLabelProvider_FacKernel;
	public String SyslogLabelProvider_FacLocal0;
	public String SyslogLabelProvider_FacLocal1;
	public String SyslogLabelProvider_FacLocal2;
	public String SyslogLabelProvider_FacLocal3;
	public String SyslogLabelProvider_FacLocal4;
	public String SyslogLabelProvider_FacLocal5;
	public String SyslogLabelProvider_FacLocal6;
	public String SyslogLabelProvider_FacLocal7;
	public String SyslogLabelProvider_FacLogAlert;
	public String SyslogLabelProvider_FacLogAudit;
	public String SyslogLabelProvider_FacLpr;
	public String SyslogLabelProvider_FacMail;
	public String SyslogLabelProvider_FacNews;
	public String SyslogLabelProvider_FacNTP;
	public String SyslogLabelProvider_FacSecurity;
	public String SyslogLabelProvider_FacSyslog;
	public String SyslogLabelProvider_FacSystem;
	public String SyslogLabelProvider_FacUser;
	public String SyslogLabelProvider_FacUUCP;
	public String SyslogLabelProvider_SevAlert;
	public String SyslogLabelProvider_SevCritical;
	public String SyslogLabelProvider_SevDebug;
	public String SyslogLabelProvider_SevEmergency;
	public String SyslogLabelProvider_SevError;
	public String SyslogLabelProvider_SevInfo;
	public String SyslogLabelProvider_SevNotice;
	public String SyslogLabelProvider_SevWarning;
	public String SyslogLabelProvider_Unknown;
	public String SyslogMonitor_ColFacility;
	public String SyslogMonitor_ColHostName;
	public String SyslogMonitor_ColMessage;
	public String SyslogMonitor_ColSeverity;
	public String SyslogMonitor_ColSource;
	public String SyslogMonitor_ColTag;
	public String SyslogMonitor_ColTimestamp;
	public String SyslogMonitor_ShowStatusColors;
	public String SyslogMonitor_ShowStatusIcons;
	public String SyslogMonitor_SubscribeJob_Error;
	public String SyslogMonitor_SubscribeJob_Title;
	public String SyslogMonitor_UnsubscribeJob_Error;
	public String SyslogMonitor_UnsubscribeJob_Title;
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
