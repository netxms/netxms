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
package org.netxms.ui.eclipse.console.api;

import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.netxms.client.NXCSession;

/**
 * Tweaklet for NetXMS console
 */
public interface ConsoleTweaklet
{
	/**
	 * Called from login thread after successful login. When this method called, UI is blocked in progress dialog.
	 * 
	 * @param session current session
	 */
	public abstract void postLogin(NXCSession session);
	
	/**
	 * Called from main application's workbench window advisor preWindowOpen
	 * @param configurer
	 */
	public abstract void preWindowOpen(IWorkbenchWindowConfigurer configurer);
	
	/**
	 * Called from main application's workbench window advisor postWindowCreate
	 * @param configurer
	 */
	public abstract void postWindowCreate(IWorkbenchWindowConfigurer configurer);
}
