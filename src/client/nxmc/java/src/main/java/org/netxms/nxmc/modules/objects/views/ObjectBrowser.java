/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.AssetGroup;
import org.netxms.client.objects.AssetRoot;
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
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser - displays object tree.
 */
public class ObjectBrowser extends NavigationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectBrowser.class);

   private SubtreeType subtreeType;
   private ObjectTree objectTree;

   /**
    * @param name
    * @param image
    */
   public ObjectBrowser(String name, ImageDescriptor image, SubtreeType subtreeType)
   {
      super(name, image, "ObjectBrowser." + subtreeType.toString(), true, true, true);
      this.subtreeType = subtreeType;
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
      objectTree = new ObjectTree(parent, SWT.NONE, true, calculateClassFilter(subtreeType), this, true, false);

      Menu menu = new ObjectContextMenuManager(this, objectTree.getSelectionProvider(), objectTree.getTreeViewer()).createContextMenu(objectTree.getTreeControl());
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
            break;
         case INFRASTRUCTURE:
            classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
            classFilter.add(AbstractObject.OBJECT_CONTAINER);
            classFilter.add(AbstractObject.OBJECT_CLUSTER);
            classFilter.add(AbstractObject.OBJECT_CHASSIS);
            classFilter.add(AbstractObject.OBJECT_RACK);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            classFilter.add(AbstractObject.OBJECT_CONDITION);
            classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
            classFilter.add(AbstractObject.OBJECT_SENSOR);
            break;
         case MAPS:
            classFilter.add(AbstractObject.OBJECT_NETWORKMAP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
            break;
         case NETWORK:
            classFilter.add(AbstractObject.OBJECT_CLUSTER);
            classFilter.add(AbstractObject.OBJECT_NETWORK);
            classFilter.add(AbstractObject.OBJECT_ZONE);
            classFilter.add(AbstractObject.OBJECT_SUBNET);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            break;
         case TEMPLATES:
            classFilter.add(AbstractObject.OBJECT_TEMPLATE);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
            break;
      }
      return classFilter;
   }

   /**
    * Check if current selection is valid for moving object
    * 
    * @return true if current selection is valid for moving object
    */
   public boolean isValidSelectionForMove(SubtreeType subtree)
   {
      TreeItem[] selection = objectTree.getTreeControl().getSelection();
      if (selection.length < 1)
         return false;

      for(int i = 0; i < selection.length; i++)
      {
         if (!isValidObjectForMove(selection, i, subtree) ||
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

      switch(subtree)
      {
         case ASSETS:
            return ((currentObject instanceof Asset) ||
                    (currentObject instanceof AssetGroup)) &&
                   ((parentObject instanceof AssetGroup) ||
                    (parentObject instanceof AssetRoot)) ? true : false;
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
                 (parentObject instanceof Dashboard))) ? true : false;
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
   public void selectObject(AbstractObject object)
   {
      objectTree.selectObject(object);
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
