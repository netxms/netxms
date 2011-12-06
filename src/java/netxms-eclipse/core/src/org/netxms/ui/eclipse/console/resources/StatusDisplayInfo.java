/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.resources;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;

/**
 * Status display information
 *
 */
public final class StatusDisplayInfo
{
	private static String[] statusText = new String[9];
	private static ImageDescriptor[] statusImageDesc = new ImageDescriptor[9];
	private static Image[] statusImage = new Image[9];
	private static Color[] statusColor = new Color[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init(Display display)
	{
		statusText[Severity.NORMAL] = Messages.getString("StatusDisplayInfo.Normal"); //$NON-NLS-1$
		statusText[Severity.WARNING] = Messages.getString("StatusDisplayInfo.Warning"); //$NON-NLS-1$
		statusText[Severity.MINOR] = Messages.getString("StatusDisplayInfo.Minor"); //$NON-NLS-1$
		statusText[Severity.MAJOR] = Messages.getString("StatusDisplayInfo.Major"); //$NON-NLS-1$
		statusText[Severity.CRITICAL] = Messages.getString("StatusDisplayInfo.Critical"); //$NON-NLS-1$
		statusText[Severity.UNKNOWN] = Messages.getString("StatusDisplayInfo.Unknown"); //$NON-NLS-1$
		statusText[Severity.UNMANAGED] = Messages.getString("StatusDisplayInfo.Unmanaged"); //$NON-NLS-1$
		statusText[Severity.DISABLED] = Messages.getString("StatusDisplayInfo.Disabled"); //$NON-NLS-1$
		statusText[Severity.TESTING] = Messages.getString("StatusDisplayInfo.Testing"); //$NON-NLS-1$

		statusImageDesc[Severity.NORMAL] = Activator.getImageDescriptor("icons/status/normal.png"); //$NON-NLS-1$
		statusImageDesc[Severity.WARNING] = Activator.getImageDescriptor("icons/status/warning.png"); //$NON-NLS-1$
		statusImageDesc[Severity.MINOR] = Activator.getImageDescriptor("icons/status/minor.png"); //$NON-NLS-1$
		statusImageDesc[Severity.MAJOR] = Activator.getImageDescriptor("icons/status/major.png"); //$NON-NLS-1$
		statusImageDesc[Severity.CRITICAL] = Activator.getImageDescriptor("icons/status/critical.png"); //$NON-NLS-1$
		statusImageDesc[Severity.UNKNOWN] = Activator.getImageDescriptor("icons/status/unknown.png"); //$NON-NLS-1$
		statusImageDesc[Severity.UNMANAGED] = Activator.getImageDescriptor("icons/status/unmanaged.png"); //$NON-NLS-1$
		statusImageDesc[Severity.DISABLED] = Activator.getImageDescriptor("icons/status/disabled.png"); //$NON-NLS-1$
		statusImageDesc[Severity.TESTING] = Activator.getImageDescriptor("icons/status/testing.png"); //$NON-NLS-1$
		
		for(int i = 0; i < statusImageDesc.length; i++)
			statusImage[i] = statusImageDesc[i].createImage();

		statusColor[Severity.NORMAL] = new Color(display, 0, 192, 0);
		statusColor[Severity.WARNING] = new Color(display, 0, 255, 255);
		statusColor[Severity.MINOR] = new Color(display, 231, 226, 0);
		statusColor[Severity.MAJOR] = new Color(display, 255, 128, 0);
		statusColor[Severity.CRITICAL] = new Color(display, 192, 0, 0);
		statusColor[Severity.UNKNOWN] = new Color(display, 0, 0, 128);
		statusColor[Severity.UNMANAGED] = new Color(display, 192, 192, 192);
		statusColor[Severity.DISABLED] = new Color(display, 128, 64, 0);
		statusColor[Severity.TESTING] = new Color(display, 255, 128, 255);
	}
	
	/**
	 * Get text for given status/severity code.
	 * 
	 * @param severity Status or severity code
	 * @return Text for given code
	 */
	public static String getStatusText(int severity)
	{
		try
		{
			return statusText[severity];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "<unknown>"; //$NON-NLS-1$
		}
	}
	
	/**
	 * Get image descriptor for given status/severity code.
	 * 
	 * @param severity Status or severity code
	 * @return Image descriptor for given code
	 */
	public static ImageDescriptor getStatusImageDescriptor(int severity)
	{
		try
		{
			return statusImageDesc[severity];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}
	
	/**
	 * Get image for given status/severity code. Image is owned by library
	 * and should not be disposed by caller.
	 * 
	 * @param severity Status or severity code
	 * @return Image descriptor for given code
	 */
	public static Image getStatusImage(int severity)
	{
		try
		{
			return statusImage[severity];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}
	
	/**
	 * Get color for given status/severity code.
	 * 
	 * @param severity Status or severity code
	 * @return Color for given code
	 */
	public static Color getStatusColor(int severity)
	{
		try
		{
			return statusColor[severity];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}
}
