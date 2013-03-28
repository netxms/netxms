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
package org.netxms.ui.eclipse.datacollection.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Last values view
 *
 */
public class LastValues extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.datacollection.view.last_values"; //$NON-NLS-1$
	
	private NXCSession session;
	private AbstractObject dcTarget;
	private LastValuesWidget dataView;
	private Action actionRefresh;
	private Action actionAutoUpdate;
	private Action actionUseMultipliers;
	private Action actionShowFilter;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		AbstractObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
		dcTarget = ((obj != null) && ((obj instanceof Node) || (obj instanceof MobileDevice))) ? obj : null;
		setPartName(Messages.LastValues_PartNamePrefix + ((dcTarget != null) ? dcTarget.getObjectName() : Messages.LastValues_Error));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);
		
		dataView = new LastValuesWidget(this, parent, SWT.NONE, dcTarget, "LastValuesWidget"); //$NON-NLS-1$
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		dataView.setLayoutData(fd);
		
		createActions();
		contributeToActionBars();
		
		dataView.setFilterCloseAction(actionShowFilter);
		
		activateContext();
	}

	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.datacollection.context.LastValues"); //$NON-NLS-1$
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		dataView.setFocus();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				dataView.refresh();
			}
		};
		
		actionAutoUpdate = new Action(Messages.LastValues_AutoRefresh, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				dataView.setAutoRefreshEnabled(!dataView.isAutoRefreshEnabled());
			}
		};
		actionAutoUpdate.setChecked(dataView.isAutoRefreshEnabled());
		
		actionUseMultipliers = new Action(Messages.LastValues_UseMultipliers, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				dataView.setUseMultipliers(!dataView.areMultipliersUsed());
			}
		};
		actionUseMultipliers.setChecked(dataView.areMultipliersUsed());

		actionShowFilter = new Action(Messages.LastValues_ShowFilter, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				dataView.enableFilter(!dataView.isFilterEnabled());
				actionShowFilter.setChecked(dataView.isFilterEnabled());
			}
      };
      actionShowFilter.setChecked(dataView.isFilterEnabled());
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
		final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
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
		manager.add(actionShowFilter);
		manager.add(actionAutoUpdate);
		manager.add(actionUseMultipliers);
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
		manager.add(actionRefresh);
	}
}
