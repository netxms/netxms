/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.client.constants.ObjectStatus;
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
		statusText[ObjectStatus.NORMAL.getValue()] = Messages.get().StatusDisplayInfo_Normal;
		statusText[ObjectStatus.WARNING.getValue()] = Messages.get().StatusDisplayInfo_Warning;
		statusText[ObjectStatus.MINOR.getValue()] = Messages.get().StatusDisplayInfo_Minor;
		statusText[ObjectStatus.MAJOR.getValue()] = Messages.get().StatusDisplayInfo_Major;
		statusText[ObjectStatus.CRITICAL.getValue()] = Messages.get().StatusDisplayInfo_Critical;
		statusText[ObjectStatus.UNKNOWN.getValue()] = Messages.get().StatusDisplayInfo_Unknown;
		statusText[ObjectStatus.UNMANAGED.getValue()] = Messages.get().StatusDisplayInfo_Unmanaged;
		statusText[ObjectStatus.DISABLED.getValue()] = Messages.get().StatusDisplayInfo_Disabled;
		statusText[ObjectStatus.TESTING.getValue()] = Messages.get().StatusDisplayInfo_Testing;

		statusImageDesc[ObjectStatus.NORMAL.getValue()] = Activator.getImageDescriptor("icons/status/normal.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.WARNING.getValue()] = Activator.getImageDescriptor("icons/status/warning.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.MINOR.getValue()] = Activator.getImageDescriptor("icons/status/minor.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.MAJOR.getValue()] = Activator.getImageDescriptor("icons/status/major.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.CRITICAL.getValue()] = Activator.getImageDescriptor("icons/status/critical.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.UNKNOWN.getValue()] = Activator.getImageDescriptor("icons/status/unknown.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.UNMANAGED.getValue()] = Activator.getImageDescriptor("icons/status/unmanaged.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.DISABLED.getValue()] = Activator.getImageDescriptor("icons/status/disabled.png"); //$NON-NLS-1$
		statusImageDesc[ObjectStatus.TESTING.getValue()] = Activator.getImageDescriptor("icons/status/testing.png"); //$NON-NLS-1$
		
		for(int i = 0; i < statusImageDesc.length; i++)
			statusImage[i] = statusImageDesc[i].createImage();

		statusColor[ObjectStatus.NORMAL.getValue()] = SharedColors.STATUS_NORMAL;
		statusColor[ObjectStatus.WARNING.getValue()] = SharedColors.STATUS_WARNING;
		statusColor[ObjectStatus.MINOR.getValue()] = SharedColors.STATUS_MINOR;
		statusColor[ObjectStatus.MAJOR.getValue()] = SharedColors.STATUS_MAJOR;
		statusColor[ObjectStatus.CRITICAL.getValue()] = SharedColors.STATUS_CRITICAL;
		statusColor[ObjectStatus.UNKNOWN.getValue()] = SharedColors.STATUS_UNKNOWN;
		statusColor[ObjectStatus.UNMANAGED.getValue()] = SharedColors.STATUS_UNMANAGED;
		statusColor[ObjectStatus.DISABLED.getValue()] = SharedColors.STATUS_DISABLED;
		statusColor[ObjectStatus.TESTING.getValue()] = SharedColors.STATUS_TESTING;
	}
	
	/**
	 * Get text for given status/severity code.
	 * 
	 * @param status Status code
	 * @return Text for given code
	 */
	public static String getStatusText(ObjectStatus status)
	{
		return statusText[status.getValue()];
	}
	
   /**
    * Get text for given status/severity code.
    * 
    * @param severity Severity code
    * @return Text for given code
    */
   public static String getStatusText(Severity severity)
   {
      return statusText[severity.getValue()];
   }
   
   /**
    * Get text for given status/severity code.
    * 
    * @param code Status or severity code
    * @return Text for given code
    */
   public static String getStatusText(int code)
   {
      return getStatusText(ObjectStatus.getByValue(code));
   }
   
	/**
	 * Get image descriptor for given status/severity code.
	 * 
	 * @param status Status code
	 * @return Image descriptor for given code
	 */
	public static ImageDescriptor getStatusImageDescriptor(ObjectStatus status)
	{
		return statusImageDesc[status.getValue()];
	}
	
   /**
    * Get image descriptor for given status/severity code.
    * 
    * @param severity Severity code
    * @return Image descriptor for given code
    */
   public static ImageDescriptor getStatusImageDescriptor(Severity severity)
   {
      return statusImageDesc[severity.getValue()];
   }
   
   /**
    * Get image descriptor for given status/severity code.
    * 
    * @param code Status or severity code
    * @return Image descriptor for given code
    */
   public static ImageDescriptor getStatusImageDescriptor(int code)
   {
      return getStatusImageDescriptor(ObjectStatus.getByValue(code));
   }
   
	/**
	 * Get image for given status/severity code. Image is owned by library
	 * and should not be disposed by caller.
	 * 
	 * @param status Status code
	 * @return Image descriptor for given code
	 */
	public static Image getStatusImage(ObjectStatus status)
	{
		return statusImage[status.getValue()];
	}
	
   /**
    * Get image for given status/severity code. Image is owned by library
    * and should not be disposed by caller.
    * 
    * @param code Status or severity code
    * @return Image descriptor for given code
    */
   public static Image getStatusImage(int code)
   {
      return getStatusImage(ObjectStatus.getByValue(code));
   }
   
   /**
    * Get image for given status/severity code. Image is owned by library
    * and should not be disposed by caller.
    * 
    * @param severity Severity code
    * @return Image descriptor for given code
    */
   public static Image getStatusImage(Severity severity)
   {
      return statusImage[severity.getValue()];
   }
   
	/**
	 * Get color for given status/severity code.
	 * 
	 * @param status Status code
	 * @return Color for given code
	 */
	public static Color getStatusColor(ObjectStatus status)
	{
		return SharedColors.getColor(statusColor[status.getValue()], Display.getCurrent());
	}
   
   /**
    * Get color for given status/severity code.
    * 
    * @param severity Severity code
    * @return Color for given code
    */
   public static Color getStatusColor(Severity severity)
   {
      return SharedColors.getColor(statusColor[severity.getValue()], Display.getCurrent());
   }
   
   /**
    * Get color for given status/severity code.
    * 
    * @param code Status or severity code
    * @return Color for given code
    */
   public static Color getStatusColor(int code)
   {
      return getStatusColor(ObjectStatus.getByValue(code));
   }
}
