/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Tray;
import org.eclipse.swt.widgets.TrayItem;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.osgi.framework.BundleContext;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin
{
	// The plug-in ID
	public static final String PLUGIN_ID = "org.netxms.ui.eclipse.console"; //$NON-NLS-1$

	// The shared instance
	private static Activator plugin;
	
	// Shared data
	private StatusLineContributionItem statusItemConnection;
	
	/*
	 * (non-Javadoc)
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext context) throws Exception
	{
		super.start(context);
		plugin = this;
		SharedIcons.init();
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
		if (ConsoleSharedData.getTrayIcon() != null)
			return;	// Tray icon already exist
		
		Tray tray = Display.getDefault().getSystemTray();
		if (tray != null)
		{
			TrayItem item = new TrayItem(tray, SWT.NONE);
			item.setToolTipText(Messages.Activator_TrayTooltip);
			item.setImage(getImageDescriptor("icons/launcher/16x16.png").createImage()); //$NON-NLS-1$
			item.addSelectionListener(new SelectionListener() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
				}

				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					final Shell shell = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell();
					shell.setVisible(true);
					shell.setMinimized(false);
				}
			});
			ConsoleSharedData.setTrayIcon(item);
		}
	}
	
	/**
	 * Hide tray icon
	 */
	public static void hideTrayIcon()
	{
		TrayItem item = ConsoleSharedData.getTrayIcon();
		if (item == null)
			return;	// No tray icon
		
		ConsoleSharedData.setTrayIcon(null);
		item.dispose();
	}

	/**
	 * Log via platform logging facilities
	 * 
	 * @param msg
	 */
	public static void logInfo(String msg)
	{
		log(Status.INFO, msg, null);
	}

	/**
	 * Log via platform logging facilities
	 * 
	 * @param msg
	 */
	public static void logError(String msg, Exception e)
	{
		log(Status.ERROR, msg, e);
	}

	/**
	 * Log via platform logging facilities
	 * 
	 * @param msg
	 * @param e
	 */
	public static void log(int status, String msg, Exception e)
	{
		getDefault().getLog().log(new Status(status, PLUGIN_ID, Status.OK, msg, e));
	}
}
