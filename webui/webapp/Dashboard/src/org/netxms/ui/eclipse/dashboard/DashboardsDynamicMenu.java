/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.ISources;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.menus.IWorkbenchContribution;
import org.eclipse.ui.services.IEvaluationService;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.dashboard.views.DashboardView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Dynamic object tools menu creator
 */
public class DashboardsDynamicMenu extends ContributionItem implements IWorkbenchContribution
{
	private IEvaluationService evalService;
	
	/**
	 * Creates a contribution item with a null id.
	 */
	public DashboardsDynamicMenu()
	{
		super();
	}

	/**
	 * Creates a contribution item with the given (optional) id.
	 * 
	 * @param id the contribution item identifier, or null
	 */
	public DashboardsDynamicMenu(String id)
	{
		super(id);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.menus.IWorkbenchContribution#initialize(org.eclipse.ui.services.IServiceLocator)
	 */
	@Override
	public void initialize(IServiceLocator serviceLocator)
	{
		evalService = (IEvaluationService)serviceLocator.getService(IEvaluationService.class);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.Menu, int)
	 */
	@Override
	public void fill(Menu menu, int index)
	{
		final Object selection = evalService.getCurrentState().getVariable(ISources.ACTIVE_MENU_SELECTION_NAME);
		if ((selection == null) || !(selection instanceof IStructuredSelection))
			return;
		
		final AbstractObject object = (AbstractObject)((IStructuredSelection)selection).getFirstElement();
		if (!(object instanceof Container) && 
		    !(object instanceof Cluster) && 
          !(object instanceof Node) && 
          !(object instanceof MobileDevice) && 
		    !(object instanceof ServiceRoot) && 
		    !(object instanceof Subnet) &&
		    !(object instanceof Zone) &&
          !(object instanceof Condition) &&
		    !(object instanceof EntireNetwork))
			return;
		
		List<AbstractObject> dashboards = object.getDashboards(true);
		if (dashboards.isEmpty())
		   return;

		Collections.sort(dashboards, new Comparator<AbstractObject>() {
			@Override
			public int compare(AbstractObject o1, AbstractObject o2)
			{
				return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
			}
		});
		
      final Menu dashboardsMenu = new Menu(menu);
		for(AbstractObject d : dashboards)
		{
         final MenuItem item = new MenuItem(dashboardsMenu, SWT.PUSH);
         item.setText(d.getObjectName());
         item.setData(d.getObjectId());
         item.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
               try
               {
                  window.getActivePage().showView(DashboardView.ID, item.getData().toString(), IWorkbenchPage.VIEW_ACTIVATE);
               }
               catch(PartInitException ex)
               {
                  MessageDialogHelper.openError(window.getShell(), Messages.get().OpenDashboard_Error, Messages.get().OpenDashboard_ErrorText + ex.getMessage());
               }
            }
         });
		}
   	MenuItem dashboardsMenuItem = new MenuItem(menu, SWT.CASCADE, index);
   	dashboardsMenuItem.setText("&Dashboards");
   	dashboardsMenuItem.setMenu(dashboardsMenu);
	}
}
