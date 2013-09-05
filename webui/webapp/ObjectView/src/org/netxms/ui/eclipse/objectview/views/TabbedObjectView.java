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
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.ISelectionService;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.activities.WorkbenchActivityHelper;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.tools.Command;
import org.netxms.ui.eclipse.console.tools.CommandBridge;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.objectview.services.SourceProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.IntermediateSelectionProvider;

/**
 * Tabbed view of currently selected object
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
	private IntermediateSelectionProvider selectionProvider;
	private SessionListener sessionListener = null;
	private Action actionRefresh;
	private SourceProvider sourceProvider = null;
	private long objectId = 0; 
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		sourceProvider = SourceProvider.getInstance();
		selectionService = getSite().getWorkbenchWindow().getSelectionService();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
		{
			Integer i = memento.getInteger("TabbedObjectView.objectId");
			if (i != null)
				objectId = i; 
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
	 */
	@Override
	public void saveState(IMemento memento)
	{
		super.saveState(memento);
		memento.putInteger("TabbedObjectView.objectId", (int)objectId);
	}

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
		header.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_HEADER_BACKGROUND, parent.getDisplay()));
		header.setForeground(SharedColors.getColor(SharedColors.OBJECT_TAB_HEADER, parent.getDisplay()));
		
		tabFolder = new CTabFolder(parent, SWT.TOP | SWT.FLAT | SWT.MULTI);
		tabFolder.setUnselectedImageVisible(true);
		tabFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		tabFolder.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				ObjectTab tab = null;
				if (e.item != null)
				{
					tab = (ObjectTab)((CTabItem)e.item).getData(); 
					tab.selected();
					selectionProvider.setSelectionProviderDelegate(tab.getSelectionProvider());
				}
				if (sourceProvider != null)
					sourceProvider.updateProperty(SourceProvider.ACTIVE_TAB, tab);
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		tabs = new ArrayList<ObjectTab>();	
		addTabs();

		selectionListener = new ISelectionListener() {
			@Override
			public void selectionChanged(IWorkbenchPart part, ISelection selection)
			{
				if ((part.getSite().getId().equals("org.netxms.ui.eclipse.view.navigation.objectbrowser")) && 
				    (selection instanceof IStructuredSelection))
				{
					if (selection.isEmpty())
					{
						setObject(null);
					}
					else
					{
						Object object = ((IStructuredSelection)selection).getFirstElement();
						if (object instanceof AbstractObject)
						{
							setObject((AbstractObject)object);
						}
					}
				}
			}
		};
		selectionService.addPostSelectionListener(selectionListener);
		
		createActions();
		contributeToActionBars();
		
		selectionProvider = new IntermediateSelectionProvider();
		getSite().setSelectionProvider(selectionProvider);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		if (objectId != 0)
			setObject(session.findObjectById(objectId));
		
		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) && (objectId == n.getSubCode()))
				{
					final AbstractObject object = (AbstractObject)n.getObject();
					getSite().getShell().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							onObjectUpdate(object);
						}
					});
				}
			}
		};
		session.addListener(sessionListener);
		
		CommandBridge.getInstance().registerCommand("TabbedObjectView/selectTab", new Command() {
			@Override
			public Object execute(String name, Object arg)
			{
				if (arg instanceof String)
					selectTab((String)arg);
				return null;
			}
		});
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
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
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
	 * Notify visible tabs about object update
	 * 
	 * @param object New object
	 */
	private void onObjectUpdate(AbstractObject object)
	{
		header.setText(object.getObjectName());
		for(final ObjectTab tab : tabs)
		{
			if (tab.isVisible())
				tab.currentObjectUpdated(object);
		}
	}

	/**
	 * Set new active object
	 * 
	 * @param object New object
	 */
	public void setObject(final AbstractObject object)
	{
		// Current focus event may not be finished yet, so we have
		// to wait for any outstanding events and only then start
		// changing tabs. Otherwise runtime exception will be thrown.
		final Display display = getSite().getShell().getDisplay();
		display.asyncExec(new Runnable() {
			@Override
			public void run() 
			{
				while(display.readAndDispatch()); // wait for events to finish before continue
				setObjectInternal(object);
			}
		});
	}
	
	/**
	 * Set new active object - internal implementation
	 * 
	 * @param object New object
	 */
	private void setObjectInternal(AbstractObject object)
	{
		if (object != null)
		{
			header.setText(object.getObjectName());
			for(final ObjectTab tab : tabs)
			{
				if (tab.showForObject(object) && !WorkbenchActivityHelper.filterItem(tab))
				{
					tab.show();
					tab.changeObject(object);
				}
				else
				{
					tab.hide();
					tab.changeObject(null);
				}
			}
			
			if ((objectId == 0) || (tabFolder.getSelection() == null))
			{
				try
				{
					tabFolder.setSelection(0);
					ObjectTab tab = (ObjectTab)tabFolder.getItem(0).getData();
					tab.selected();
					selectionProvider.setSelectionProviderDelegate(tab.getSelectionProvider());
				}
				catch(IllegalArgumentException e)
				{
				}
			}
			objectId = (object != null) ? object.getObjectId() : 0;
		}
		else
		{
			for(final ObjectTab tab : tabs)
			{
				tab.hide();
				tab.changeObject(null);
			}
			objectId = 0;
			header.setText("");
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
				Activator.log("Exception when instantiating object tab", e);
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
	
	/**
	 * Select tab with given ID
	 * 
	 * @param tabId
	 */
	public void selectTab(String tabId)
	{
		for(int i = 0; i < tabFolder.getItemCount(); i++)
		{
			CTabItem item = tabFolder.getItem(i);
			ObjectTab tab = (ObjectTab)item.getData();
			if (tab.getLocalId().equals(tabId))
			{
				tabFolder.setSelection(i);
				tab.selected();
				selectionProvider.setSelectionProviderDelegate(tab.getSelectionProvider());
				break;
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		CommandBridge.getInstance().unregisterCommand("TabbedObjectView/selectTab");
		ConsoleSharedData.getSession().removeListener(sessionListener);
		if (sourceProvider != null)
			sourceProvider.updateProperty(SourceProvider.ACTIVE_TAB, null);
		getSite().setSelectionProvider(null);
		if ((selectionService != null) && (selectionListener != null))
			selectionService.removePostSelectionListener(selectionListener);
		for(final ObjectTab tab : tabs)
		{
			tab.dispose();
		}
		headerFont.dispose();
		super.dispose();
	}
}
