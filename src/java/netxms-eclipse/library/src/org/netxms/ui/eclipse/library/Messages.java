package org.netxms.ui.eclipse.library;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.library.messages"; //$NON-NLS-1$
	public static String AbstractSelector_CopyToClipboard;
	public static String AbstractSelector_Select;
	public static String AbstractTraceView_Clear;
	public static String AbstractTraceView_CopyToClipboard;
	public static String AbstractTraceView_Pause;
	public static String AbstractTraceView_ShowFilter;
	public static String ConsoleJob_ErrorDialogTitle;
	public static String FilterText_CloseFilter;
	public static String FilterText_Filter;
	public static String FilterText_FilterIsEmpty;
	public static String NumericTextFieldValidator_ErrorMessage_Part1;
	public static String NumericTextFieldValidator_ErrorMessage_Part2;
	public static String NumericTextFieldValidator_ErrorMessage_Part3;
	public static String NumericTextFieldValidator_RangeSeparator;
	public static String RefreshAction_Name;
	public static String WidgetHelper_Action_Copy;
	public static String WidgetHelper_Action_Cut;
	public static String WidgetHelper_Action_Delete;
	public static String WidgetHelper_Action_Paste;
	public static String WidgetHelper_Action_SelectAll;
	public static String WidgetHelper_InputValidationError;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
