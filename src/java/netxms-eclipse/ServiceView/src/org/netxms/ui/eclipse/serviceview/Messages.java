package org.netxms.ui.eclipse.serviceview;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.serviceview.messages"; //$NON-NLS-1$
	public static String OpenServiceTree_Error;
	public static String OpenServiceTree_OpenViewError;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}
}
