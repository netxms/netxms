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
package org.netxms.ui.eclipse.datacollection;

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.datacollection.views.SummaryTable;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Dynamic object tools menu creator
 */
public class SummaryTablesDynamicMenu extends ContributionItem implements IWorkbenchContribution
{
	private IEvaluationService evalService;
	
	/**
	 * Creates a contribution item with a null id.
	 */
	public SummaryTablesDynamicMenu()
	{
		super();
	}

	/**
	 * Creates a contribution item with the given (optional) id.
	 * 
	 * @param id the contribution item identifier, or null
	 */
	public SummaryTablesDynamicMenu(String id)
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
		
		final AbstractObject baseObject = (AbstractObject)((IStructuredSelection)selection).getFirstElement();
		if (!(baseObject instanceof Container) && 
		    !(baseObject instanceof Cluster) && 
		    !(baseObject instanceof ServiceRoot) && 
		    !(baseObject instanceof Subnet) &&
		    !(baseObject instanceof Zone) &&
		    !(baseObject instanceof EntireNetwork))
			return;

		final Menu tablesMenu = new Menu(menu);
		
		DciSummaryTableDescriptor[] tables = SummaryTablesCache.getTables();
		Arrays.sort(tables, new Comparator<DciSummaryTableDescriptor>() {
			@Override
			public int compare(DciSummaryTableDescriptor arg0, DciSummaryTableDescriptor arg1)
			{
				return arg0.getMenuPath().replace("&", "").compareToIgnoreCase(arg1.getMenuPath().replace("&", "")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
			}
		});
		
		Map<String, Menu> menus = new HashMap<String, Menu>();
		int added = 0;
		for(int i = 0; i < tables.length; i++)
		{
			String[] path = tables[i].getMenuPath().split("\\-\\>"); //$NON-NLS-1$
		
			Menu rootMenu = tablesMenu;
			for(int j = 0; j < path.length - 1; j++)
			{
				String key = path[j].replace("&", ""); //$NON-NLS-1$ //$NON-NLS-2$
				Menu currMenu = menus.get(key);
				if (currMenu == null)
				{
					currMenu = new Menu(rootMenu);
					MenuItem item = new MenuItem(rootMenu, SWT.CASCADE);
					item.setText(path[j]);
					item.setMenu(currMenu);
					menus.put(key, currMenu);
				}
				rootMenu = currMenu;
			}
			
			final MenuItem item = new MenuItem(rootMenu, SWT.PUSH);
			item.setText(path[path.length - 1]);
			item.setData(tables[i]);
			item.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					queryTable(baseObject.getObjectId(), ((DciSummaryTableDescriptor)item.getData()).getId());
				}
			});
			
			added++;
		}

		if (added > 0)
		{
			MenuItem tablesMenuItem = new MenuItem(menu, SWT.CASCADE, index);
			tablesMenuItem.setText(Messages.SummaryTablesDynamicMenu_MenuName);
			tablesMenuItem.setMenu(tablesMenu);
		}
		else
		{
			tablesMenu.dispose();
		}
	}
		
	/**
	 * Query table using selected base object
	 * 
	 * @param baseObjectId
	 * @param tableId
	 */
	private void queryTable(final long baseObjectId, final int tableId)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.SummaryTablesDynamicMenu_QueryTableJob, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Table results = session.queryDciSummaryTable(tableId, baseObjectId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						String secondaryId = Integer.toString(tableId) + "&" + Long.toString(baseObjectId); //$NON-NLS-1$
						IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
						try
						{
							SummaryTable view =  (SummaryTable)window.getActivePage().showView(SummaryTable.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
							view.setTable(results);
						}
						catch(PartInitException e)
						{
							MessageDialogHelper.openError(window.getShell(), Messages.SummaryTablesDynamicMenu_Error, String.format(Messages.SummaryTablesDynamicMenu_CannotOpenView, e.getLocalizedMessage()));
						}
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.SummaryTablesDynamicMenu_CannotReadData;
			}
		}.start();
	}
}
