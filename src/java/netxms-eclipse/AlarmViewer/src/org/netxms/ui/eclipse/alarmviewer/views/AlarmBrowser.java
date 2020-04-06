/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IPartListener;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.tools.VisibilityValidator;

/**
 * Alarm browser view
 */
public class AlarmBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.alarmviewer.view.alarm_browser"; //$NON-NLS-1$
	
	private AlarmList alarmView;
	private Action actionRefresh;
	private Action actionExportToCsv;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());

      alarmView = new AlarmList(this, parent, SWT.NONE, "AlarmBrowser", new VisibilityValidator() { //$NON-NLS-1$
         @Override
         public boolean isVisible()
         {
            return getSite().getPage().isPartVisible(AlarmBrowser.this);
         }
      });

		createActions();
		contributeToActionBars();
		activateContext();
		
		getSite().setSelectionProvider(alarmView.getSelectionProvider());

      getSite().getPage().addPartListener(new IPartListener() {
         @Override
         public void partOpened(IWorkbenchPart part)
         {
         }

         @Override
         public void partDeactivated(IWorkbenchPart part)
         {
         }

         @Override
         public void partClosed(IWorkbenchPart part)
         {
         }

         @Override
         public void partBroughtToTop(IWorkbenchPart part)
         {
            if (part == AlarmBrowser.this)
               alarmView.doPendingUpdates();
         }

         @Override
         public void partActivated(IWorkbenchPart part)
         {
            if (part == AlarmBrowser.this)
               alarmView.doPendingUpdates();
         }
      });
	}

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.alarmviewer.context.AlarmBrowser"); //$NON-NLS-1$
      }
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				alarmView.refresh();
			}
		};
		
		actionExportToCsv = new ExportToCsvAction(this, alarmView.getViewer(), false);
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
      manager.add(alarmView.getActionShowColors());
      manager.add(alarmView.getActionShowFilter());
      manager.add(new Separator());
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportToCsv);
      manager.add(alarmView.getActionShowFilter());
		manager.add(actionRefresh);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		alarmView.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		alarmView.dispose();
		super.dispose();
	}
}
