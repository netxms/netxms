package org.netxms.ui.eclipse.usermanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.usermanager.messages"; //$NON-NLS-1$
	public String Authentication_AccountDisabled;
	public String Authentication_AccountOptions;
	public String Authentication_AuthMethod_Group;
	public String Authentication_AuthMethod_Label;
	public String Authentication_CannotChangePassword;
	public String Authentication_Certificate;
	public String Authentication_CertificateOrPassword;
	public String Authentication_CertificateOrRADIUS;
	public String Authentication_CertMapping;
	public String Authentication_JobError;
	public String Authentication_JobTitle;
	public String Authentication_MappingData;
	public String Authentication_MustChangePassword;
	public String Authentication_Password;
	public String Authentication_PublicKey;
	public String Authentication_RADIUS;
	public String Authentication_Subject;
	public String CertificateLabelProvider_CERT_TYPE_TRUSTED_CA;
   public String CertificateView_AckDeleteCertif;
   public String CertificateView_Comments;
   public String CertificateView_Confirmation;
   public String CertificateView_CreateNewCertif;
   public String CertificateView_DeleteCertif;
   public String CertificateView_EditCertifData;
   public String CertificateView_ErrorDeleteCert;
   public String CertificateView_ErrorIncorrectCertifFile;
   public String CertificateView_ErrorNewCertifCreation;
   public String CertificateView_ErrorSaveCertif;
   public String CertificateView_ErrorUnableGetCertifList;
   public String CertificateView_ID;
   public String CertificateView_RefreshCertif;
   public String CertificateView_SaveCertif;
   public String CertificateView_Subject;
   public String CertificateView_Type;
   public String ChangePassword_Error;
	public String ChangePassword_Information;
	public String ChangePassword_JobTitle;
	public String ChangePassword_SuccessMessage;
	public String ChangePasswordDialog_ConfirmNewPassword;
	public String ChangePasswordDialog_NewPassword;
	public String ChangePasswordDialog_OldPassword;
	public String ChangePasswordDialog_Title;
	public String CreateNewCertificateDialog_All;
   public String CreateNewCertificateDialog_BrouseLabel;
   public String CreateNewCertificateDialog_CertificateCommentLabel;
   public String CreateNewCertificateDialog_FileNameLabel;
   public String CreateNewCertificateDialog_SelectFileHeader;
   public String CreateObjectDialog_DefAddProp;
	public String CreateObjectDialog_LoginName;
	public String CreateObjectDialog_NewGroup;
	public String CreateObjectDialog_NewUser;
	public String EditCertificateDialog_CertCommentLabel;
   public String EditCertificateDialog_CertIDLabel;
   public String EditCertificateDialog_CertSubjectLabel;
   public String EditCertificateDialog_CertType_AC;
   public String EditCertificateDialog_CertType_UC;
   public String EditCertificateDialog_CertTypeLabel;
   public String General_Description;
	public String General_FullName;
	public String General_JobError;
	public String General_JobTitle;
	public String General_LoginName;
	public String General_ObjectID;
	public String Members_Add;
	public String Members_Delete;
	public String Members_JobError;
	public String Members_JobTitle;
	public String Members_LoginName;
	public String OpenCertificateManager_ErrorOpeningCert;
   public String OpenCertificateManager_OpenCertificateManager_ErrorOpeningCert;
   public String OpenUserManager_Error;
	public String OpenUserManager_ErrorText;
	public String SelectUserDialog_AvailableUsers;
	public String SelectUserDialog_LoginName;
	public String SelectUserDialog_Title;
	public String SelectUserDialog_Warning;
	public String SelectUserDialog_WarningText;
	public String SystemRights_AccessConsole;
	public String SystemRights_ConfigureActions;
	public String SystemRights_ConfigureEvents;
	public String SystemRights_ConfigureObjTools;
	public String SystemRights_ConfigureSituations;
	public String SystemRights_ConfigureTraps;
	public String SystemRights_ControlSessions;
	public String SystemRights_DeleteAlarms;
	public String SystemRights_EditEPP;
	public String SystemRights_EditServerConfig;
	public String SystemRights_JobError;
	public String SystemRights_JobTitle;
	public String SystemRights_LoginAsMobile;
	public String SystemRights_ManageAgents;
	public String SystemRights_ManageFiles;
	public String SystemRights_ManageMappingTables;
	public String SystemRights_ManagePackages;
	public String SystemRights_ManageScripts;
	public String SystemRights_ManageUsers;
	public String SystemRights_ReadFiles;
	public String SystemRights_RegisterAgents;
	public String SystemRights_SendSMS;
	public String SystemRights_ViewAuditLog;
	public String SystemRights_ViewEventConfig;
	public String SystemRights_ViewEventLog;
	public String SystemRights_ViewTrapLog;
	public String UserLabelProvider_Group;
	public String UserLabelProvider_User;
	public String UserManagementView_7;
	public String UserManagementView_CannotChangePassword;
	public String UserManagementView_ChangePassword;
	public String UserManagementView_ConfirmDeletePlural;
	public String UserManagementView_ConfirmDeleteSingular;
	public String UserManagementView_ConfirmDeleteTitle;
	public String UserManagementView_CreateGroupJobError;
	public String UserManagementView_CreateGroupJobName;
	public String UserManagementView_CreateNewGroup;
	public String UserManagementView_CreateNewUser;
	public String UserManagementView_CreateUserJobError;
	public String UserManagementView_CreateUserJobName;
	public String UserManagementView_Delete;
	public String UserManagementView_DeleteJobError;
	public String UserManagementView_DeleteJobName;
	public String UserManagementView_Description;
	public String UserManagementView_FullName;
	public String UserManagementView_GUID;
	public String UserManagementView_Name;
	public String UserManagementView_OpenJobError;
	public String UserManagementView_OpenJobName;
	public String UserManagementView_Properties;
	public String UserManagementView_Type;
	public String UserManagementView_UnlockJobError;
	public String UserManagementView_UnlockJobName;
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
