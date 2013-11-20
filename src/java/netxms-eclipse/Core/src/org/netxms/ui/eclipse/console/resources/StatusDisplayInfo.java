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
 */
public final class StatusDisplayInfo
{
	private static String[] statusText = new String[9];
	private static ImageDescriptor[] statusImageDesc = new ImageDescriptor[9];
	private static Image[] statusImage = new Image[9];
	private static String[] statusColor = new String[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init(Display display)
	{
		statusText[Severity.NORMAL] = Messages.get().StatusDisplayInfo_Normal;
		statusText[Severity.WARNING] = Messages.get().StatusDisplayInfo_Warning;
		statusText[Severity.MINOR] = Messages.get().StatusDisplayInfo_Minor;
		statusText[Severity.MAJOR] = Messages.get().StatusDisplayInfo_Major;
		statusText[Severity.CRITICAL] = Messages.get().StatusDisplayInfo_Critical;
		statusText[Severity.UNKNOWN] = Messages.get().StatusDisplayInfo_Unknown;
		statusText[Severity.UNMANAGED] = Messages.get().StatusDisplayInfo_Unmanaged;
		statusText[Severity.DISABLED] = Messages.get().StatusDisplayInfo_Disabled;
		statusText[Severity.TESTING] = Messages.get().StatusDisplayInfo_Testing;

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

		statusColor[Severity.NORMAL] = SharedColors.STATUS_NORMAL;
		statusColor[Severity.WARNING] = SharedColors.STATUS_WARNING;
		statusColor[Severity.MINOR] = SharedColors.STATUS_MINOR;
		statusColor[Severity.MAJOR] = SharedColors.STATUS_MAJOR;
		statusColor[Severity.CRITICAL] = SharedColors.STATUS_CRITICAL;
		statusColor[Severity.UNKNOWN] = SharedColors.STATUS_UNKNOWN;
		statusColor[Severity.UNMANAGED] = SharedColors.STATUS_UNMANAGED;
		statusColor[Severity.DISABLED] = SharedColors.STATUS_DISABLED;
		statusColor[Severity.TESTING] = SharedColors.STATUS_TESTING;
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
			return SharedColors.getColor(statusColor[severity], Display.getCurrent());
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}
}
