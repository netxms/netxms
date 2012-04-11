package org.netxms.ui.eclipse.logviewer;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.logviewer.messages"; //$NON-NLS-1$
	private static final ResourceBundle RESOURCE_BUNDLE = ResourceBundle.getBundle(BUNDLE_NAME);
	public static String LogViewer_AlarmLog;
	public static String LogViewer_AuditLog;
	public static String LogViewer_EventLog;
	public static String LogViewer_SnmpTrapLog;
	public static String LogViewer_syslog;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}

	public static String getString(String key)
	{
		try
		{
			return RESOURCE_BUNDLE.getString(key);
		}
		catch(MissingResourceException e)
		{
			return '!' + key + '!';
		}
	}
}
