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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITreeSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
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
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenListener;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.IActionConstants;

/**
 * Object browser view
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.view.navigation.objectbrowser"; //$NON-NLS-1$
	
	private enum SubtreeType { NETWORK, INFRASTRUCTURE, TEMPLATES, DASHBOARDS, MAPS, BUSINESS_SERVICES };
	
	private ObjectTree objectTree;
	private Action actionShowFilter;
	private Action actionShowStatusIndicator;
	private Action actionHideUnmanaged;
	private Action actionHideTemplateChecks;
	private Action actionMoveObject;
	private Action actionMoveTemplate;
	private Action actionMoveBusinessService;
	private Action actionRefresh;
	private Action actionProperties;
	private boolean initHideUnmanaged = false;
	private boolean initHideTemplateChecks = false;
	private boolean initShowFilter = true;
	private boolean initShowStatus = false;
	private String initialObjectSelection = null;
	private List<OpenHandlerData> openHandlers = new ArrayList<OpenHandlerData>(0);
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
		{
			initHideUnmanaged = safeCast(memento.getBoolean("ObjectBrowser.hideUnmanaged"), false); //$NON-NLS-1$
			initHideTemplateChecks = safeCast(memento.getBoolean("ObjectBrowser.hideTemplateChecks"), false); //$NON-NLS-1$
			initShowFilter = safeCast(memento.getBoolean("ObjectBrowser.showFilter"), true); //$NON-NLS-1$
			initShowStatus = safeCast(memento.getBoolean("ObjectBrowser.showStatusIndicator"), false); //$NON-NLS-1$
			initialObjectSelection = memento.getString("ObjectBrowser.selectedObject"); //$NON-NLS-1$
		}
		registerOpenHandlers();
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
		memento.putBoolean("ObjectBrowser.hideUnmanaged", objectTree.isHideUnmanaged()); //$NON-NLS-1$
		memento.putBoolean("ObjectBrowser.hideTemplateChecks", objectTree.isHideTemplateChecks()); //$NON-NLS-1$
		memento.putBoolean("ObjectBrowser.showFilter", objectTree.isFilterEnabled()); //$NON-NLS-1$
		memento.putBoolean("ObjectBrowser.showStatusIndicator", objectTree.isStatusIndicatorEnabled()); //$NON-NLS-1$
		saveSelection(memento);
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
		/*
		Object value = ConsoleSharedData.getProperty("ObjectBrowser.rootObjects"); //$NON-NLS-1$
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		*/
		
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
		objectTree.enableStatusIndicator(initShowStatus);
		
		objectTree.addOpenListener(new ObjectOpenListener() {
			@Override
			public boolean openObject(AbstractObject object)
			{
				return callOpenObjectHandler(object);
			}
		});
		
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
		objectTree.setFilterCloseAction(actionShowFilter);
		
		activateContext();
		restoreSelection();
	}
	
	/**
	 * Save tree viewer selection
	 * 
	 * @param memento
	 */
	private void saveSelection(IMemento memento)
	{
		ITreeSelection selection = (ITreeSelection)objectTree.getTreeViewer().getSelection();
		if (selection.size() == 1)
		{
			TreePath path = selection.getPaths()[0];
			StringBuilder sb = new StringBuilder();
			for(int i = 0; i < path.getSegmentCount(); i++)
			{
				sb.append('/');
				sb.append(((AbstractObject)path.getSegment(i)).getObjectId());
			}
			memento.putString("ObjectBrowser.selectedObject", sb.toString());
		}
		else
		{
			memento.putString("ObjectBrowser.selectedObject", "");
		}
	}
	
	/**
	 * Restore selection in the tree
	 */
	private void restoreSelection()
	{		
		if ((initialObjectSelection == null) || initialObjectSelection.isEmpty() || !initialObjectSelection.startsWith("/"))
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final String[] parts = initialObjectSelection.split("/");
		final Object[] elements = new Object[parts.length - 1];
		for(int i = 1; i < parts.length; i++)
		{
			long id;
			try
			{
				id = Long.parseLong(parts[i]);
			}
			catch(NumberFormatException e)
			{
				return;
			}
			elements[i - 1] = session.findObjectById(id);
			if (elements[i - 1] == null)
				return;	// path element is missing
		}
		objectTree.getTreeViewer().setSelection(new TreeSelection(new TreePath(elements)), true);
	}
	
	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.objectbrowser.context.ObjectBrowser"); //$NON-NLS-1$
		}
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionRefresh = new RefreshAction(this) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				objectTree.refresh();
			}
		};
		
		actionMoveObject = new Action(Messages.ObjectBrowser_MoveObject) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				moveObject(SubtreeType.INFRASTRUCTURE);
			}
		};
		
		actionMoveTemplate = new Action(Messages.ObjectBrowser_MoveTemplate) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				moveObject(SubtreeType.TEMPLATES);
			}
		};
		
		actionMoveBusinessService = new Action(Messages.ObjectBrowser_MoveService) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				moveObject(SubtreeType.BUSINESS_SERVICES);
			}
		};
		
      actionHideUnmanaged = new Action(Messages.ObjectBrowser_HideUnmanaged, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				objectTree.setHideUnmanaged(!objectTree.isHideUnmanaged());
		      actionHideUnmanaged.setChecked(objectTree.isHideUnmanaged());
			}
      };
      actionHideUnmanaged.setChecked(objectTree.isHideUnmanaged());

      actionHideTemplateChecks = new Action(Messages.ObjectBrowser_HideCheckTemplates, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				objectTree.setHideTemplateChecks(!objectTree.isHideTemplateChecks());
		      actionHideTemplateChecks.setChecked(objectTree.isHideTemplateChecks());
			}
      };
      actionHideTemplateChecks.setChecked(objectTree.isHideTemplateChecks());

      actionShowFilter = new Action(Messages.ObjectBrowser_ShowFilter, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				objectTree.enableFilter(!objectTree.isFilterEnabled());
				actionShowFilter.setChecked(objectTree.isFilterEnabled());
			}
      };
      actionShowFilter.setChecked(objectTree.isFilterEnabled());
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.show_object_filter"); //$NON-NLS-1$
		final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
      
      actionShowStatusIndicator = new Action(Messages.ObjectBrowser_ShowStatusIndicator, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				objectTree.enableStatusIndicator(actionShowStatusIndicator.isChecked());
			}
      };
      actionShowStatusIndicator.setChecked(objectTree.isStatusIndicatorEnabled());
      actionShowStatusIndicator.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.show_status_indicator"); //$NON-NLS-1$
		final ActionHandler showStatusIndicatorHandler = new ActionHandler(actionShowStatusIndicator);
		handlerService.activateHandler(actionShowStatusIndicator.getActionDefinitionId(), showStatusIndicatorHandler);

      actionProperties = new PropertyDialogAction(getSite(), objectTree.getTreeViewer());
	}

	/**
	 * Create view menu
	 */
   private void createMenu()
   {
      IMenuManager manager = getViewSite().getActionBars().getMenuManager();
      manager.add(actionShowFilter);
      manager.add(actionShowStatusIndicator);
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
			private static final long serialVersionUID = 1L;

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
		manager.add(new GroupMarker(IActionConstants.MB_NXVS));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_BINDING));
		if (isValidSelectionForMove(SubtreeType.INFRASTRUCTURE))
			manager.add(actionMoveObject);
		if (isValidSelectionForMove(SubtreeType.TEMPLATES))
			manager.add(actionMoveTemplate);
		if (isValidSelectionForMove(SubtreeType.BUSINESS_SERVICES))
			manager.add(actionMoveBusinessService);
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
	private boolean isValidSelectionForMove(SubtreeType subtree)
	{
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		if (selection.length != 1)
			return false;
		
		if (selection[0].getParentItem() == null)
			return false;
		
		final Object currentObject = selection[0].getData();
		final Object parentObject = selection[0].getParentItem().getData();
		
		switch(subtree)
		{
			case INFRASTRUCTURE:
				return ((currentObject instanceof Node) ||
		            (currentObject instanceof Cluster) ||
			         (currentObject instanceof Subnet) ||
		            (currentObject instanceof Condition) ||
		            (currentObject instanceof Rack) ||
			         (currentObject instanceof Container)) &&
			        ((parentObject instanceof Container) ||
			         (parentObject instanceof ServiceRoot));
			case TEMPLATES:
				return ((currentObject instanceof Template) ||
			         (currentObject instanceof TemplateGroup)) &&
			        ((parentObject instanceof TemplateGroup) ||
			         (parentObject instanceof TemplateRoot));
			case BUSINESS_SERVICES:
				return (currentObject instanceof BusinessService) &&
				       ((parentObject instanceof BusinessService) ||
						  (parentObject instanceof BusinessServiceRoot));
			default:
				return false;
		}
	}
	
	/**
	 * Move selected object to another container
	 */
	private void moveObject(SubtreeType subtree)
	{
		if (!isValidSelectionForMove(subtree))
			return;
		
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		final Object currentObject = selection[0].getData();
		final Object parentObject = selection[0].getParentItem().getData();
		
		Set<Integer> filter;
		switch(subtree)
		{
			case INFRASTRUCTURE:
				filter = ObjectSelectionDialog.createContainerSelectionFilter();
				break;
			case TEMPLATES:
				filter = ObjectSelectionDialog.createTemplateSelectionFilter();
				break;
			case BUSINESS_SERVICES:
				filter = ObjectSelectionDialog.createBusinessServiceSelectionFilter();
				break;
			default:
				filter = null;
				break;
		}
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, filter);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			final AbstractObject target = dlg.getSelectedObjects().get(0);
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.ObjectBrowser_MoveJob_Title + ((AbstractObject)currentObject).getObjectName(), this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					long objectId = ((AbstractObject)currentObject).getObjectId();
					session.bindObject(target.getObjectId(), objectId);
					session.unbindObject(((AbstractObject)parentObject).getObjectId(), objectId);
				}
	
				@Override
				protected String getErrorMessage()
				{
					return Messages.ObjectBrowser_MoveJob_Error + ((AbstractObject)currentObject).getObjectName();
				}
			}.start();
		}
	}
	
	/**
	 * Register object open handlers
	 */
	private void registerOpenHandlers()
	{
		// Read all registered extensions and create tabs
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectbrowser.objectOpenHandlers");
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final OpenHandlerData h = new OpenHandlerData();
				h.handler = (ObjectOpenHandler)elements[i].createExecutableExtension("class");
				h.priority = safeParseInt(elements[i].getAttribute("priority"));
				final String className = elements[i].getAttribute("enabledFor");
				try
				{
					h.enabledFor = (className != null) ? Class.forName(className) : null;
				}
				catch(Exception e)
				{
					h.enabledFor = null;
				}
				openHandlers.add(h);
			}
			catch(CoreException e)
			{
				e.printStackTrace();
			}
		}
		
		// Sort tabs by appearance order
		Collections.sort(openHandlers, new Comparator<OpenHandlerData>() {
			@Override
			public int compare(OpenHandlerData arg0, OpenHandlerData arg1)
			{
				return arg0.priority - arg1.priority;
			}
		});
	}
	
	/**
	 * @param s
	 * @return
	 */
	private static int safeParseInt(String s)
	{
		if (s == null)
			return 65535;
		try
		{
			return Integer.parseInt(s);
		}
		catch(NumberFormatException e)
		{
			return 65535;
		}
	}
	
	/**
	 * Call object open handler
	 * 
	 * @return
	 */
	private boolean callOpenObjectHandler(AbstractObject object)
	{
		for(OpenHandlerData h : openHandlers)
		{
			if ((h.enabledFor == null) || (h.enabledFor.isInstance(object)))
			{
				if (h.handler.openObject(object))
					return true;
			}
		}
		return false;
	}

	/**
	 * Internal data for object open handlers
	 */
	private class OpenHandlerData
	{
		ObjectOpenHandler handler;
		int priority;
		Class<?> enabledFor;
	}
}
