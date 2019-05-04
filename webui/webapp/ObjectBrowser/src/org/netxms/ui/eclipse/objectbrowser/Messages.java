package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectbrowser.messages"; //$NON-NLS-1$
	public String ChildObjectListDialog_Filter;
	public String ChildObjectListDialog_Name;
	public String ChildObjectListDialog_SelectSubordinate;
	public String CreateObjectDialog_ObjectName;
	public String CreateObjectDialog_Title;
	public String CreateObjectDialog_Warning;
	public String CreateObjectDialog_WarningText;
	public String IPAddressSelectionDialog_Interface;
	public String IPAddressSelectionDialog_IPAddress;
	public String IPAddressSelectionDialog_Title;
	public String IPAddressSelectionDialog_Warning;
	public String IPAddressSelectionDialog_WarningText;
	public String IPAddressSelector_None;
   public String ObjectBrowser_HideCheckTemplates;
   public String ObjectBrowser_HideSubInterfaces;
	public String ObjectBrowser_HideUnmanaged;
	public String ObjectBrowser_MoveJob_Error;
	public String ObjectBrowser_MoveJob_Title;
	public String ObjectBrowser_MoveObject;
	public String ObjectBrowser_MoveService;
	public String ObjectBrowser_MoveTemplate;
	public String ObjectBrowser_ShowFilter;
	public String ObjectBrowser_ShowStatusIndicator;
	public String ObjectList_Add;
	public String ObjectList_Delete;
	public String ObjectSelectionDialog_SameObjecsAstargetAndSourceWarning;
   public String ObjectSelectionDialog_Title;
	public String ObjectSelectionDialog_Warning;
	public String ObjectSelectionDialog_WarningText;
	public String ObjectSelector_None;
	public String ObjectStatusIndicator_HideDisabled;
	public String ObjectStatusIndicator_HideNormal;
	public String ObjectStatusIndicator_HideUnknown;
	public String ObjectStatusIndicator_HideUnmanaged;
	public String ObjectStatusIndicator_ShowIcons;
	public String ObjectTree_JobTitle;
   public String ObjectBrowser_MoveDashboard;
   public String ObjectBrowser_MoveMap;
   public String ObjectBrowser_MovePolicy;
   public String ObjectBrowser_Rename;
   public String ObjectBrowser_RenameJobError;
   public String ObjectBrowser_RenameJobName;
   public String ObjectDecorator_MaintenanceSuffix;
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
