/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.Map;
import java.util.Properties;
import java.util.TreeMap;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.console.api.BrandingProvider;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.ui.eclipse.console.dialogs.DefaultLoginForm;
import org.netxms.ui.eclipse.console.dialogs.DefaultMobileLoginForm;

/**
 * Branding manager. There should be only one instance of branding manager,
 * created early during application startup.
 */
public class BrandingManager
{
	private static BrandingManager instance = null;
	
	/**
	 * Get branding manager instance.
	 * 
	 * @return
	 */
	public static BrandingManager getInstance()
	{
		return instance;
	}
	
	/**
	 * Create branding manager instance
	 */
	protected static synchronized void create()
	{
		if (instance == null)
			instance = new BrandingManager();
	}
	
	private Map<Integer, BrandingProvider> providers = new TreeMap<Integer, BrandingProvider>();
	
	/**
	 * Constructor
	 */
	private BrandingManager()
	{
		// Read all registered extensions and create branding providers
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.branding"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final BrandingProvider p = (BrandingProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				int priority = 65535;
				String value = elements[i].getAttribute("priority"); //$NON-NLS-1$
				if (value != null)
				{
					try
					{
						priority = Integer.parseInt(value);
						if (priority < 0)
							priority = 65535;
					}
					catch(NumberFormatException e)
					{
					}
				}
				providers.put(priority, p);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * Get product name.
	 * 
	 * @return product name or default product name if no branding provider defines one.
	 */
	public String getProductName()
	{
		for(BrandingProvider p : providers.values())
		{
			String name = p.getProductName();
			if (name != null)
				return name;
		}
		return Messages.get().ApplicationActionBarAdvisor_AboutProductName;
	}
	
	/**
	 * Get default perspective ID. 
	 * 
	 * @return default perspective ID or null if no branding provider defines one.
	 */
	public String getDefaultPerspective()
	{
		for(BrandingProvider p : providers.values())
		{
			String pid = p.getDefaultPerspective();
			if (pid != null)
				return pid;
		}
		return null;
	}
	
	/**
	 * Get image descriptor for login dialog title image.
	 * 
	 * @return image descriptor for login dialog title image or null if no branding provider defines one.
	 */
	public ImageDescriptor getLoginTitleImage()
	{
		for(BrandingProvider p : providers.values())
		{
			ImageDescriptor d = p.getLoginTitleImage();
			if (d != null)
				return d;
		}
		return null;
	}
	
	/**
	 * Get filler color for login dialog title image.
	 * 
	 * @return image descriptor for login dialog title image or null if no branding provider defines one.
	 */
	public RGB getLoginTitleColor()
	{
		for(BrandingProvider p : providers.values())
		{
			RGB rgb = p.getLoginTitleColor();
			if (rgb != null)
				return rgb;
		}
		return null;
	}

	/**
	 * Get default perspective ID. 
	 * 
	 * @return default perspective ID or null if no branding provider defines one.
	 */
	public String getLoginTitle()
	{
		for(BrandingProvider p : providers.values())
		{
			String t = p.getLoginTitle();
			if (t != null)
				return t;
		}
		return null;
	}
	
	/**
	 * Get custom login form. It is guaranteed that returned form implements LoginForm interface.
	 * 
	 * @param parentShell parent shell for login form
	 * @param properties system properties
	 * @return custom login form or default login form if no branding provider defines one.
	 */
	public Window getLoginForm(Shell parentShell, Properties properties)
	{
		for(BrandingProvider p : providers.values())
		{
			Window form = p.getLoginForm(parentShell, properties);
			if ((form != null) && (form instanceof LoginForm))
				return form;
		}
		return new DefaultLoginForm(parentShell, properties);
	}
	
   /**
    * Get custom login form for mobile UI. It is guaranteed that returned form implements LoginForm interface.
    * 
    * @param parentShell parent shell for login form
    * @param properties system properties
    * @return custom login form or default login form if no branding provider defines one.
    */
   public Window getMobileLoginForm(Shell parentShell, Properties properties)
   {
      for(BrandingProvider p : providers.values())
      {
         Window form = p.getMobileLoginForm(parentShell, properties);
         if ((form != null) && (form instanceof LoginForm))
            return form;
      }
      return new DefaultMobileLoginForm(parentShell, properties);
   }
   
	/**
	 * Get custom "About" dialog.
	 * 
	 * @param parentShell parent shell for dialog
	 * @return custom "About" dialog or null if no branding provider defines one.
	 */
	public Dialog getAboutDialog(Shell parentShell)
	{
		for(BrandingProvider p : providers.values())
		{
			Dialog d = p.getAboutDialog(parentShell);
			if (d != null)
				return d;
		}
		return null;
	}
	
	/**
	 * Get redirection URL for web console.
	 * 
	 * @return redirection URL for web console
	 */
	public String getRedirectionURL()
	{
		for(BrandingProvider p : providers.values())
		{
			String t = p.getRedirectionURL();
			if (t != null)
				return t;
		}
		return "nxmc"; //$NON-NLS-1$
	}
}
