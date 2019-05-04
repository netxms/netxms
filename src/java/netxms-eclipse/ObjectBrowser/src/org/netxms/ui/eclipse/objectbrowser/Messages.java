package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectbrowser.messages"; //$NON-NLS-1$
   public static String ChildObjectListDialog_Filter;
   public static String ChildObjectListDialog_Name;
   public static String ChildObjectListDialog_SelectSubordinate;
   public static String CreateObjectDialog_ObjectName;
   public static String CreateObjectDialog_Title;
   public static String CreateObjectDialog_Warning;
   public static String CreateObjectDialog_WarningText;
   public static String IPAddressSelectionDialog_Interface;
   public static String IPAddressSelectionDialog_IPAddress;
   public static String IPAddressSelectionDialog_Title;
   public static String IPAddressSelectionDialog_Warning;
   public static String IPAddressSelectionDialog_WarningText;
   public static String IPAddressSelector_None;
   public static String ObjectBrowser_HideCheckTemplates;
   public static String ObjectBrowser_HideSubInterfaces;
   public static String ObjectBrowser_HideUnmanaged;
   public static String ObjectBrowser_MoveDashboard;
   public static String ObjectBrowser_MoveJob_Error;
   public static String ObjectBrowser_MoveJob_Title;
   public static String ObjectBrowser_MoveMap;
   public static String ObjectBrowser_MoveObject;
   public static String ObjectBrowser_MovePolicy;
   public static String ObjectBrowser_MoveService;
   public static String ObjectBrowser_MoveTemplate;
   public static String ObjectBrowser_Rename;
   public static String ObjectBrowser_RenameJobError;
   public static String ObjectBrowser_RenameJobName;
   public static String ObjectBrowser_ShowFilter;
   public static String ObjectBrowser_ShowStatusIndicator;
   public static String ObjectDecorator_MaintenanceSuffix;
   public static String ObjectList_Add;
   public static String ObjectList_Delete;
   public static String ObjectSelectionDialog_SameObjecsAstargetAndSourceWarning;
   public static String ObjectSelectionDialog_Title;
   public static String ObjectSelectionDialog_Warning;
   public static String ObjectSelectionDialog_WarningText;
   public static String ObjectSelector_None;
   public static String ObjectStatusIndicator_HideDisabled;
   public static String ObjectStatusIndicator_HideNormal;
   public static String ObjectStatusIndicator_HideUnknown;
   public static String ObjectStatusIndicator_HideUnmanaged;
   public static String ObjectStatusIndicator_ShowIcons;
   public static String ObjectTree_JobTitle;
   public static String TrustedNodes_Add;
   public static String TrustedNodes_Delete;
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
