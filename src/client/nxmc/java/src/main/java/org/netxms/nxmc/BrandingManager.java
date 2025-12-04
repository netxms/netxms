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

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.dialogs.AboutDialog;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Branding manager. There should be only one instance of branding manager,
 * created early during application startup.
 */
public final class BrandingManager
{
   private static final Logger logger = LoggerFactory.getLogger(BrandingManager.class);
   private static final List<BrandingProvider> providers = new ArrayList<BrandingProvider>(0);

   static
   {
      ServiceLoader<BrandingProvider> loader = ServiceLoader.load(BrandingProvider.class, BrandingManager.class.getClassLoader());
      for(BrandingProvider p : loader)
      {
         logger.info("Registered branding provider \"" + p.getDescription() + "\" (" + p.getClass().getName() + ")");
         providers.add(p);
      }
      logger.info("Branding manager initialized " + (providers.isEmpty() ? "without" : "with") + " custom providers");
   }

	/**
	 * Get product name.
	 * 
	 * @return product name or default product name if no branding provider defines one.
	 */
   public static String getProductName()
	{
      for(BrandingProvider p : providers)
		{
			String name = p.getProductName();
			if (name != null)
				return name;
		}
      return "NetXMS";
	}

   /**
    * Get prefix for window icon resources.
    * 
    * @return prefix for window icon resources
    */
   public static String getWindowIconResourcePrefix()
   {
      for(BrandingProvider p : providers)
      {
         String prefix = p.getWindowIconResourcePrefix();
         if (prefix != null)
            return prefix;
      }
      return "icons/window/";
   }

   /**
    * Get product name for management client.
    * 
    * @return product name or default product name if no branding provider defines one.
    */
   public static String getClientProductName()
   {
      for(BrandingProvider p : providers)
      {
         String name = p.getClientProductName();
         if (name != null)
            return name;
      }
      final I18n i18n = LocalizationHelper.getI18n(BrandingManager.class);
      return i18n.tr("NetXMS Management Client");
	}

   /**
    * Get login image.
    *
    * @return login image
    */
   public static ImageDescriptor getLoginImage()
   {
      for(BrandingProvider p : providers)
      {
         ImageDescriptor image = p.getLoginImage();
         if (image != null)
            return image;
      }
      return ResourceManager.getImageDescriptor(WidgetHelper.isSystemDarkTheme() ? "icons/login-dark-mode.png" : "icons/login.png");
   }

   /**
    * Get login image background.
    *
    * @return login image background
    */
   public static RGB getLoginImageBackground()
   {
      for(BrandingProvider p : providers)
      {
         RGB color = p.getLoginImageBackground();
         if (color != null)
            return color;
      }
      return new RGB(14, 50, 78);
   }

   /**
    * Get application header image.
    *
    * @return application header image
    */
   public static ImageDescriptor getAppHeaderImage()
   {
      for(BrandingProvider p : providers)
      {
         ImageDescriptor image = p.getAppHeaderImage();
         if (image != null)
            return image;
      }
      return ResourceManager.getImageDescriptor("icons/app-logo.png");
   }

   /**
    * Get application header background.
    *
    * @return application header background
    */
   public static RGB getAppHeaderBackground()
   {
      for(BrandingProvider p : providers)
      {
         RGB color = p.getAppHeaderBackground();
         if (color != null)
            return color;
      }
      return ThemeEngine.getBackgroundColorDefinition("Window.Header");
   }

	/**
	 * Get default perspective ID. 
	 * 
	 * @return default perspective ID or null if no branding provider defines one.
	 */
   public static String getDefaultPerspective()
	{
      for(BrandingProvider p : providers)
		{
			String pid = p.getDefaultPerspective();
			if (pid != null)
				return pid;
		}
		return null;
	}

	/**
    * Create "About" dialog.
    * 
    * @param parentShell parent shell for dialog
    * @return custom "About" dialog or default one if no branding provider defines one.
    */
   public static Dialog createAboutDialog(Shell parentShell)
	{
      for(BrandingProvider p : providers)
		{
         Dialog d = p.createAboutDialog(parentShell);
			if (d != null)
				return d;
		}
      return new AboutDialog(parentShell);
	}

   /**
    * Get URL of administrator guide.
    *
    * @return URL of administrator guide
    */
   public static String getAdministratorGuideURL()
   {
      for(BrandingProvider p : providers)
      {
         String url = p.getAdministratorGuideURL();
         if (url != null)
            return url;
      }
      return "https://go.netxms.com/adminguide-online";
   }

   /**
    * Control if extended help menu (with support options, etc.) is enabled.
    *
    * @return true if extended help menu is enabled
    */
   public static boolean isExtendedHelpMenuEnabled()
   {
      for(BrandingProvider p : providers)
      {
         Boolean enabled = p.isExtendedHelpMenuEnabled();
         if (enabled != null)
            return enabled;
      }
      return true;
   }

   /**
    * Control if welcome page is enabled.
    *
    * @return true if welcome page is enabled
    */
   public static boolean isWelcomePageEnabled()
   {
      for(BrandingProvider p : providers)
      {
         Boolean enabled = p.isWelcomePageEnabled();
         if (enabled != null)
            return enabled;
      }
      return true;
   }
}
