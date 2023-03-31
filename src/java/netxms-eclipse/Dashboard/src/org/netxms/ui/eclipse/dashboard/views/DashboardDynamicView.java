/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.views;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.ISelectionService;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.IntermediateSelectionProvider;

/**
 * Dynamic dashboard view - change dashboard when selection in dashboard
 * navigator changes
 */
public class DashboardDynamicView extends ViewPart implements DashboardControlOwner
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardDynamicView"; //$NON-NLS-1$

   private IntermediateSelectionProvider selectionProvider;
	private Dashboard dashboard = null;
   private ScrolledComposite scroller = null;
	private DashboardControl dbc = null;
	private ISelectionService selectionService;
	private ISelectionListener selectionListener;
	private Composite parentComposite;
	private RefreshAction actionRefresh;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
      selectionProvider = new IntermediateSelectionProvider();
      getSite().setSelectionProvider(selectionProvider);
      
		parentComposite = parent;
		if (dashboard != null)
         rebuildDashboard();

		createActions();
		contributeToActionBars();

		selectionService = getSite().getWorkbenchWindow().getSelectionService();
		selectionListener = new ISelectionListener() {
			@Override
			public void selectionChanged(IWorkbenchPart part, ISelection selection)
			{
				if ((part instanceof DashboardNavigator) && (selection instanceof IStructuredSelection) && !selection.isEmpty())
				{
					Object object = ((IStructuredSelection)selection).getFirstElement();
					if (object instanceof Dashboard)
					{
						setObject((Dashboard)object);
					}
				}
			}
		};
		selectionService.addSelectionListener(selectionListener);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				rebuildDashboard();
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
		manager.add(actionRefresh);
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
      if ((dbc != null) && !dbc.isDisposed())
		{
			dbc.setFocus();
		}
	}

	/**
	 * @param object
	 */
	private void setObject(Dashboard object)
	{
		dashboard = object;
      rebuildDashboard();
	}
	
	/**
	 * Rebuild current dashboard
	 */
	private void rebuildDashboard()
	{
		if (dashboard == null)
			return;

		if (dbc != null)
			dbc.dispose();

      dashboard = ConsoleSharedData.getSession().findObjectById(dashboard.getObjectId(), Dashboard.class);
      if (dashboard == null)
      {
         dbc = null;
         return;
      }
      setPartName(Messages.get().DashboardDynamicView_PartNamePrefix + dashboard.getObjectName());

      if ((scroller != null) && !dashboard.isScrollable())
      {
         scroller.dispose();
         scroller = null;
      }
      else if ((scroller == null) && dashboard.isScrollable())
      {
         scroller = new ScrolledComposite(parentComposite, SWT.V_SCROLL);
         scroller.setExpandHorizontal(true);
         scroller.setExpandVertical(true);
         scroller.getVerticalBar().setIncrement(20);
         parentComposite.layout(true, true);
      }

      dbc = new DashboardControl(dashboard.isScrollable() ? scroller : parentComposite, SWT.NONE, dashboard, null, this, selectionProvider, false);
      if (dashboard.isScrollable())
      {
         scroller.setContent(dbc);
         scroller.addControlListener(new ControlAdapter() {
            public void controlResized(ControlEvent e)
            {
               updateScroller();
            }
         });
         updateScroller();
      }
      else
      {
         parentComposite.layout();
      }
	}

   /**
    * Update scroll area
    */
   private void updateScroller()
   {
      dbc.layout(true, true);
      Rectangle r = scroller.getClientArea();
      Point s = dbc.computeSize(r.width, SWT.DEFAULT);
      scroller.setMinSize(s);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
		if ((selectionService != null) && (selectionListener != null))
			selectionService.removeSelectionListener(selectionListener);
		super.dispose();
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.views.DashboardControlOwner#requestDashboardLayout()
    */
   @Override
   public void requestDashboardLayout()
   {
      if (scroller != null)
         updateScroller();
      else
         parentComposite.layout(true, true);
   }
}
