/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Tray;
import org.eclipse.swt.widgets.TrayItem;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.osgi.framework.BundleContext;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin
{
	// The plug-in ID
	public static final String PLUGIN_ID = "org.netxms.ui.eclipse.console";

	// The shared instance
	private static Activator plugin;
	
	// Shared data
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
	 * @return the statusItemConnection
	 */
	public StatusLineContributionItem getStatusItemConnection()
	{
		return statusItemConnection;
	}

	/**
	 * @param statusItemConnection the statusItemConnection to set
	 */
	public void setStatusItemConnection(StatusLineContributionItem statusItemConnection)
	{
		this.statusItemConnection = statusItemConnection;
	}
	
	/**
	 * Show tray icon
	 */
	public static void showTrayIcon()
	{
		if (NXMCSharedData.getInstance().getTrayIcon() != null)
			return;	// Tray icon already exist
		
		Tray tray = Display.getDefault().getSystemTray();
		if (tray != null)
		{
			TrayItem item = new TrayItem(tray, SWT.NONE);
			item.setToolTipText("NetXMS Management Console");
			item.setImage(getImageDescriptor("icons/alt_window_16.gif").createImage());
			NXMCSharedData.getInstance().setTrayIcon(item);
		}
	}
	
	/**
	 * Hide tray icon
	 */
	public static void hideTrayIcon()
	{
		TrayItem item = NXMCSharedData.getInstance().getTrayIcon();
		if (item == null)
			return;	// No tray icon
		
		NXMCSharedData.getInstance().setTrayIcon(null);
		item.dispose();
	}
}
