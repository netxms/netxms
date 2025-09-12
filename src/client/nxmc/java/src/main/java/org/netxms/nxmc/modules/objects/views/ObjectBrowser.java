/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerEditor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.AssetGroup;
import org.netxms.client.objects.AssetRoot;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.NetworkMapGroup;
import org.netxms.client.objects.NetworkMapRoot;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.NavigationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.SubtreeType;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser - displays object tree.
 */
public class ObjectBrowser extends NavigationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectBrowser.class);

   private SubtreeType subtreeType;
   private ObjectFilter objectFilter;
   private ObjectTree objectTree;

   private Action actionMoveObject;
   private Action actionMoveInterface;
   private Action actionMoveTemplate;
   private Action actionMoveBusinessService;
   private Action actionMoveDashboard;
   private Action actionMoveMap;
   private Action actionMoveAsset;

   /**
    * @param name
    * @param image
    */
   public ObjectBrowser(String name, ImageDescriptor image, SubtreeType subtreeType, ObjectFilter objectFilter)
   {
      super(name, image, "objects.browser." + subtreeType.toString().toLowerCase(), true, true, true);
      this.subtreeType = subtreeType;
      this.objectFilter = objectFilter;
   }

   /**
    * @see org.netxms.nxmc.base.views.NavigationView#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return (objectTree != null) ? objectTree.getSelectionProvider() : null;
   }

   /**
    * @see org.netxms.nxmc.base.views.NavigationView#setSelection(java.lang.Object)
    */
   @Override
   public void setSelection(Object selection)
   {
      objectTree.getTreeViewer().setSelection(new StructuredSelection(selection));
      objectTree.getTreeViewer().reveal(selection);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      objectTree = new ObjectTree(parent, SWT.NONE, true, calculateClassFilter(subtreeType), objectFilter, this, true, false);

      createActions();
      final MenuManager menuManager = new ObjectContextMenuManager(this, objectTree.getSelectionProvider(), objectTree.getTreeViewer()) {
         @Override
         protected void addObjectMoveActions(IStructuredSelection selection)
         {
            switch(subtreeType)
            {
               case INFRASTRUCTURE:
                  addInfrastructureTreeMoveAction(this, selection);
                  break;
               case TEMPLATES:
                  add(actionMoveTemplate);
                  break;
               case BUSINESS_SERVICES:
                  add(actionMoveBusinessService);
                  break;
               case DASHBOARDS:
                  add(actionMoveDashboard);
                  break;
               case MAPS:
                  add(actionMoveMap);
                  break;
               case ASSETS:
                  add(actionMoveAsset);
                  break;
               default:
                  break;
            }
         }
      };
      Menu menu = menuManager.createContextMenu(objectTree.getTreeControl());
      objectTree.getTreeControl().setMenu(menu);

      objectTree.enableDropSupport(this);
      objectTree.enableDragSupport();

      final TreeViewer treeViewer = objectTree.getTreeViewer();
      TreeViewerEditor.create(treeViewer, new ColumnViewerEditorActivationStrategy(treeViewer) {
         @Override
         protected boolean isEditorActivationEvent(ColumnViewerEditorActivationEvent event)
         {
            return event.eventType == ColumnViewerEditorActivationEvent.PROGRAMMATIC;
         }
      }, ColumnViewerEditor.DEFAULT);

      TextCellEditor editor = new TextCellEditor(treeViewer.getTree(), SWT.BORDER);
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

      treeViewer.setCellEditors(new CellEditor[] { editor });
      treeViewer.setColumnProperties(new String[] { "name" });
      treeViewer.setCellModifier(new ICellModifier() {
         @Override
         public void modify(Object element, String property, Object value)
         {
            objectTree.enableRefresh();

            final Object data = (element instanceof Item) ? ((Item)element).getData() : element;
            if (property.equals("name") && (data instanceof AbstractObject))
            {
               final NXCSession session = Registry.getSession();
               final String newName = value.toString();
               new Job(i18n.tr("Renaming object"), ObjectBrowser.this) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.setObjectName(((AbstractObject)data).getObjectId(), newName);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format(i18n.tr("Cannot rename object %s"), ((AbstractObject)data).getObjectName());
                  }
               }.start();
            }
         }

         @Override
         public Object getValue(Object element, String property)
         {
            if (property.equals("name") && (element instanceof AbstractObject))
               return ((AbstractObject)element).getObjectName();
            return null;
         }

         @Override
         public boolean canModify(Object element, String property)
         {
            if (!property.equals("name"))
               return false;

            objectTree.disableRefresh();
            return true;
         }
      });

      objectTree.getTreeViewer().expandToLevel(2);
   }

   /**
    * Add correct move action to object context menu in infrastructure tree.
    *
    * @param manager menu manager
    * @param selection current selection
    */
   private void addInfrastructureTreeMoveAction(ObjectContextMenuManager manager, IStructuredSelection selection)
   {
      boolean moveInterface = false;
      for(Object o : selection.toList())
      {
         if (o instanceof Interface)
            moveInterface = true;
         else if (moveInterface)
            return; // Mixed selection, cannot add move action
      }

      if (!moveInterface)
      {
         manager.add(actionMoveObject);
         return;
      }

      // Check if all selected elements under same parent class (node or circuit)
      boolean canMove = true;
      TreeItem[] treeSelection = objectTree.getTreeControl().getSelection();
      for(int i = 0; i < treeSelection.length; i++)
      {
         AbstractObject parent = (AbstractObject)treeSelection[i].getParentItem().getData();
         if (!(parent instanceof Circuit))
            canMove = false;
      }

      if (canMove)
         manager.add(actionMoveInterface);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionMoveObject = new Action(i18n.tr("Move to another container...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.INFRASTRUCTURE);
         }
      };

      actionMoveInterface = new Action(i18n.tr("Move to another circuit...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.INFRASTRUCTURE);
         }
      };

      actionMoveTemplate = new Action(i18n.tr("Move to another group...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.TEMPLATES);
         }
      };

      actionMoveBusinessService = new Action(i18n.tr("Move to another group...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.BUSINESS_SERVICES);
         }
      };

      actionMoveDashboard = new Action(i18n.tr("Move to another group...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.DASHBOARDS);
         }
      };

      actionMoveMap = new Action(i18n.tr("Move to another group...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.MAPS);
         }
      };

      actionMoveAsset = new Action(i18n.tr("Move to another group...")) {
         @Override
         public void run()
         {
            moveObject(SubtreeType.ASSETS);
         }
      };
   }

   /**
    * Move selected objects to another container
    */
   private void moveObject(SubtreeType subtree)
   {
      if (!isValidSelectionForMove(subtree, true))
         return;

      List<AbstractObject> selectedObjects = new ArrayList<AbstractObject>();
      List<AbstractObject> parentObjects = new ArrayList<AbstractObject>();

      TreeItem[] selection = objectTree.getTreeControl().getSelection();
      for (int i = 0; i < selection.length; i++)
      {
         selectedObjects.add((AbstractObject)selection[i].getData());
         parentObjects.add((AbstractObject)selection[i].getParentItem().getData());
      }

      Set<Integer> filter;
      switch(subtree)
      {
         case INFRASTRUCTURE:
            if (selectedObjects.get(0) instanceof Interface)
               filter = ObjectSelectionDialog.createCircuitSelectionFilter();
            else
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
         case ASSETS:
            filter = ObjectSelectionDialog.createAssetGroupsSelectionFilter();
            break;
         default:
            filter = null;
            break;
      }
      
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), filter, selectedObjects);
      dlg.enableMultiSelection(false);
      if (dlg.open() == Window.OK)
      {
         final AbstractObject target = dlg.getSelectedObjects().get(0);
         for (int i = 0; i < selection.length; i++)
         {
            moveObject(target, parentObjects.get(i), selectedObjects.get(i), true);
         }
      }
   }

   /**
    * Calculate class filter based on subtree type.
    *
    * @return root objects
    */
   public static Set<Integer> calculateClassFilter(SubtreeType subtreeType)
   {
      Set<Integer> classFilter = new HashSet<Integer>();
      switch(subtreeType)
      {
         case ASSETS:
            classFilter.add(AbstractObject.OBJECT_ASSET);
            classFilter.add(AbstractObject.OBJECT_ASSETGROUP);
            classFilter.add(AbstractObject.OBJECT_ASSETROOT);
            break;
         case BUSINESS_SERVICES:
            classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICE);
            classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE);
            classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEROOT);
            break;
         case DASHBOARDS:
            classFilter.add(AbstractObject.OBJECT_DASHBOARD);
            classFilter.add(AbstractObject.OBJECT_DASHBOARDGROUP);
            classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
            classFilter.add(AbstractObject.OBJECT_DASHBOARDTEMPLATE);
            break;
         case INFRASTRUCTURE:
            classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
            classFilter.add(AbstractObject.OBJECT_CHASSIS);
            classFilter.add(AbstractObject.OBJECT_CIRCUIT);
            classFilter.add(AbstractObject.OBJECT_CLUSTER);
            classFilter.add(AbstractObject.OBJECT_COLLECTOR);
            classFilter.add(AbstractObject.OBJECT_CONDITION);
            classFilter.add(AbstractObject.OBJECT_CONTAINER);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_RACK);
            classFilter.add(AbstractObject.OBJECT_SENSOR);
            classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
            classFilter.add(AbstractObject.OBJECT_SUBNET);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_WIRELESSDOMAIN);
            break;
         case MAPS:
            classFilter.add(AbstractObject.OBJECT_NETWORKMAP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
            break;
         case NETWORK:
            classFilter.add(AbstractObject.OBJECT_CLUSTER);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_NETWORK);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_SUBNET);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_ZONE);
            break;
         case TEMPLATES:
            classFilter.add(AbstractObject.OBJECT_TEMPLATE);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
            if (Registry.getSession().getClientConfigurationHintAsBoolean("ObjectBrowser.ShowTargetsUnderTemplates", false))
            {
               classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
               classFilter.add(AbstractObject.OBJECT_CLUSTER);
               classFilter.add(AbstractObject.OBJECT_COLLECTOR);
               classFilter.add(AbstractObject.OBJECT_INTERFACE);
               classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
               classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
               classFilter.add(AbstractObject.OBJECT_NODE);
               classFilter.add(AbstractObject.OBJECT_SENSOR);
               classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            }
            break;
      }
      return classFilter;
   }

   /**
    * Check if current selection is valid for copying or moving object
    *
    * @param subtree object subtree to check on
    * @param move true if operation is to move object
    * @return true if current selection is valid for moving object
    */
   public boolean isValidSelectionForMove(SubtreeType subtree, boolean move)
   {
      TreeItem[] selection = objectTree.getTreeControl().getSelection();
      if (selection.length < 1)
         return false;

      long commonParentId = ((AbstractObject)selection[0].getParentItem().getData()).getObjectId();
      for(int i = 0; i < selection.length; i++)
      {
         if (!isValidObjectForMove(selection[i], subtree, move) || ((AbstractObject)selection[i].getParentItem().getData()).getObjectId() != commonParentId)
            return false;
      }
      return true;
   }

   /**
    * Check if given selection object is valid for copy or move
    *
    * @param item current tree selection item
    * @param subtree object subtree to check on
    * @param move true if operation is to move object
    * @return true if current selection object is valid for moving object
    */
   private boolean isValidObjectForMove(TreeItem item, SubtreeType subtree, boolean move)
   {
      if (item.getParentItem() == null)
         return false;

      final AbstractObject currentObject = (AbstractObject)item.getData();
      final AbstractObject parentObject = (AbstractObject)item.getParentItem().getData();

      switch(subtree)
      {
         case ASSETS:
            return ((currentObject instanceof Asset) ||
                    (currentObject instanceof AssetGroup)) &&
                   ((parentObject instanceof AssetGroup) ||
                    (parentObject instanceof AssetRoot)) ? true : false;
         case INFRASTRUCTURE:
            if (currentObject instanceof Interface)
               return !move || (parentObject instanceof Circuit);
            return ((currentObject instanceof Node) ||
                    (currentObject instanceof Cluster) ||
                    (currentObject instanceof Subnet) ||
                    (currentObject instanceof Condition) ||
                    (currentObject instanceof Rack) ||
                    (currentObject instanceof MobileDevice) ||
                    (currentObject instanceof Circuit) || 
                    (currentObject instanceof Collector) || 
                    (currentObject instanceof Container) || 
                    (currentObject instanceof Sensor)) &&
                   ((parentObject instanceof Collector) || 
                    (parentObject instanceof Container) ||
                    (parentObject instanceof ServiceRoot)) ? true : false;
         case TEMPLATES:
            return ((currentObject instanceof Template) ||
                    (currentObject instanceof TemplateGroup)) &&
                   ((parentObject instanceof TemplateGroup) ||
                    (parentObject instanceof TemplateRoot)) ? true : false;
         case BUSINESS_SERVICES:
            return (currentObject instanceof BusinessService) &&
                   ((parentObject instanceof BusinessService) ||
                    (parentObject instanceof BusinessServiceRoot)) ? true : false;
         case MAPS:
            return ((currentObject instanceof NetworkMap) ||
                  (currentObject instanceof NetworkMapGroup)) &&
                  ((parentObject instanceof NetworkMapGroup) ||
                  (parentObject instanceof NetworkMapRoot)) ? true : false;
         case DASHBOARDS:
            return (((currentObject instanceof Dashboard) ||
                  (currentObject instanceof DashboardGroup)) &&
                 ((parentObject instanceof DashboardRoot) ||
                 (parentObject instanceof DashboardGroup) ||
                 (parentObject instanceof Dashboard)) || 
                 ((currentObject instanceof DashboardTemplate) &&
                 ((parentObject instanceof DashboardRoot) || 
                 (parentObject instanceof DashboardGroup)))) ? true : false;
         default:
            return false;
      }
   }

   /**
    * Move object to new parent
    *
    * @param destination destination (new parent for movable object)
    * @param source source (current parent for movable object)
    * @param movableObject object to move
    * @param move true if operation is a "true" move (object should be removed from old parent)
    */
   public void moveObject(final AbstractObject destination, final AbstractObject source, final AbstractObject movableObject, final boolean move)
   {
      if (destination.getObjectId() == source.getObjectId())
         return;

      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Moving object %s"), movableObject.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long objectId = ((AbstractObject)movableObject).getObjectId();
            session.bindObject(destination.getObjectId(), objectId);
            if (move)
               session.unbindObject(source.getObjectId(), objectId);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot move object %s"), ((AbstractObject)movableObject).getObjectName());
         }
      }.start();
   }
   
   /**
    * Select object 
    * 
    * @param objectId id of the object to be selected
    * @return true if selection occurred
    */
   public boolean selectObject(AbstractObject object)
   {
      objectTree.selectObject(object);
      if (objectTree.getFirstSelectedObject() != object.getObjectId())
      {
         objectTree.setFilterText("");
         objectTree.selectObject(object);
      }
      return objectTree.getFirstSelectedObject() == object.getObjectId();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      objectTree.refresh();
   }
}
