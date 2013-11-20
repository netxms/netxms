package org.netxms.ui.eclipse.console;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.console.messages"; //$NON-NLS-1$
	
	public String AbstractSelector_ClearSelection;
	public String AbstractSelector_CopyToClipboard;
	public String AbstractSelector_Select;
	public String AbstractTraceView_Clear;
	public String AbstractTraceView_CopyToClipboard;
	public String AbstractTraceView_Pause;
	public String AbstractTraceView_ShowFilter;
	public String ConsoleJob_ErrorDialogTitle;
	public String ExportToCsvAction_ExportToCsv;
	public String ExportToCsvAction_ExportAllToCsv;
	public String ExportToCsvAction_SaveError;
	public String ExportToCsvAction_SaveTo;
	public String FilterText_CloseFilter;
	public String FilterText_Filter;
	public String FilterText_FilterIsEmpty;
	public String NumericTextFieldValidator_ErrorMessage;
   public String NumericTextFieldValidator_RangeSeparator;
	public String RefreshAction_Name;
	public String WidgetHelper_Action_Copy;
	public String WidgetHelper_Action_Cut;
	public String WidgetHelper_Action_Delete;
	public String WidgetHelper_Action_Paste;
	public String WidgetHelper_Action_SelectAll;
	public String WidgetHelper_InputValidationError;
	public String Activator_TrayTooltip;
	public String DataCollectionDisplayInfo_Float;
	public String DataCollectionDisplayInfo_Integer;
	public String DataCollectionDisplayInfo_Integer64;
	public String DataCollectionDisplayInfo_Null;
	public String DataCollectionDisplayInfo_String;
	public String DataCollectionDisplayInfo_UInteger;
	public String DataCollectionDisplayInfo_UInteger64;
	public String DataCollectionDisplayInfo_Unknown;
	public String HttpProxyPrefs_ExcludedAddresses;
	public String HttpProxyPrefs_Login;
	public String HttpProxyPrefs_Password;
	public String HttpProxyPrefs_Port;
	public String HttpProxyPrefs_ProxyRequireAuth;
	public String HttpProxyPrefs_ProxyServer;
	public String HttpProxyPrefs_UserProxyMessage;
	public String IPAddressValidator_ErrorMessage;
	public String IPNetMaskValidator_ErrorMessage;
	public String LoginDialog_Auth;
   public String LoginDialog_Cert;
   public String LoginDialog_Error;
	public String LoginDialog_login;
   public String LoginDialog_NoCertSelected;
   public String LoginDialog_Passwd;
	public String LoginDialog_server;
	public String LoginDialog_title;
   public String LoginDialog_Warning;
   public String LoginDialog_WrongKeyStorePasswd;
	public String LoginJob_connecting;
	public String LoginJob_init_extensions;
	public String LoginJob_subscribe;
	public String LoginJob_sync_event_db;
	public String LoginJob_sync_objects;
	public String LoginJob_sync_users;
	public String MacAddressValidator_ErrorMessage;
	public String ApplicationActionBarAdvisor_About;
	public String ApplicationActionBarAdvisor_AboutTitle;
	public String ApplicationActionBarAdvisor_AboutText;
	public String ApplicationActionBarAdvisor_ConfirmRestart;
	public String ApplicationActionBarAdvisor_FullScreen;
	public String ApplicationActionBarAdvisor_LangChinese;
	public String ApplicationActionBarAdvisor_LangEnglish;
	public String ApplicationActionBarAdvisor_LangRussian;
	public String ApplicationActionBarAdvisor_LangSpanish;
	public String ApplicationActionBarAdvisor_Language;
	public String ApplicationActionBarAdvisor_Configuration;
	public String ApplicationActionBarAdvisor_File;
	public String ApplicationActionBarAdvisor_Help;
	public String ApplicationActionBarAdvisor_Monitor;
	public String ApplicationActionBarAdvisor_Tools;
	public String ApplicationActionBarAdvisor_View;
	public String ApplicationActionBarAdvisor_Navigation;
	public String ApplicationActionBarAdvisor_OpenPerspective;
	public String ApplicationActionBarAdvisor_Progress;
	public String ApplicationActionBarAdvisor_RestartConsoleMessage;
	public String ApplicationActionBarAdvisor_ShowView;
	public String ApplicationActionBarAdvisor_Window;
	public String ApplicationWorkbenchAdvisor_CommunicationError;
	public String ApplicationWorkbenchAdvisor_ConnectionLostMessage;
	public String ApplicationWorkbenchAdvisor_OKToCloseMessage;
	public String ApplicationWorkbenchAdvisor_ServerShutdownMessage;
	public String ApplicationWorkbenchWindowAdvisor_CannotChangePswd;
	public String ApplicationWorkbenchWindowAdvisor_CertDialogTitle;
   public String ApplicationWorkbenchWindowAdvisor_CertPassword;
   public String ApplicationWorkbenchWindowAdvisor_CertPasswordMsg;
   public String ApplicationWorkbenchWindowAdvisor_CertStorePassword;
   public String ApplicationWorkbenchWindowAdvisor_CertStorePasswordMsg;
   public String ApplicationWorkbenchWindowAdvisor_ChangingPassword;
	public String ApplicationWorkbenchWindowAdvisor_ConnectionError;
	public String ApplicationWorkbenchWindowAdvisor_Exception;
   public String ApplicationWorkbenchWindowAdvisor_NoEncryptionSupport;
   public String ApplicationWorkbenchWindowAdvisor_NoEncryptionSupportDetails;
	public String ApplicationWorkbenchWindowAdvisor_PasswordChanged;
   public String ApplicationWorkbenchWindowAdvisor_PkcsFiles;
	public String ApplicationWorkbenchWindowAdvisor_Error;
	public String ApplicationWorkbenchWindowAdvisor_Information;
	public String ObjectNameValidator_ErrorMessage1;
   public String ObjectNameValidator_ErrorMessage2;
	public String PasswordExpiredDialog_confirm_passwd;
	public String PasswordExpiredDialog_new_passwd;
	public String PasswordExpiredDialog_passwd_expired;
	public String PasswordExpiredDialog_title;
	public String PreferenceInitializer_DefaultDateFormat;
   public String PreferenceInitializer_DefaultShortTimeFormat;
	public String PreferenceInitializer_DefaultTimeFormat;
	public String RegionalSettingsPrefPage_DateFormatString;
	public String RegionalSettingsPrefPage_DateTimeFormat;
	public String RegionalSettingsPrefPage_Example;
	public String RegionalSettingsPrefPage_FmtCustom;
	public String RegionalSettingsPrefPage_FmtJava;
	public String RegionalSettingsPrefPage_FmtServer;
	public String RegionalSettingsPrefPage_ShortTimeExample;
   public String RegionalSettingsPrefPage_ShortTimeFormatString;
   public String RegionalSettingsPrefPage_TimeFormatString;
	public String SecurityWarningDialog_DontAskAgain;
   public String SecurityWarningDialog_Title;
   public String SendSMS_DialogTextPrefix;
	public String SendSMS_DialogTextSuffix;
	public String SendSMS_DialogTitle;
	public String SendSMS_JobTitle;
	public String SendSMS_SendError;
	public String SendSMSDialog_Message;
	public String SendSMSDialog_PhoneNumber;
	public String SendSMSDialog_Title;
	public String ServerClock_OptionShowText;
   public String ServerClock_OptionShowTimeZone;
   public String ServerClock_ServerTime;
   public String ServerClock_Tooltip;
	public String StatusDisplayInfo_Critical;
	public String StatusDisplayInfo_Disabled;
	public String StatusDisplayInfo_Major;
	public String StatusDisplayInfo_Minor;
	public String StatusDisplayInfo_Normal;
	public String StatusDisplayInfo_Testing;
	public String StatusDisplayInfo_Unknown;
	public String StatusDisplayInfo_Unmanaged;
	public String StatusDisplayInfo_Warning;
	public String WorkbenchGeneralPrefs_HideWhenMinimized;
	public String WorkbenchGeneralPrefs_show_heap;
	public String WorkbenchGeneralPrefs_show_tray_icon;
	public String WorkbenchGeneralPrefs_ShowHiddenAttrs;
   public String WorkbenchGeneralPrefs_ShowServerClock;	
	public String SplashHandler_Version;
   //start
	public String ApplicationActionBarAdvisor_AboutProductName;
	public String ApplicationWorkbenchWindowAdvisor_AppTitle;
   public String LoginForm_AdvOptionsDisabled;
   public String LoginForm_Error;
   public String LoginForm_LoginButton;
   public String LoginForm_Options;
   public String LoginForm_Password;
   public String LoginForm_Title;
   public String LoginForm_UserName;
   public String LoginForm_Version;
   public String LoginSettingsDialog_ServerAddress;
   public String LoginSettingsDialog_Title;
   //end
	
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
