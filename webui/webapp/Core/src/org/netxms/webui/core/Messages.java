package org.netxms.webui.core;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.webui.core.messages"; //$NON-NLS-1$
	public static String AbstractSelector_CopyToClipboard;
	public static String AbstractSelector_Select;
	public static String ConsoleJob_ErrorDialogTitle;
	public static String FilterText_CloseFilter;
	public static String FilterText_Filter;
	public static String FilterText_FilterIsEmpty;
	public static String RefreshAction_Name;
	public static String WidgetHelper_Action_Copy;
	public static String WidgetHelper_Action_Cut;
	public static String WidgetHelper_Action_Delete;
	public static String WidgetHelper_Action_Paste;
	public static String WidgetHelper_Action_SelectAll;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
