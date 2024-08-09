/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.resources;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.Severity;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorCache;
import org.xnap.commons.i18n.I18n;

/**
 * Status display information
 */
public final class StatusDisplayInfo
{
   private static I18n i18n = LocalizationHelper.getI18n(StatusDisplayInfo.class);
	private static String[] statusText = new String[9];
	private static ImageDescriptor[] statusImageDescriptors = new ImageDescriptor[9];
	private static Image[] statusImages = new Image[9];
   private static ImageDescriptor[] overlayImageDescriptors = new ImageDescriptor[9];
	private static ColorCache colorCache;
	private static Color statusColor[] = new Color[9]; 
   private static Color statusBackgroundColor[] = new Color[9];

	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init(Display display)
	{
      statusText[ObjectStatus.NORMAL.getValue()] = i18n.tr("Normal");
      statusText[ObjectStatus.WARNING.getValue()] = i18n.tr("Warning");
      statusText[ObjectStatus.MINOR.getValue()] = i18n.tr("Minor");
      statusText[ObjectStatus.MAJOR.getValue()] = i18n.tr("Major");
      statusText[ObjectStatus.CRITICAL.getValue()] = i18n.tr("Critical");
      statusText[ObjectStatus.UNKNOWN.getValue()] = i18n.tr("Unknown");
      statusText[ObjectStatus.UNMANAGED.getValue()] = i18n.tr("Unmanaged");
      statusText[ObjectStatus.DISABLED.getValue()] = i18n.tr("Disabled");
      statusText[ObjectStatus.TESTING.getValue()] = i18n.tr("Testing");

      statusImageDescriptors[ObjectStatus.NORMAL.getValue()] = ResourceManager.getImageDescriptor("icons/status/normal.png");
      statusImageDescriptors[ObjectStatus.WARNING.getValue()] = ResourceManager.getImageDescriptor("icons/status/warning.png");
      statusImageDescriptors[ObjectStatus.MINOR.getValue()] = ResourceManager.getImageDescriptor("icons/status/minor.png");
      statusImageDescriptors[ObjectStatus.MAJOR.getValue()] = ResourceManager.getImageDescriptor("icons/status/major.png");
      statusImageDescriptors[ObjectStatus.CRITICAL.getValue()] = ResourceManager.getImageDescriptor("icons/status/critical.png");
      statusImageDescriptors[ObjectStatus.UNKNOWN.getValue()] = ResourceManager.getImageDescriptor("icons/status/unknown.png");
      statusImageDescriptors[ObjectStatus.UNMANAGED.getValue()] = ResourceManager.getImageDescriptor("icons/status/unmanaged.png");
      statusImageDescriptors[ObjectStatus.DISABLED.getValue()] = ResourceManager.getImageDescriptor("icons/status/disabled.png");
      statusImageDescriptors[ObjectStatus.TESTING.getValue()] = ResourceManager.getImageDescriptor("icons/status/testing.png");

		for(int i = 0; i < statusImageDescriptors.length; i++)
			statusImages[i] = statusImageDescriptors[i].createImage();

		overlayImageDescriptors[ObjectStatus.WARNING.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/warning.png");
		overlayImageDescriptors[ObjectStatus.MINOR.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/minor.png");
		overlayImageDescriptors[ObjectStatus.MAJOR.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/major.png");
		overlayImageDescriptors[ObjectStatus.CRITICAL.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/critical.png");
		overlayImageDescriptors[ObjectStatus.UNKNOWN.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/unknown.gif");
		overlayImageDescriptors[ObjectStatus.UNMANAGED.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/unmanaged.gif");
		overlayImageDescriptors[ObjectStatus.DISABLED.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/disabled.gif");
		overlayImageDescriptors[ObjectStatus.TESTING.getValue()] = ResourceManager.getImageDescriptor("icons/status/overlay/testing.png");
		
		colorCache = new ColorCache();
		updateStatusColors();
	}

	/**
	 * Update status colors
	 */
	public static void updateStatusColors()
	{
      PreferenceStore ps = PreferenceStore.getInstance();

      statusColor[0] = colorCache.create(ps.getAsColor("Status.Colors.Normal", ThemeEngine.getForegroundColorDefinition("Status.Normal")));
      statusColor[1] = colorCache.create(ps.getAsColor("Status.Colors.Warning", ThemeEngine.getForegroundColorDefinition("Status.Warning")));
      statusColor[2] = colorCache.create(ps.getAsColor("Status.Colors.Minor", ThemeEngine.getForegroundColorDefinition("Status.Minor")));
      statusColor[3] = colorCache.create(ps.getAsColor("Status.Colors.Major", ThemeEngine.getForegroundColorDefinition("Status.Major")));
      statusColor[4] = colorCache.create(ps.getAsColor("Status.Colors.Critical", ThemeEngine.getForegroundColorDefinition("Status.Critical")));
      statusColor[5] = colorCache.create(ps.getAsColor("Status.Colors.Unknown", ThemeEngine.getForegroundColorDefinition("Status.Unknown")));
      statusColor[6] = colorCache.create(ps.getAsColor("Status.Colors.Unmanaged", ThemeEngine.getForegroundColorDefinition("Status.Unmanaged")));
      statusColor[7] = colorCache.create(ps.getAsColor("Status.Colors.Disabled", ThemeEngine.getForegroundColorDefinition("Status.Disabled")));
      statusColor[8] = colorCache.create(ps.getAsColor("Status.Colors.Testing", ThemeEngine.getForegroundColorDefinition("Status.Testing")));

      statusBackgroundColor[0] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Normal", ThemeEngine.getBackgroundColorDefinition("Status.Normal")));
      statusBackgroundColor[1] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Warning", ThemeEngine.getBackgroundColorDefinition("Status.Warning")));
      statusBackgroundColor[2] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Minor", ThemeEngine.getBackgroundColorDefinition("Status.Minor")));
      statusBackgroundColor[3] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Major", ThemeEngine.getBackgroundColorDefinition("Status.Major")));
      statusBackgroundColor[4] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Critical", ThemeEngine.getBackgroundColorDefinition("Status.Critical")));
      statusBackgroundColor[5] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Unknown", ThemeEngine.getBackgroundColorDefinition("Status.Unknown")));
      statusBackgroundColor[6] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Unmanaged", ThemeEngine.getBackgroundColorDefinition("Status.Unmanaged")));
      statusBackgroundColor[7] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Disabled", ThemeEngine.getBackgroundColorDefinition("Status.Disabled")));
      statusBackgroundColor[8] = colorCache.create(ps.getAsColor("Status.BackgroundColors.Testing", ThemeEngine.getBackgroundColorDefinition("Status.Testing")));
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
		return statusImageDescriptors[status.getValue()];
	}

   /**
    * Get image descriptor for given status/severity code.
    * 
    * @param severity Severity code
    * @return Image descriptor for given code
    */
   public static ImageDescriptor getStatusImageDescriptor(Severity severity)
   {
      return statusImageDescriptors[severity.getValue()];
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
		return statusImages[status.getValue()];
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
      return statusImages[severity.getValue()];
   }

   /**
    * Get overlay image descriptor for given status/severity code.
    * 
    * @param status Status code
    * @return Overlay image descriptor for given code
    */
   public static ImageDescriptor getStatusOverlayImageDescriptor(ObjectStatus status)
   {
      return overlayImageDescriptors[status.getValue()];
   }

   /**
    * Get overlay image descriptor for given status/severity code.
    * 
    * @param severity Severity code
    * @return Overlay image descriptor for given code
    */
   public static ImageDescriptor getStatusOverlayImageDescriptor(Severity severity)
   {
      return overlayImageDescriptors[severity.getValue()];
   }
   
   /**
    * Get image descriptor for given status/severity code.
    * 
    * @param code Status or severity code
    * @return Overlay image descriptor for given code
    */
   public static ImageDescriptor getStatusOverlayImageDescriptor(int code)
   {
      return getStatusOverlayImageDescriptor(ObjectStatus.getByValue(code));
   }

	/**
	 * Get color for given status/severity code.
	 * 
	 * @param status Status code
	 * @return Color for given code
	 */
	public static Color getStatusColor(ObjectStatus status)
	{
		return statusColor[status.getValue()];
	}

   /**
    * Get color for given status/severity code.
    * 
    * @param severity Severity code
    * @return Color for given code
    */
   public static Color getStatusColor(Severity severity)
   {
      return statusColor[severity.getValue()];
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

   /**
    * Get background color for given status/severity code.
    * 
    * @param status Status code
    * @return Color for given code
    */
   public static Color getStatusBackgroundColor(ObjectStatus status)
   {
      Color color = statusBackgroundColor[status.getValue()];
      return (color != null) ? color : getStatusColor(status);
   }

   /**
    * Get background color for given status/severity code.
    * 
    * @param severity Severity code
    * @return Color for given code
    */
   public static Color getStatusBackgroundColor(Severity severity)
   {
      Color color = statusBackgroundColor[severity.getValue()];
      return (color != null) ? color : getStatusColor(severity);
   }

   /**
    * Get background color for given status/severity code.
    * 
    * @param code Status or severity code
    * @return Color for given code
    */
   public static Color getStatusBackgroundColor(int code)
   {
      return getStatusBackgroundColor(ObjectStatus.getByValue(code));
   }
}
