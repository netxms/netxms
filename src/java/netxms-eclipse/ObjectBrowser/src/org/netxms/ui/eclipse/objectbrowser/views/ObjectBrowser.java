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
package org.netxms.ui.eclipse.objectbrowser.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Object browser view
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.view.navigation.objectbrowser"; //$NON-NLS-1$
	
	private ObjectTree objectTree;
	private Action actionShowFilter;
	private Action actionHideUnmanaged;
	private Action actionHideTemplateChecks;
	private Action actionMoveObject;
	private Action actionRefresh;
	private Action actionProperties;
	private boolean initHideUnmanaged = false;
	private boolean initHideTemplateChecks = false;
	private boolean initShowFilter = true;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
		{
			initHideUnmanaged = safeCast(memento.getBoolean("ObjectBrowser.hideUnmanaged"), false);
			initHideTemplateChecks = safeCast(memento.getBoolean("ObjectBrowser.hideTemplateChecks"), false);
			initShowFilter = safeCast(memento.getBoolean("ObjectBrowser.showFilter"), true);
		}
	}

	private static boolean safeCast(Boolean b, boolean defval)
	{
		return (b != null) ? b : defval;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
	 */
	@Override
	public void saveState(IMemento memento)
	{
		super.saveState(memento);
		memento.putBoolean("ObjectBrowser.hideUnmanaged", objectTree.isHideUnmanaged());
		memento.putBoolean("ObjectBrowser.hideTemplateChecks", objectTree.isHideTemplateChecks());
		memento.putBoolean("ObjectBrowser.showFilter", objectTree.isFilterEnabled());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);
		
		// Read custom root objects
		long[] rootObjects = null;
		Object value = ConsoleSharedData.getProperty("ObjectBrowser.rootObjects"); //$NON-NLS-1$
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.MULTI, rootObjects, null);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.setLayoutData(fd);
		
		objectTree.setHideTemplateChecks(initHideTemplateChecks);
		objectTree.setHideUnmanaged(initHideUnmanaged);
		objectTree.enableFilter(initShowFilter);
		
		createActions();
		createMenu();
		createToolBar();
		createPopupMenu();
		
		objectTree.enableDragSupport();
		getSite().setSelectionProvider(objectTree.getTreeViewer());
		
		objectTree.getTreeViewer().addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				int size = ((IStructuredSelection)objectTree.getTreeViewer().getSelection()).size();
				actionMoveObject.setEnabled(size == 1);
				actionProperties.setEnabled(size == 1);
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
				objectTree.refresh();
			}
		};
		
		actionMoveObject = new Action("&Move to another container") {
			@Override
			public void run()
			{
				moveObject();
			}
		};
		
      actionHideUnmanaged = new Action("&Hide unmanaged objects", SWT.TOGGLE)
      {
			@Override
			public void run()
			{
				objectTree.setHideUnmanaged(!objectTree.isHideUnmanaged());
		      actionHideUnmanaged.setChecked(objectTree.isHideUnmanaged());
			}
      };
      actionHideUnmanaged.setChecked(objectTree.isHideUnmanaged());

      actionHideTemplateChecks = new Action("Hide check templates", SWT.TOGGLE)
      {
			@Override
			public void run()
			{
				objectTree.setHideTemplateChecks(!objectTree.isHideTemplateChecks());
		      actionHideTemplateChecks.setChecked(objectTree.isHideTemplateChecks());
			}
      };
      actionHideTemplateChecks.setChecked(objectTree.isHideTemplateChecks());

      actionShowFilter = new Action("Show &filter", SWT.TOGGLE) //$NON-NLS-1$
      {
			@Override
			public void run()
			{
				objectTree.enableFilter(!objectTree.isFilterEnabled());
				actionShowFilter.setChecked(objectTree.isFilterEnabled());
			}
      };
      actionShowFilter.setChecked(objectTree.isFilterEnabled());
      
      actionProperties = new PropertyDialogAction(getSite(), objectTree.getTreeViewer());
	}

	/**
	 * Create view menu
	 */
   private void createMenu()
   {
      IMenuManager manager = getViewSite().getActionBars().getMenuManager();
      manager.add(actionShowFilter);
      manager.add(actionHideUnmanaged);
      manager.add(actionHideTemplateChecks);
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

	/**
	 * Create view toolbar
	 */
   private void createToolBar()
   {
      IToolBarManager manager = getViewSite().getActionBars().getToolBarManager();
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(actionRefresh);
   }

	/**
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = manager.createContextMenu(objectTree.getTreeControl());
		objectTree.getTreeControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(manager, objectTree.getTreeViewer());
	}

	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_CREATION));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_BINDING));
		if (isValidSelectionForMove())
			manager.add(actionMoveObject);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_TOPOLOGY));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		manager.add(actionProperties);
	}
	
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
	public void setFocus() 
	{
		objectTree.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}
	
	/**
	 * Check if current selection is valid for moving object
	 * 
	 * @return true if current selection is valid for moving object
	 */
	private boolean isValidSelectionForMove()
	{
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		if (selection.length != 1)
			return false;
		
		if (selection[0].getParentItem() == null)
			return false;
		
		final Object currentObject = selection[0].getData();
		final Object parentObject = selection[0].getParentItem().getData();
		
		return (((currentObject instanceof Node) ||
	            (currentObject instanceof Cluster) ||
		         (currentObject instanceof Subnet) ||
	            (currentObject instanceof Condition) ||
		         (currentObject instanceof Container)) &&
		        ((parentObject instanceof Container) ||
		         (parentObject instanceof ServiceRoot)));
	}
	
	/**
	 * Move selected object to another container
	 */
	private void moveObject()
	{
		if (!isValidSelectionForMove())
			return;
		
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		final Object currentObject = selection[0].getData();
		final Object parentObject = selection[0].getParentItem().getData();
		
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createContainerSelectionFilter());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			final GenericObject target = dlg.getSelectedObjects().get(0);
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob("Moving object " + ((GenericObject)currentObject).getObjectName(), this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					long objectId = ((GenericObject)currentObject).getObjectId();
					session.bindObject(target.getObjectId(), objectId);
					session.unbindObject(((GenericObject)parentObject).getObjectId(), objectId);
				}
	
				@Override
				protected String getErrorMessage()
				{
					return "Cannot move object " + ((GenericObject)currentObject).getObjectName();
				}
			}.start();
		}
	}
}
