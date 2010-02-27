/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.shared;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.library.Activator;

/**
 * Status display information
 *
 */
public final class StatusDisplayInfo
{
	private static String[] statusText = new String[9];
	private static ImageDescriptor[] statusImage = new ImageDescriptor[9];
	private static Color[] statusColor = new Color[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		statusText[Severity.NORMAL] = "Normal";
		statusText[Severity.WARNING] = "Warning";
		statusText[Severity.MINOR] = "Minor";
		statusText[Severity.MAJOR] = "Major";
		statusText[Severity.CRITICAL] = "Critical";
		statusText[Severity.UNKNOWN] = "Unknown";
		statusText[Severity.UNMANAGED] = "Unmanaged";
		statusText[Severity.DISABLED] = "Disabled";
		statusText[Severity.TESTING] = "Testing";

		statusImage[Severity.NORMAL] = Activator.getImageDescriptor("icons/status/normal.png");
		statusImage[Severity.WARNING] = Activator.getImageDescriptor("icons/status/warning.png");
		statusImage[Severity.MINOR] = Activator.getImageDescriptor("icons/status/minor.png");
		statusImage[Severity.MAJOR] = Activator.getImageDescriptor("icons/status/major.png");
		statusImage[Severity.CRITICAL] = Activator.getImageDescriptor("icons/status/critical.png");
		statusImage[Severity.UNKNOWN] = Activator.getImageDescriptor("icons/status/unknown.png");
		statusImage[Severity.UNMANAGED] = Activator.getImageDescriptor("icons/status/unmanaged.png");
		statusImage[Severity.DISABLED] = Activator.getImageDescriptor("icons/status/disabled.png");
		statusImage[Severity.TESTING] = Activator.getImageDescriptor("icons/status/testing.png");

		Display display = Display.getDefault();
		statusColor[Severity.NORMAL] = new Color(display, 0, 192, 0);
		statusColor[Severity.WARNING] = new Color(display, 0, 255, 255);
		statusColor[Severity.MINOR] = new Color(display, 255, 255, 0);
		statusColor[Severity.MAJOR] = new Color(display, 255, 128, 0);
		statusColor[Severity.CRITICAL] = new Color(display, 192, 0, 0);
		statusColor[Severity.UNKNOWN] = new Color(display, 0, 0, 128);
		statusColor[Severity.UNMANAGED] = new Color(display, 192, 192, 192);
		statusColor[Severity.DISABLED] = new Color(display, 0, 0, 0);
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
			return "<unknown>";
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
