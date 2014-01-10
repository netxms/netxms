package org.netxms.ui.eclipse.usermanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.usermanager.messages"; //$NON-NLS-1$
	public static String Authentication_AccountDisabled;
	public static String Authentication_AccountOptions;
	public static String Authentication_AuthMethod_Group;
	public static String Authentication_AuthMethod_Label;
	public static String Authentication_CannotChangePassword;
	public static String Authentication_Certificate;
	public static String Authentication_CertificateOrPassword;
	public static String Authentication_CertificateOrRADIUS;
	public static String Authentication_CertMapping;
	public static String Authentication_JobError;
	public static String Authentication_JobTitle;
	public static String Authentication_MappingData;
	public static String Authentication_MustChangePassword;
	public static String Authentication_Password;
	public static String Authentication_PublicKey;
	public static String Authentication_RADIUS;
	public static String Authentication_Subject;
	public static String CertificateLabelProvider_CERT_TYPE_TRUSTED_CA;
   public static String CertificateView_AckDeleteCertif;
   public static String CertificateView_Comments;
   public static String CertificateView_Confirmation;
   public static String CertificateView_CreateNewCertif;
   public static String CertificateView_DeleteCertif;
   public static String CertificateView_EditCertifData;
   public static String CertificateView_ErrorDeleteCert;
   public static String CertificateView_ErrorIncorrectCertifFile;
   public static String CertificateView_ErrorNewCertifCreation;
   public static String CertificateView_ErrorSaveCertif;
   public static String CertificateView_ErrorUnableGetCertifList;
   public static String CertificateView_ID;
   public static String CertificateView_RefreshCertif;
   public static String CertificateView_SaveCertif;
   public static String CertificateView_Subject;
   public static String CertificateView_Type;
   public static String ChangePassword_Error;
	public static String ChangePassword_Information;
	public static String ChangePassword_JobTitle;
	public static String ChangePassword_SuccessMessage;
	public static String ChangePasswordDialog_ConfirmNewPassword;
	public static String ChangePasswordDialog_NewPassword;
	public static String ChangePasswordDialog_OldPassword;
	public static String ChangePasswordDialog_Title;
	public static String CreateNewCertificateDialog_All;
   public static String CreateNewCertificateDialog_BrouseLabel;
   public static String CreateNewCertificateDialog_CertificateCommentLabel;
   public static String CreateNewCertificateDialog_FileNameLabel;
   public static String CreateNewCertificateDialog_SelectFileHeader;
   public static String CreateObjectDialog_DefAddProp;
	public static String CreateObjectDialog_LoginName;
	public static String CreateObjectDialog_NewGroup;
	public static String CreateObjectDialog_NewUser;
	public static String EditCertificateDialog_CertCommentLabel;
   public static String EditCertificateDialog_CertIDLabel;
   public static String EditCertificateDialog_CertSubjectLabel;
   public static String EditCertificateDialog_CertType_AC;
   public static String EditCertificateDialog_CertType_UC;
   public static String EditCertificateDialog_CertTypeLabel;
   public static String General_Description;
	public static String General_FullName;
	public static String General_JobError;
	public static String General_JobTitle;
	public static String General_LoginName;
	public static String General_ObjectID;
	public static String Members_Add;
	public static String Members_Delete;
	public static String Members_JobError;
	public static String Members_JobTitle;
	public static String Members_LoginName;
	public static String OpenCertificateManager_ErrorOpeningCert;
   public static String OpenCertificateManager_OpenCertificateManager_ErrorOpeningCert;
   public static String OpenUserManager_Error;
	public static String OpenUserManager_ErrorText;
	public static String SelectUserDialog_AvailableUsers;
	public static String SelectUserDialog_LoginName;
	public static String SelectUserDialog_Title;
	public static String SelectUserDialog_Warning;
	public static String SelectUserDialog_WarningText;
	public static String SystemRights_AccessConsole;
	public static String SystemRights_ConfigureActions;
	public static String SystemRights_ConfigureEvents;
	public static String SystemRights_ConfigureObjTools;
	public static String SystemRights_ConfigureSituations;
	public static String SystemRights_ConfigureTraps;
	public static String SystemRights_ControlSessions;
	public static String SystemRights_DeleteAlarms;
	public static String SystemRights_EditEPP;
	public static String SystemRights_EditServerConfig;
	public static String SystemRights_JobError;
	public static String SystemRights_JobTitle;
	public static String SystemRights_LoginAsMobile;
	public static String SystemRights_ManageAgents;
	public static String SystemRights_ManageFiles;
	public static String SystemRights_ManageMappingTables;
	public static String SystemRights_ManagePackages;
	public static String SystemRights_ManageScripts;
	public static String SystemRights_ManageUsers;
	public static String SystemRights_ReadFiles;
	public static String SystemRights_RegisterAgents;
	public static String SystemRights_SendSMS;
	public static String SystemRights_ViewAuditLog;
	public static String SystemRights_ViewEventConfig;
	public static String SystemRights_ViewEventLog;
	public static String SystemRights_ViewTrapLog;
	public static String UserLabelProvider_Group;
	public static String UserLabelProvider_User;
	public static String UserManagementView_7;
	public static String UserManagementView_CannotChangePassword;
	public static String UserManagementView_ChangePassword;
	public static String UserManagementView_ConfirmDeletePlural;
	public static String UserManagementView_ConfirmDeleteSingular;
	public static String UserManagementView_ConfirmDeleteTitle;
	public static String UserManagementView_CreateGroupJobError;
	public static String UserManagementView_CreateGroupJobName;
	public static String UserManagementView_CreateNewGroup;
	public static String UserManagementView_CreateNewUser;
	public static String UserManagementView_CreateUserJobError;
	public static String UserManagementView_CreateUserJobName;
	public static String UserManagementView_Delete;
	public static String UserManagementView_DeleteJobError;
	public static String UserManagementView_DeleteJobName;
	public static String UserManagementView_Description;
	public static String UserManagementView_FullName;
	public static String UserManagementView_GUID;
	public static String UserManagementView_Name;
	public static String UserManagementView_OpenJobError;
	public static String UserManagementView_OpenJobName;
	public static String UserManagementView_Properties;
	public static String UserManagementView_Type;
	public static String UserManagementView_UnlockJobError;
	public static String UserManagementView_UnlockJobName;
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
