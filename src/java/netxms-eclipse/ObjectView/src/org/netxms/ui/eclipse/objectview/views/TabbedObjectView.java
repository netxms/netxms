/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.views;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.ISelectionService;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;

/**
 * Tabbed view of currently selected object
 *
 */
public class TabbedObjectView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objectview.view.tabbed_object_view";
	
	private CLabel header;
	private CTabFolder tabFolder;
	private Font headerFont;
	private List<ObjectTab> tabs;
	private ISelectionService selectionService = null;
	private ISelectionListener selectionListener = null;
	private Action actionRefresh;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		parent.setLayout(layout);
		
		headerFont = new Font(parent.getDisplay(), "Verdana", 11, SWT.BOLD);
		
		header = new CLabel(parent, SWT.BORDER);
		header.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
		header.setFont(headerFont);
		header.setBackground(new Color(parent.getDisplay(), 153, 180, 209));
		header.setForeground(new Color(parent.getDisplay(), 255, 255, 255));
		
		tabFolder = new CTabFolder(parent, SWT.TOP | SWT.FLAT | SWT.MULTI);
		tabFolder.setUnselectedImageVisible(true);
		tabFolder.setSimple(true);
		tabFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		tabFolder.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (e.item != null)
				{
					((ObjectTab)((CTabItem)e.item).getData()).selected();
				}
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		tabs = new ArrayList<ObjectTab>();	
		addTabs();

		selectionService = getSite().getWorkbenchWindow().getSelectionService();
		selectionListener = new ISelectionListener() {
			@Override
			public void selectionChanged(IWorkbenchPart part, ISelection selection)
			{
				if ((selection instanceof IStructuredSelection) && !selection.isEmpty())
				{
					Object object = ((IStructuredSelection)selection).getFirstElement();
					if (object instanceof GenericObject)
					{
						setObject((GenericObject)object);
					}
				}
			}
		};
		selectionService.addSelectionListener(selectionListener);
		
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
				refreshCurrentTab();
			}
		};
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
	}
	
	/**
	 * Fill local tool bar
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}
	
	/**
	 * Refresh current tab
	 */
	private void refreshCurrentTab()
	{
		CTabItem item = tabFolder.getSelection();
		if (item != null)
			((ObjectTab)item.getData()).refresh();
	}

	/**
	 * Set new active object
	 * 
	 * @param object New object
	 */
	private void setObject(GenericObject object)
	{
		header.setText(object.getObjectName());
		for(final ObjectTab tab : tabs)
		{
			if (tab.showForObject(object))
			{
				tab.show();
				tab.changeObject(object);
			}
			else
			{
				tab.hide();
			}
		}
		
		if (tabFolder.getSelection() == null)
		{
			try
			{
				tabFolder.setSelection(tabFolder.getItem(0));
			}
			catch(IllegalArgumentException e)
			{
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		tabFolder.setFocus();
		CTabItem item = tabFolder.getSelection();
		if (item != null)
			((ObjectTab)item.getData()).selected();
	}
	
	/**
	 * Add all tabs
	 */
	private void addTabs()
	{
		// Read all registered extensions and create tabs
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectview.tabs");
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ObjectTab tab = (ObjectTab)elements[i].createExecutableExtension("class");
				tab.configure(elements[i], this);
				tabs.add(tab);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		// Sort tabs by appearance order
		Collections.sort(tabs, new Comparator<ObjectTab>() {
			@Override
			public int compare(ObjectTab arg0, ObjectTab arg1)
			{
				return arg0.getOrder() - arg1.getOrder();
			}
		});
		
		// Create widgets for all tabs
		for(final ObjectTab tab : tabs)
		{
			tab.create(tabFolder);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((selectionService != null) && (selectionListener != null))
			selectionService.removeSelectionListener(selectionListener);
		for(final ObjectTab tab : tabs)
		{
			tab.dispose();
		}
		headerFont.dispose();
		super.dispose();
	}
}
