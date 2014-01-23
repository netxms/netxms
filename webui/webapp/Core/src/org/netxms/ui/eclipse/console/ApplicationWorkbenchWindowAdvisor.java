/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import org.eclipse.core.commands.common.NotDefinedException;
import org.eclipse.jface.bindings.BindingManager;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CBanner;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.internal.keys.BindingService;
import org.eclipse.ui.keys.IBindingService;

/**
 * Workbench window advisor
 */
@SuppressWarnings("restriction")
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{
	/**
	 * @param configurer
	 */
	public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		super(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#createActionBarAdvisor(org.eclipse.ui.application.IActionBarConfigurer)
	 */
	public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
	{
		return new ApplicationActionBarAdvisor(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#preWindowOpen()
	 */
	@Override
	public void preWindowOpen()
	{
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		configurer.setShowCoolBar(true);
		configurer.setShowPerspectiveBar(true);
		configurer.setShowStatusLine(false);
		configurer.setTitle(Messages.get().ApplicationWorkbenchWindowAdvisor_AppTitle);
		configurer.setShellStyle(SWT.NO_TRIM);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
	 */
	@Override
	public void postWindowCreate()
	{
		super.postWindowCreate();
		
		BindingService service = (BindingService)getWindowConfigurer().getWindow().getWorkbench().getService(IBindingService.class);
		BindingManager bindingManager = service.getBindingManager();
		try
		{
			bindingManager.setActiveScheme(service.getScheme("org.netxms.ui.eclipse.defaultKeyBinding")); //$NON-NLS-1$
		}
		catch(NotDefinedException e)
		{
			e.printStackTrace();
		}
		
		final Shell shell = getWindowConfigurer().getWindow().getShell(); 
		shell.setMaximized(true);
		
		for(Control ctrl : shell.getChildren())
		{
			ctrl.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			if (ctrl instanceof CBanner)
			{
				for(Control cc : ((CBanner)ctrl).getChildren())
					cc.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			}
			else if (ctrl.getClass().getName().equals("org.eclipse.swt.widgets.Composite")) //$NON-NLS-1$
			{
				for(Control cc : ((Composite)ctrl).getChildren())
					cc.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			}
		}
		
		shell.getMenuBar().setData(RWT.CUSTOM_VARIANT, "menuBar"); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowClose()
	 */
	@Override
	public void postWindowClose()
	{
		super.postWindowClose();
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor != null)
         executor.execute("location.reload(true);");
	}
}
