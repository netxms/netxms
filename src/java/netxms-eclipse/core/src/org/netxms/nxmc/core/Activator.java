package org.netxms.nxmc.core;

import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.client.NXCSession;
import org.osgi.framework.BundleContext;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin
{
	// The plug-in ID
	public static final String PLUGIN_ID = "org.netxms.nxmc.core";

	// The shared instance
	private static Activator plugin;
	
	// Shared data
	private NXCSession session;
	private StatusLineContributionItem statusItemConnection;
	
	/**
	 * The constructor
	 */
	public Activator()
	{
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext context) throws Exception
	{
		super.start(context);
		plugin = this;
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext context) throws Exception
	{
		plugin = null;
		super.stop(context);
	}

	/**
	 * Returns the shared instance
	 *
	 * @return the shared instance
	 */
	public static Activator getDefault()
	{
		return plugin;
	}

	/**
	 * Returns an image descriptor for the image file at the given
	 * plug-in relative path
	 *
	 * @param path the path
	 * @return the image descriptor
	 */
	public static ImageDescriptor getImageDescriptor(String path)
	{
		return imageDescriptorFromPlugin(PLUGIN_ID, path);
	}

	/**
	 * @return the session
	 */
	public NXCSession getSession()
	{
		return session;
	}

	/**
	 * @param session the session to set
	 */
	public void setSession(NXCSession session)
	{
		this.session = session;
	}

	/**
	 * @return the statusItemConnection
	 */
	public StatusLineContributionItem getStatusItemConnection()
	{
		return statusItemConnection;
	}

	/**
	 * @param statusItemConnection the statusItemConnection to set
	 */
	public void setStatusItemConnection(
			StatusLineContributionItem statusItemConnection)
	{
		this.statusItemConnection = statusItemConnection;
	}
}
