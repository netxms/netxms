/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITreeSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerEditor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.NetworkMapGroup;
import org.netxms.client.objects.NetworkMapRoot;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.PolicyGroup;
import org.netxms.client.objects.PolicyRoot;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectActionValidator;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandlerRegistry;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenListener;
import org.netxms.ui.eclipse.objectbrowser.api.SubtreeType;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.CommandBridge;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;

/**
 * Object browser view
 */
public class ObjectBrowser extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.view.navigation.objectbrowser"; //$NON-NLS-1$
	
	private ObjectTree objectTree;
	private Action actionShowFilter;
	private Action actionShowStatusIndicator;
	private Action actionHideUnmanaged;
	private Action actionHideTemplateChecks;
   private Action actionHideSubInterfaces;
	private Action actionMoveObject;
	private Action actionMoveTemplate;
	private Action actionMoveBusinessService;
	private Action actionMoveDashboard;
	private Action actionMoveMap;
	private Action actionMovePolicy;
	private Action actionRefresh;
	private Action actionRenameObject;
	private boolean initHideUnmanaged = false;
	private boolean initHideTemplateChecks = false;
   private boolean initHideSubInterfaces = false;
	private boolean initShowFilter = true;
	private boolean initShowStatus = false;
	private String initialObjectSelection = null;
	private ObjectOpenHandlerRegistry openHandlers;
	private ObjectActionValidator[] actionValidators;
	
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
			initHideSubInterfaces = safeCast(memento.getBoolean("ObjectBrowser.hideSubInterfaces"), false); //$NON-NLS-1$
			initShowStatus = safeCast(memento.getBoolean("ObjectBrowser.showStatusIndicator"), false); //$NON-NLS-1$
			initialObjectSelection = memento.getString("ObjectBrowser.selectedObject"); //$NON-NLS-1$
		}
		openHandlers = new ObjectOpenHandlerRegistry();
		registerActionValidators();
		
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		initShowFilter = safeCast(settings.get("ObjectBrowser.showFilter"), settings.getBoolean("ObjectBrowser.showFilter"), initShowFilter);
	}

	/**
	 * @param b
	 * @param defval
	 * @return
	 */
	private static boolean safeCast(Boolean b, boolean defval)
	{
		return (b != null) ? b : defval;
	}
	
	private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
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
      memento.putBoolean("ObjectBrowser.hideSubInterfaces", objectTree.isHideSubInterfaces()); //$NON-NLS-1$
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
		Object value = ConsoleSharedData.getProperty("ObjectBrowser.rootObjects"); //$NON-NLS-1$
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.MULTI, rootObjects, null, true, true);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.setLayoutData(fd);

		objectTree.setHideSubInterfaces(initHideSubInterfaces);
		objectTree.setHideTemplateChecks(initHideTemplateChecks);
		objectTree.setHideUnmanaged(initHideUnmanaged);
		objectTree.enableFilter(initShowFilter);
		objectTree.enableStatusIndicator(initShowStatus);
		
		objectTree.addOpenListener(new ObjectOpenListener() {
			@Override
			public boolean openObject(AbstractObject object)
			{
				return openHandlers.callOpenObjectHandler(object);
			}
		});
		
		createActions();
		createMenu();
		createToolBar();
		createPopupMenu();
		
      objectTree.enableDropSupport(this);
		objectTree.enableDragSupport();
		getSite().setSelectionProvider(objectTree.getTreeViewer());
		
		objectTree.getTreeViewer().addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				int size = ((IStructuredSelection)objectTree.getTreeViewer().getSelection()).size();
				actionMoveObject.setEnabled(size == 1);
			}
		});
		objectTree.setFilterCloseAction(new Action() {
         @Override
         public void run()
         {
            actionShowFilter.setChecked(false);
            objectTree.enableFilter(false);
         }
      });
		
		final TreeViewer tree = objectTree.getTreeViewer();		
		TreeViewerEditor.create(tree, new ColumnViewerEditorActivationStrategy(tree) {
         @Override
         protected boolean isEditorActivationEvent(ColumnViewerEditorActivationEvent event)
         {
            return event.eventType == ColumnViewerEditorActivationEvent.PROGRAMMATIC;
         }
      }, ColumnViewerEditor.DEFAULT);
		
		TextCellEditor editor =  new TextCellEditor(tree.getTree(), SWT.BORDER);		
		editor.addListener(new ICellEditorListener() {
         
         @Override
         public void editorValueChanged(boolean oldValidState, boolean newValidState)
         {
         }
         
         @Override
         public void cancelEditor()
         {
            objectTree.enableRefresh();
         }
         
         @Override
         public void applyEditorValue()
         {
         }
      });
		
		//TODO: override editor method that creates control to disable refresh
		
		tree.setCellEditors(new CellEditor[] { editor });
		tree.setColumnProperties(new String[] { "name" }); //$NON-NLS-1$
		tree.setCellModifier(new ICellModifier() {
         @Override
         public void modify(Object element, String property, Object value)
         {
            final Object data = (element instanceof Item) ? ((Item)element).getData() : element;

            if (property.equals("name")) //$NON-NLS-1$
            {
               if (data instanceof AbstractObject)
               {
                  final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
                  final String newName = value.toString();
                  new ConsoleJob(Messages.get().ObjectBrowser_RenameJobName, null, Activator.PLUGIN_ID, null) {
                     @Override
                     protected void runInternal(IProgressMonitor monitor) throws Exception
                     {
                        session.setObjectName(((AbstractObject)data).getObjectId(), newName);
                     }
            
                     @Override
                     protected String getErrorMessage()
                     {
                        return String.format(Messages.get().ObjectBrowser_RenameJobError, ((AbstractObject)data).getObjectName());
                     }
                  }.start();
               }
            }
            objectTree.enableRefresh();
         }

         @Override
         public Object getValue(Object element, String property)
         {
            if (property.equals("name")) //$NON-NLS-1$
            {
               if (element instanceof AbstractObject)
               {
                  return ((AbstractObject)element).getObjectName();
               }
            }
            return null;
         }

         @Override
         public boolean canModify(Object element, String property)
         {
            if (property.equals("name")) //$NON-NLS-1$
            {
               objectTree.disableRefresh();               
               return true;
            }
            return false; //$NON-NLS-1$
         }
      });
		
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
			memento.putString("ObjectBrowser.selectedObject", sb.toString()); //$NON-NLS-1$
		}
		else
		{
			memento.putString("ObjectBrowser.selectedObject", ""); //$NON-NLS-1$ //$NON-NLS-2$
		}
	}
	
	/**
	 * Restore selection in the tree
	 */
	private void restoreSelection()
	{		
		if ((initialObjectSelection == null) || initialObjectSelection.isEmpty() || !initialObjectSelection.startsWith("/")) //$NON-NLS-1$
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final String[] parts = initialObjectSelection.split("/"); //$NON-NLS-1$
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
		
		final Display display = getSite().getShell().getDisplay();
		display.asyncExec(new Runnable() {
			@Override
			public void run()
			{
				while(display.readAndDispatch()); // wait for events to finish before continue
				CommandBridge.getInstance().execute("TabbedObjectView/changeObject", ((AbstractObject)elements[elements.length - 1]).getObjectId()); //$NON-NLS-1$
			}
		});
	}
	
	/**
	 * Set selection in the tree and in object details view.
	 * 
	 * @param objectId ID of object to be selected
	 * @param tabId tab ID to be selected or null if tab selection not needed
	 */
	public void setSelection(long objectId, String tabId)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final AbstractObject object = session.findObjectById(objectId);
		if (object != null)
		{
			objectTree.getTreeViewer().setSelection(new StructuredSelection(object), true);
			CommandBridge.getInstance().execute("TabbedObjectView/changeObject", objectId); //$NON-NLS-1$
			if (tabId != null)
			{		
				CommandBridge.getInstance().execute("TabbedObjectView/selectTab", tabId); //$NON-NLS-1$
			}
		}
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
			@Override
			public void run()
			{
				objectTree.refresh();
			}
		};
		
		actionMoveObject = new Action(Messages.get().ObjectBrowser_MoveObject) {
			@Override
			public void run()
			{
				moveObject(SubtreeType.INFRASTRUCTURE);
			}
		};
		actionMoveObject.setId("org.netxms.ui.eclipse.objectbrowser.actions.moveObject"); //$NON-NLS-1$
		
		actionMoveTemplate = new Action(Messages.get().ObjectBrowser_MoveTemplate) {
			@Override
			public void run()
			{
				moveObject(SubtreeType.TEMPLATES);
			}
		};
		actionMoveTemplate.setId("org.netxms.ui.eclipse.objectbrowser.actions.moveTemplate"); //$NON-NLS-1$
		
		actionMoveBusinessService = new Action(Messages.get().ObjectBrowser_MoveService) {
			@Override
			public void run()
			{
				moveObject(SubtreeType.BUSINESS_SERVICES);
			}
		};
		actionMoveBusinessService.setId("org.netxms.ui.eclipse.objectbrowser.actions.moveBusinessService"); //$NON-NLS-1$
		
		actionMoveDashboard = new Action(Messages.get().ObjectBrowser_MoveDashboard) { 
         @Override
         public void run()
         {
            moveObject(SubtreeType.DASHBOARDS);
         }
      };
      actionMoveDashboard.setId("org.netxms.ui.eclipse.objectbrowser.actions.moveDashboard"); //$NON-NLS-1$
      
      actionMoveMap = new Action(Messages.get().ObjectBrowser_MoveMap) { 
         @Override
         public void run()
         {
            moveObject(SubtreeType.MAPS);
         }
      };
      actionMoveMap.setId("org.netxms.ui.eclipse.objectbrowser.actions.moveMap"); //$NON-NLS-1$
      
      actionMovePolicy = new Action(Messages.get().ObjectBrowser_MovePolicy) { 
         @Override
         public void run()
         {
            moveObject(SubtreeType.POLICIES);
         }
      };
      actionMovePolicy.setId("org.netxms.ui.eclipse.objectbrowser.actions.movePolicy"); //$NON-NLS-1$
		
      actionHideUnmanaged = new Action(Messages.get().ObjectBrowser_HideUnmanaged, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				objectTree.setHideUnmanaged(actionHideUnmanaged.isChecked());
			}
      };
      actionHideUnmanaged.setId("org.netxms.ui.eclipse.objectbrowser.actions.hideUnmanaged"); //$NON-NLS-1$
      actionHideUnmanaged.setChecked(objectTree.isHideUnmanaged());

      actionHideTemplateChecks = new Action(Messages.get().ObjectBrowser_HideCheckTemplates, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				objectTree.setHideTemplateChecks(actionHideTemplateChecks.isChecked());
			}
      };
      actionHideTemplateChecks.setId("org.netxms.ui.eclipse.objectbrowser.actions.hideTemplateChecks"); //$NON-NLS-1$
      actionHideTemplateChecks.setChecked(objectTree.isHideTemplateChecks());

      actionHideSubInterfaces = new Action(Messages.get().ObjectBrowser_HideSubInterfaces, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            objectTree.setHideSubInterfaces(actionHideSubInterfaces.isChecked());
         }
      };
      actionHideSubInterfaces.setId("org.netxms.ui.eclipse.objectbrowser.actions.hideSubInterfaces"); //$NON-NLS-1$
      actionHideSubInterfaces.setChecked(objectTree.isHideSubInterfaces());

      actionShowFilter = new Action(Messages.get().ObjectBrowser_ShowFilter, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				objectTree.enableFilter(actionShowFilter.isChecked());
			}
      };
      actionShowFilter.setId("org.netxms.ui.eclipse.objectbrowser.actions.showFilter"); //$NON-NLS-1$
      actionShowFilter.setChecked(objectTree.isFilterEnabled());
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.show_object_filter"); //$NON-NLS-1$
		final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
      
      actionShowStatusIndicator = new Action(Messages.get().ObjectBrowser_ShowStatusIndicator, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				objectTree.enableStatusIndicator(actionShowStatusIndicator.isChecked());
			}
      };
      actionShowStatusIndicator.setId("org.netxms.ui.eclipse.objectbrowser.actions.showStatusIndicator"); //$NON-NLS-1$
      actionShowStatusIndicator.setChecked(objectTree.isStatusIndicatorEnabled());
      actionShowStatusIndicator.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.show_status_indicator"); //$NON-NLS-1$
		final ActionHandler showStatusIndicatorHandler = new ActionHandler(actionShowStatusIndicator);
		handlerService.activateHandler(actionShowStatusIndicator.getActionDefinitionId(), showStatusIndicatorHandler);
		
		actionRenameObject = new Action(Messages.get().ObjectBrowser_Rename)
		{
		   @Override
		   public void run()
		   {
		      TreeItem[] selection = objectTree.getTreeControl().getSelection();
		      if(selection.length != 1)
		         return;
		      objectTree.getTreeViewer().editElement(selection[0].getData(), 0);
		   }
		};
		actionRenameObject.setId("org.netxms.ui.eclipse.objectbrowser.actions.rename"); //$NON-NLS-1$
		actionRenameObject.setActionDefinitionId("org.netxms.ui.eclipse.objectbrowser.commands.rename_object"); //$NON-NLS-1$
      final ActionHandler renameObjectHandler = new ActionHandler(actionRenameObject);
      handlerService.activateHandler(actionRenameObject.getActionDefinitionId(), renameObjectHandler);
	}

	/**
	 * Create view menu
	 */
   private void createMenu()
   {
      IMenuManager manager = getViewSite().getActionBars().getMenuManager();
      FilteringMenuManager.add(manager, actionShowFilter, Activator.PLUGIN_ID);
      FilteringMenuManager.add(manager, actionShowStatusIndicator, Activator.PLUGIN_ID);
      FilteringMenuManager.add(manager, actionHideSubInterfaces, Activator.PLUGIN_ID);
      FilteringMenuManager.add(manager, actionHideUnmanaged, Activator.PLUGIN_ID);
      FilteringMenuManager.add(manager, actionHideTemplateChecks, Activator.PLUGIN_ID);
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator());
      FilteringMenuManager.add(manager, actionRefresh, Activator.PLUGIN_ID);
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
		MenuManager manager = new FilteringMenuManager(Activator.PLUGIN_ID);
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
	   ObjectContextMenu.fill(manager, getSite(), objectTree.getTreeViewer());
	
      if (isValidSelectionForMove(SubtreeType.INFRASTRUCTURE))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMoveObject);
      if (isValidSelectionForMove(SubtreeType.TEMPLATES))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMoveTemplate);
      if (isValidSelectionForMove(SubtreeType.BUSINESS_SERVICES))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMoveBusinessService);
      if (isValidSelectionForMove(SubtreeType.DASHBOARDS))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMoveDashboard);
      if (isValidSelectionForMove(SubtreeType.MAPS))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMoveMap);
      if (isValidSelectionForMove(SubtreeType.POLICIES))
         manager.insertAfter(GroupMarkers.MB_OBJECT_BINDING, actionMovePolicy);
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
	   IDialogSettings settings = Activator.getDefault().getDialogSettings();
	   settings.put("ObjectBrowser.showFilter", objectTree.isFilterEnabled());
		super.dispose();
	}
	
	/**
	 * Check if current selection is valid for moving object
	 * 
	 * @return true if current selection is valid for moving object
	 */
	public boolean isValidSelectionForMove(SubtreeType subtree)
	{
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		if(selection.length < 1)
		   return false;
		
	   for(int i = 0; i < selection.length; i++)
	   {
	      if(!isValidObjectForMove(selection, i, subtree) || 
	            ((AbstractObject)selection[0].getParentItem().getData()).getObjectId() != ((AbstractObject)selection[i].getParentItem().getData()).getObjectId())	   
	         return false;
	   }
	   return true;		
	}
	
	/**
    * Check if given selection object is valid for move
    * 
    * @return true if current selection object is valid for moving object
    */
	public boolean isValidObjectForMove(TreeItem[] selection, int i, SubtreeType subtree)
	{
	   if (selection[i].getParentItem() == null)
         return false;
      
      final AbstractObject currentObject = (AbstractObject)selection[i].getData();
      final AbstractObject parentObject = (AbstractObject)selection[i].getParentItem().getData();
      
      for(ObjectActionValidator v : actionValidators)
      {
         int result = v.isValidSelectionForMove(subtree, currentObject, parentObject);
         if (result == ObjectActionValidator.APPROVE)
            return true;
         if (result == ObjectActionValidator.REJECT)
            return false;
      }
      return false;
	}
	
	/**
	 * Move selected objects to another container
	 */
	private void moveObject(SubtreeType subtree)
	{
		if (!isValidSelectionForMove(subtree))
			return;
		
		List<Object> currentObject = new ArrayList<Object>();
		List<Object> parentObject = new ArrayList<Object>();
		
		TreeItem[] selection = objectTree.getTreeControl().getSelection();
		for (int i = 0; i < selection.length; i++)
		{
   		currentObject.add(selection[i].getData());
   		parentObject.add(selection[i].getParentItem().getData());
		}
		
		Set<Integer> filter;
		switch(subtree)
		{
			case INFRASTRUCTURE:
				filter = ObjectSelectionDialog.createContainerSelectionFilter();
				break;
			case TEMPLATES:
				filter = ObjectSelectionDialog.createTemplateGroupSelectionFilter();
				break;
			case BUSINESS_SERVICES:
				filter = ObjectSelectionDialog.createBusinessServiceSelectionFilter();
				break;
			case DASHBOARDS:
			   filter = ObjectSelectionDialog.createDashboardGroupSelectionFilter();
			   break;
			case MAPS:
			   filter = ObjectSelectionDialog.createNetworkMapGroupsSelectionFilter();
			   break;
			case POLICIES:
			   filter = ObjectSelectionDialog.createPolicyGroupSelectionFilter();
			   break;
			default:
				filter = null;
				break;
		}
		
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, filter, currentObject);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
	      final AbstractObject target = dlg.getSelectedObjects().get(0);
	      for (int i = 0; i < selection.length; i++)
	      {
	         performObjectMove(target, parentObject.get(i), currentObject.get(i), true);
	      }
		}
	}
	
	public void performObjectMove(final AbstractObject target, final Object parentObject, final Object currentObject, final boolean isMove){
      if (target.getObjectId() != ((AbstractObject)parentObject).getObjectId())
      {
         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         
         new ConsoleJob(Messages.get().ObjectBrowser_MoveJob_Title + ((AbstractObject)currentObject).getObjectName(), this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               long objectId = ((AbstractObject)currentObject).getObjectId();
               session.bindObject(target.getObjectId(), objectId);
               if (isMove)
                  session.unbindObject(((AbstractObject)parentObject).getObjectId(), objectId);
            }
   
            @Override
            protected String getErrorMessage()
            {
               return Messages.get().ObjectBrowser_MoveJob_Error + ((AbstractObject)currentObject).getObjectName();
            }
         }.start();
      }
	}
	
	/**
	 * Register object action validators
	 */
	private void registerActionValidators()
	{
		List<ActionValidatorData> list = new ArrayList<ActionValidatorData>();
		
		// Read all registered extensions and create validators
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectbrowser.objectActionValidators"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ActionValidatorData v = new ActionValidatorData();
				v.validator = (ObjectActionValidator)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				v.priority = safeParseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
				list.add(v);
			}
			catch(CoreException e)
			{
				e.printStackTrace();
			}
		}
		
		// Sort handlers by priority
		Collections.sort(list, new Comparator<ActionValidatorData>() {
			@Override
			public int compare(ActionValidatorData arg0, ActionValidatorData arg1)
			{
				return arg0.priority - arg1.priority;
			}
		});
		
		actionValidators = new ObjectActionValidator[list.size() + 1];
		int i = 0;
		for(ActionValidatorData v : list)
			actionValidators[i++] = v.validator;
		
		// Default validator
		actionValidators[i] = new ObjectActionValidator() {
			@Override
			public int isValidSelectionForMove(SubtreeType subtree, AbstractObject currentObject, AbstractObject parentObject)
			{
				switch(subtree)
				{
					case INFRASTRUCTURE:
						return ((currentObject instanceof Node) ||
						        (currentObject instanceof Cluster) ||
					           (currentObject instanceof Subnet) ||
				              (currentObject instanceof Condition) ||
				              (currentObject instanceof Rack) ||
                          (currentObject instanceof MobileDevice) ||
					           (currentObject instanceof Container) || 
					           (currentObject instanceof Sensor)) &&
					          ((parentObject instanceof Container) ||
					           (parentObject instanceof ServiceRoot)) ? APPROVE : REJECT;
					case TEMPLATES:
						return ((currentObject instanceof Template) ||
					           (currentObject instanceof TemplateGroup)) &&
					          ((parentObject instanceof TemplateGroup) ||
					           (parentObject instanceof TemplateRoot)) ? APPROVE : REJECT;
					case BUSINESS_SERVICES:
						return (currentObject instanceof BusinessService) &&
						       ((parentObject instanceof BusinessService) ||
								  (parentObject instanceof BusinessServiceRoot)) ? APPROVE : REJECT;
					case MAPS:
					   return ((currentObject instanceof NetworkMap) ||
					         (currentObject instanceof NetworkMapGroup)) &&
					         ((parentObject instanceof NetworkMapGroup) ||
                        (parentObject instanceof NetworkMapRoot)) ? APPROVE : REJECT;
					case DASHBOARDS:
					   return (((currentObject instanceof Dashboard) ||
                        (currentObject instanceof DashboardGroup)) &&
                       ((parentObject instanceof DashboardRoot) ||
                       (parentObject instanceof DashboardGroup) ||
                       (parentObject instanceof Dashboard))) ? APPROVE : REJECT;
					default:
						return REJECT;
				}
			}
		};
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
	 * Internal data for object action validators
	 */
	private class ActionValidatorData
	{
		ObjectActionValidator validator;
		int priority;
	}
}
