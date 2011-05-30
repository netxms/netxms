/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.events.ShellAdapter;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.IWorkbenchConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.model.ContributionComparator;
import org.eclipse.ui.model.IContributionService;

/**
 * Workbench advisor for NetXMS console application
 */
public class NXMCWorkbenchAdvisor extends WorkbenchAdvisor
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#createWorkbenchWindowAdvisor(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
	 */
	public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		return new NXMCWorkbenchWindowAdvisor(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#getInitialWindowPerspectiveId()
	 */
	public String getInitialWindowPerspectiveId()
	{
		return Activator.getDefault().getPreferenceStore().getString("INITIAL_PERSPECTIVE");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#initialize(org.eclipse.ui.application.IWorkbenchConfigurer)
	 */
	@Override
	public void initialize(IWorkbenchConfigurer configurer)
	{
		super.initialize(configurer);
		
		IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		configurer.setSaveAndRestore(ps.getBoolean("SAVE_AND_RESTORE"));
		
		if (ps.getBoolean("HTTP_PROXY_ENABLED"))
		{
			System.setProperty("http.proxyHost", ps.getString("HTTP_PROXY_SERVER"));
			System.setProperty("http.proxyPort", ps.getString("HTTP_PROXY_PORT"));
			System.setProperty("http.noProxyHosts", ps.getString("HTTP_PROXY_EXCLUSIONS"));
		}
		else
		{
			System.clearProperty("http.proxyHost");
			System.clearProperty("http.proxyPort");
			System.clearProperty("http.noProxyHosts");
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#getComparatorFor(java.lang.String)
	 */
	@Override
	public ContributionComparator getComparatorFor(String contributionType)
	{
		if (contributionType.equals(IContributionService.TYPE_PROPERTY))
			return new ExtendedContributionComparator();
		return super.getComparatorFor(contributionType);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#postStartup()
	 */
	@Override
	public void postStartup()
	{
		super.postStartup();
		final Shell shell = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell();
		shell.addShellListener(new ShellAdapter()	{
			public void shellIconified(ShellEvent e)
			{
				if (Activator.getDefault().getPreferenceStore().getBoolean("HIDE_WHEN_MINIMIZED"))
					shell.setVisible(false);
			}
		});
	}
}
