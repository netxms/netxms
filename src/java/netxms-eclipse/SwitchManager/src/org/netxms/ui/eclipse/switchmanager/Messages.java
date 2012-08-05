package org.netxms.ui.eclipse.switchmanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.switchmanager.messages"; //$NON-NLS-1$
	public static String Dot1xStatusView_ColBackend;
	public static String Dot1xStatusView_ColDevice;
	public static String Dot1xStatusView_ColInterface;
	public static String Dot1xStatusView_ColPAE;
	public static String Dot1xStatusView_ColSlotPort;
	public static String Dot1xStatusView_PartNamePrefix;
	public static String OpenDot1xStateView_Error;
	public static String OpenDot1xStateView_ErrorText;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
