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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.constants.UserAccessRights;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardModifyListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.IntermediateSelectionProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Dashboard view
 */
public class DashboardView extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardView"; //$NON-NLS-1$
	
	private NXCSession session;
	private Dashboard dashboard;
	private boolean readOnly = true;
	private IntermediateSelectionProvider selectionProvider;
	private DashboardControl dbc;
	private Composite parentComposite;
	private DashboardModifyListener dbcModifyListener;
	private RefreshAction actionRefresh;
	private Action actionEditMode;
	private Action actionSave;
	private Action actionAddAlarmBrowser;
	private Action actionAddLabel;
	private Action actionAddBarChart;
	private Action actionAddPieChart;
	private Action actionAddTubeChart;
	private Action actionAddLineChart;
	private Action actionAddAvailabilityChart;
	private Action actionAddDashboard;
	private Action actionAddStatusIndicator;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
		dashboard = (Dashboard)session.findObjectById(Long.parseLong(site.getSecondaryId()));
		if (dashboard == null)
			throw new PartInitException(Messages.get().DashboardView_InitError);
		setPartName(Messages.get().DashboardView_PartNamePrefix + dashboard.getObjectName());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
	   selectionProvider = new IntermediateSelectionProvider();
	   getSite().setSelectionProvider(selectionProvider);
	   
	   ConsoleJob job = new ConsoleJob(Messages.get().DashboardView_GetEffectiveRights, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            readOnly = ((dashboard.getEffectiveRights() & UserAccessRights.OBJECT_ACCESS_MODIFY) == 0);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().DashboardView_GetEffectiveRightsError;
         }
      };
      job.start();
      
      // FIXME: rewrite waiting
      try
      {
         job.join();
      }
      catch(InterruptedException e)
      {
      }
	   
		parentComposite = parent;
		dbc = new DashboardControl(parent, SWT.NONE, dashboard, this, selectionProvider, false);
		if (!readOnly)
		{
   		dbcModifyListener = new DashboardModifyListener() {
   			@Override
   			public void save()
   			{
   				actionSave.setEnabled(false);
   				firePropertyChange(PROP_DIRTY);
   			}
   			
   			@Override
   			public void modify()
   			{
   				actionSave.setEnabled(true);
   				firePropertyChange(PROP_DIRTY);
   			}
   		};
   		dbc.setModifyListener(dbcModifyListener);
		}
		createActions();
		contributeToActionBars();
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
				if (dbc.isModified())
				{
					if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().DashboardView_Refresh, 
							Messages.get().DashboardView_Confirmation))
						return;
				}
				rebuildDashboard(true);
			}
		};
		
		actionSave = new Action(Messages.get().DashboardView_Save) {
			@Override
			public void run()
			{
				dbc.saveDashboard(DashboardView.this);
			}
		};
		actionSave.setImageDescriptor(SharedIcons.SAVE);
		actionSave.setEnabled(false);
		
		actionEditMode = new Action(Messages.get().DashboardView_EditMode, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				dbc.setEditMode(!dbc.isEditMode());
				actionEditMode.setChecked(dbc.isEditMode());
				if (!dbc.isEditMode())
					rebuildDashboard(false);
			}
		};
		actionEditMode.setImageDescriptor(SharedIcons.EDIT);
		actionEditMode.setChecked(dbc.isEditMode());
		
		actionAddAlarmBrowser = new Action(Messages.get().DashboardView_AddAlarmBrowser) {
			@Override
			public void run()
			{
				dbc.addAlarmBrowser();
			}
		};
		
		actionAddLabel = new Action(Messages.get().DashboardView_AddLabel) {
			@Override
			public void run()
			{
				dbc.addLabel();
			}
		};
		
		actionAddBarChart = new Action(Messages.get().DashboardView_AddBarChart) {
			@Override
			public void run()
			{
				dbc.addBarChart();
			}
		};

		actionAddPieChart = new Action(Messages.get().DashboardView_AddPieChart) {
			@Override
			public void run()
			{
				dbc.addPieChart();
			}
		};

		actionAddTubeChart = new Action(Messages.get().DashboardView_AddTubeChart) {
			@Override
			public void run()
			{
				dbc.addTubeChart();
			}
		};

		actionAddLineChart = new Action(Messages.get().DashboardView_AddLineChart) {
			@Override
			public void run()
			{
				dbc.addLineChart();
			}
		};

		actionAddAvailabilityChart = new Action(Messages.get().DashboardView_AddAvailChart) {
			@Override
			public void run()
			{
				dbc.addAvailabilityChart();
			}
		};

		actionAddDashboard = new Action(Messages.get().DashboardView_AddDashboard) {
			@Override
			public void run()
			{
				dbc.addEmbeddedDashboard();
			}
		};

		actionAddStatusIndicator = new Action(Messages.get().DashboardView_AddStatusIndicator) {
			@Override
			public void run()
			{
				dbc.addStatusIndicator();
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
	   if (!readOnly)
	   {
   		manager.add(actionEditMode);
   		manager.add(actionSave);
   		manager.add(new Separator());
   		manager.add(actionAddAlarmBrowser);
   		manager.add(actionAddLabel);
   		manager.add(actionAddLineChart);
   		manager.add(actionAddBarChart);
   		manager.add(actionAddPieChart);
   		manager.add(actionAddTubeChart);
   		manager.add(actionAddStatusIndicator);
   		manager.add(actionAddAvailabilityChart);
   		manager.add(actionAddDashboard);
   		manager.add(new Separator());
	   }
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
	   if (!readOnly)
	   {
   		manager.add(actionEditMode);
   		manager.add(actionSave);
	   }
		manager.add(actionRefresh);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		dbc.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		dbc.saveDashboard(this);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSaveAs()
	 */
	@Override
	public void doSaveAs()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isDirty()
	 */
	@Override
	public boolean isDirty()
	{
		return dbc.isModified() && !readOnly;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
	 */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
	 */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return dbc.isModified() && !readOnly;
	}

	/**
	 * Rebuild current dashboard
	 * 
	 * @param reload if true, dashboard object will be reloaded and all unsaved changes
	 * will be lost
	 */
	private void rebuildDashboard(boolean reload)
	{
		if (dashboard == null)
			return;
		
		if (dbc != null)
			dbc.dispose();
		
		if (reload)
		{
			dashboard = (Dashboard)((NXCSession)ConsoleSharedData.getSession()).findObjectById(dashboard.getObjectId(), Dashboard.class);

			if (dashboard != null)
			{
				dbc = new DashboardControl(parentComposite, SWT.NONE, dashboard, this, selectionProvider, false);
				parentComposite.layout(true, true);
				setPartName(Messages.get().DashboardView_PartNamePrefix + dashboard.getObjectName());
				if (!readOnly)
				{
				   dbc.setModifyListener(dbcModifyListener);
				}
			}
			else
			{
				dbc = null;
			}
		}
		else
		{
			dbc = new DashboardControl(parentComposite, SWT.NONE, dashboard, dbc.getElements(), this, selectionProvider, dbc.isModified());
			parentComposite.layout(true, true);
			if (!readOnly)
			{
			   dbc.setModifyListener(dbcModifyListener);
			}
		}

		actionSave.setEnabled((dbc != null) ? dbc.isModified() : false);
		firePropertyChange(PROP_DIRTY);
	}
}
