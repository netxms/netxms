package org.netxms.ui.eclipse.usermanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.usermanager.messages"; //$NON-NLS-1$
	public static String ChangePassword_Error;
	public static String ChangePassword_Information;
	public static String ChangePassword_JobTitle;
	public static String ChangePassword_SuccessMessage;
	public static String ChangePasswordDialog_ConfirmNewPassword;
	public static String ChangePasswordDialog_NewPassword;
	public static String ChangePasswordDialog_OldPassword;
	public static String ChangePasswordDialog_Title;
	public static String CreateObjectDialog_DefAddProp;
	public static String CreateObjectDialog_LoginName;
	public static String CreateObjectDialog_NewGroup;
	public static String CreateObjectDialog_NewUser;
	public static String OpenUserManager_Error;
	public static String OpenUserManager_ErrorText;
	public static String SelectUserDialog_AvailableUsers;
	public static String SelectUserDialog_LoginName;
	public static String SelectUserDialog_Title;
	public static String SelectUserDialog_Warning;
	public static String SelectUserDialog_WarningText;
	public static String UserLabelProvider_Group;
	public static String UserLabelProvider_User;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
