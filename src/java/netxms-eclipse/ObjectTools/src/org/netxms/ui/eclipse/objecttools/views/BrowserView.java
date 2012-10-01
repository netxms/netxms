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
package org.netxms.ui.eclipse.objecttools.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Web browser view
 */
public class BrowserView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.BrowserView";
	
	private Browser browser;
	private Action actionBack;
	private Action actionForward;
	private Action actionStop;
	private Action actionReload;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		browser = new Browser(parent, SWT.NONE);
		browser.addLocationListener(new LocationListener() {
			@Override
			public void changing(LocationEvent event)
			{
				setPartName("Web Browser - [" + event.location + "]");
				actionStop.setEnabled(true);
			}
			
			@Override
			public void changed(LocationEvent event)
			{
				setPartName("Web Browser - " + event.location);
				actionStop.setEnabled(false);
			}
		});
		
		createActions();
		contributeToActionBars();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionBack = new Action("&Back", SharedIcons.NAV_BACKWARD) {
			@Override
			public void run()
			{
				browser.back();
			}
		};

		actionForward = new Action("&Forward", SharedIcons.NAV_FORWARD) {
			@Override
			public void run()
			{
				browser.forward();
			}
		};

		actionStop = new Action("&Stop", Activator.getImageDescriptor("icons/stop.png")) {
			@Override
			public void run()
			{
				browser.stop();
			}
		};

		actionReload = new RefreshAction(this) {
			@Override
			public void run()
			{
				browser.refresh();
			}
		};
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionBack);
		manager.add(actionForward);
		manager.add(actionStop);
		manager.add(actionReload);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionBack);
		manager.add(actionForward);
		manager.add(actionStop);
		manager.add(actionReload);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		if (!browser.isDisposed())
			browser.setFocus();
	}

	/**
	 * @param url
	 */
	public void openUrl(final String url)
	{
		browser.setUrl(url);
	}
}
