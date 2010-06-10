/**
 * 
 */
package org.netxms.ui.eclipse.shared;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.swt.widgets.TrayItem;
import org.netxms.client.NXCSession;

/**
 * Shared data for NXMC extensions
 * 
 * @author Victor
 *
 */
public class NXMCSharedData
{
	private static NXMCSharedData instance = new NXMCSharedData();
	private NXCSession session = null;
	private TrayItem trayIcon = null;
	private Map<String, Object> consoleProperties = new HashMap<String, Object>(0);
	
	/**
	 * Private constructor
	 */
	private NXMCSharedData()
	{
	}
	
	
	/**
	 * Get instance of shared data object
	 * 
	 * @return Shared data instance
	 */
	public static NXMCSharedData getInstance()
	{
		return instance;
	}
	
	/**
	 * Get current NetXMS client library session
	 * 
	 * @return Current session
	 */
	public NXCSession getSession()
	{
		return session;
	}

	/**
	 * Set current NetXMS client library session
	 * 
	 * @param session Current session
	 */
	public void setSession(NXCSession session)
	{
		this.session = session;
	}
	
	/**
	 * Get value of console property
	 * 
	 * @param name name of the property
	 * @return property value or null
	 */
	public Object getProperty(final String name)
	{
		return consoleProperties.get(name);
	}
	
	/**
	 * Set value of console property
	 * 
	 * @param name name of the property
	 * @param value new property value
	 */
	public void setProperty(final String name, final Object value)
	{
		consoleProperties.put(name, value);
	}


	/**
	 * @return the trayIcon
	 */
	public TrayItem getTrayIcon()
	{
		return trayIcon;
	}


	/**
	 * @param trayIcon the trayIcon to set
	 */
	public void setTrayIcon(TrayItem trayIcon)
	{
		this.trayIcon = trayIcon;
	}
}
