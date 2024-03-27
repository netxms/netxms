/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.views;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.SharedIcons;

/**
 * Web browser view
 */
public class BrowserView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.views.BrowserView"; //$NON-NLS-1$

   private String title = null;
   private String initialUrl = null;
	private Browser browser;
	private Action actionBack;
	private Action actionForward;
	private Action actionStop;
	private Action actionReload;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
   @Override
   public void init(IViewSite site, IMemento memento) throws PartInitException
   {
      initialUrl = (memento != null) ? memento.getString("URL") : null;
      if ((initialUrl != null) && initialUrl.isEmpty())
         initialUrl = null;

      title = (memento != null) ? memento.getString("Title") : null;
      if ((title != null) && title.isEmpty())
         title = null;

      super.init(site, memento);
   }

   /**
    * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
    */
   @Override
   public void saveState(IMemento memento)
   {
      super.saveState(memento);
      memento.putString("URL", browser.getUrl());
      memento.putString("Title", (title != null) ? title : "");
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
      browser = new Browser(parent, SWT.EDGE);
		browser.addLocationListener(new LocationListener() {
			@Override
			public void changing(LocationEvent event)
			{
            if (title == null)
               setPartName(String.format(Messages.get().BrowserView_PartName_Changing, event.location));
				actionStop.setEnabled(true);
			}
			
			@Override
			public void changed(LocationEvent event)
			{
            if (title == null)
               setPartName(String.format(Messages.get().BrowserView_PartName_Changed, getTitle(browser.getText(), event.location)));
				actionStop.setEnabled(false);
			}
		});
		
		createActions();
		contributeToActionBars();

      if (title != null)
         setPartName(title);
      if (initialUrl != null)
         openUrl(initialUrl);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionBack = new Action(Messages.get().BrowserView_Back, SharedIcons.NAV_BACKWARD) {
			@Override
			public void run()
			{
				browser.back();
			}
		};

		actionForward = new Action(Messages.get().BrowserView_Forward, SharedIcons.NAV_FORWARD) {
			@Override
			public void run()
			{
				browser.forward();
			}
		};

		actionStop = new Action(Messages.get().BrowserView_Stop, Activator.getImageDescriptor("icons/stop.png")) { //$NON-NLS-1$
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

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
		if (!browser.isDisposed())
			browser.setFocus();
	}

   /**
    * Set fixed title for browser part
    * 
    * @param title title to be shown
    */
   public void setFixedTitle(String title)
   {
      this.title = title;
      setPartName(title);
   }

	/**
	 * @param url
	 */
	public void openUrl(final String url)
	{
		browser.setUrl(url);
	}
	
	/**
	 * @param html
	 * @param fallback
	 * @return
	 */
	private static String getTitle(String html, String fallback)
	{
	   Pattern p = Pattern.compile("\\<html\\>.*\\<head\\>.*\\<title\\>(.*)\\</title\\>.*", Pattern.CASE_INSENSITIVE | Pattern.DOTALL); //$NON-NLS-1$
	   Matcher m = p.matcher(html);
	   if (m.matches())
	   {
	      int index = m.start(1);
	      if (index == -1)
	         return fallback;
	      return html.substring(index, m.end(1));
	   }
	   return fallback;
	}
}
