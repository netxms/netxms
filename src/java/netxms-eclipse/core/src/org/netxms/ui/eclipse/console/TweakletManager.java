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
package org.netxms.ui.eclipse.console;

import java.util.HashSet;
import java.util.Set;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleTweaklet;

/**
 * Class for managing tweaklets
 */
public class TweakletManager
{
	private static Set<ConsoleTweaklet> tweaklets = new HashSet<ConsoleTweaklet>();
	
	/**
	 * Initialize tweaklets
	 */
	static void initTweaklets()
	{
		// Read all registered extensions and create tweaklets
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.tweaklets"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ConsoleTweaklet t = (ConsoleTweaklet)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				tweaklets.add(t);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * Call postLogin() method for all tweaklets. This method called from login thread,
	 * why UI is blocked in wait dialog.
	 * 
	 * @param monitor
	 */
	static void postLogin(NXCSession session)
	{
		for(ConsoleTweaklet t : tweaklets)
			t.postLogin(session);
	}
	
	/**
	 * @param configurer
	 */
	static void preWindowOpen(IWorkbenchWindowConfigurer configurer)
	{
		for(ConsoleTweaklet t : tweaklets)
			t.preWindowOpen(configurer);
	}

	/**
	 * @param configurer
	 */
	static void postWindowCreate(IWorkbenchWindowConfigurer configurer)
	{
		for(ConsoleTweaklet t : tweaklets)
			t.postWindowCreate(configurer);
	}
}
