package org.netxms.ui.eclipse.serverconsole;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.serverconsole.messages"; //$NON-NLS-1$
	
	public static String OpenServerConsole_Error;
	public static String OpenServerConsole_JobTitle;
	public static String OpenServerConsole_OpenErrorMessage;
	public static String OpenServerConsole_ViewErrorMessage;
   public static String ServerConsole_CannotOpen;
	public static String ServerConsole_ClearTerminal;
	public static String ServerConsole_OpenServerConsole;
	public static String ServerConsole_ScrollLock;
	
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
