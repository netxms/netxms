/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Shell;

/**
 * Branding provider interface
 */
public interface BrandingProvider
{
   /**
    * Get description for this branding provider.
    *
    * @return description for this branding provider
    */
   public String getDescription();

	/**
    * Get common product name.
    * 
    * @return product name or null to use default
    */
	public String getProductName();

	/**
    * Get product name for management client.
    * 
    * @return product name for managemebt client or null to use default
    */
   public String getClientProductName();

   /**
    * Get image for login dialog (form).
    *
    * @return custom image for login dialog or null to use default
    */
   public ImageDescriptor getLoginImage();

   /**
    * Get background color for login image (only for web UI).
    *
    * @return background color for login image or null to use default
    */
   public RGB getLoginImageBackground();

   /**
    * Get image for application header.
    *
    * @return custom image for application header or null to use default
    */
   public ImageDescriptor getAppHeaderImage();

   /**
    * Get background color for application header.
    *
    * @return background color for application header or null to use default
    */
   public RGB getAppHeaderBackground();

	/**
	 * Get default perspective. Should return null to use default (or defined by another branding manager)
	 * 
	 * @return default perspective ID or null to use default.
	 */
	public String getDefaultPerspective();

	/**
    * Create custom "About" dialog. New dialog must be returned on each call.
    * 
    * @param parentShell parent shell for dialog
    * @return custom "About" dialog or null to use default
    */
   public Dialog createAboutDialog(Shell parentShell);
}
