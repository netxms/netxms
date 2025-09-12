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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.TreeEvent;
import org.eclipse.swt.events.TreeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.modules.objects.ObjectOpenListener;
import org.netxms.nxmc.modules.objects.SubtreeType;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectTreeComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectTreeContentProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectTreeViewer;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectViewerFilter;
import org.netxms.nxmc.tools.RefreshTimer;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Object tree control
 */
public class ObjectTree extends Composite
{
   private View view;
   private ObjectTreeViewer objectTree;
   private FilterText filterText;
   private ObjectViewerFilter filter;
   private SessionListener sessionListener = null;
   private NXCSession session = null;
   private Map<Long, AbstractObject> updatedObjects = new HashMap<>();
   private boolean fullRefresh = false;
   private RefreshTimer refreshTimer;
   private ObjectStatusIndicator statusIndicator = null;
   private SelectionListener statusIndicatorSelectionListener = null;
   private TreeListener statusIndicatorTreeListener;
   private Set<ObjectOpenListener> openListeners = new HashSet<ObjectOpenListener>(0);
   private ObjectTreeContentProvider contentProvider;
   private boolean filterEnabled = true;
   private boolean statusIndicatorEnabled = false;
   private boolean objectsFullySync;

   /**
    * Create object tree control. If vie wis specified then use view's filter instead of own.
    *
    * @param parent parent composite
    * @param style control style bits
    * @param multiSelection true to enable selection of multiple objects
    * @param classFilter class filter for objects to be displayed
    * @param objectFilter additional filter for top level objects (optional, can be null)
    * @param view owning view or null
    * @param showFilterToolTip tru to show filter's tooltips
    * @param showFilterCloseButton true to show filter's close button
    */
   public ObjectTree(Composite parent, int style, boolean multiSelection, Set<Integer> classFilter, ObjectFilter objectFilter, View view, boolean showFilterToolTip, boolean showFilterCloseButton)
   {
      super(parent, style);

      this.view = view;

      PreferenceStore store = PreferenceStore.getInstance();
      objectsFullySync = store.getAsBoolean("ObjectBrowser.FullSync", false);

      session = Registry.getSession();
      refreshTimer = new RefreshTimer(session.getMinViewRefreshInterval(), this, new Runnable() {
         @Override
         public void run()
         {
            if (isDisposed() || objectTree.getControl().isDisposed())
               return;

            WidgetHelper.setRedraw(objectTree.getTree(), false);
            synchronized(updatedObjects)
            {
               if (fullRefresh)
               {
                  objectTree.refresh();
                  fullRefresh = false;
               }
               else
               {
                  objectTree.refreshObjects(updatedObjects.values());
               }
               updatedObjects.clear();
            }
            if (statusIndicatorEnabled)
               updateStatusIndicator();
            WidgetHelper.setRedraw(objectTree.getTree(), true);
         }
      });

      FormLayout formLayout = new FormLayout();
      setLayout(formLayout);

      // Create filter area
      final String tooltip = showFilterToolTip ? " > - Search by IP address part \n ^ - Search by exact IP address \n # - Search by ID \n / - Search by comment \n @ - Search by Zone ID" : null;
      if (view != null)
      {
         view.addFilterModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               onFilterModify();
            }
         });
         view.setFilterTooltip(tooltip);
      }
      else
      {
         filterText = new FilterText(this, SWT.NONE, tooltip, showFilterCloseButton);
         filterText.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               onFilterModify();
            }
         });
         filterText.setCloseAction(new Action() {
            @Override
            public void run()
            {
               enableFilter(false);
            }
         });
      }
      setupFilterText(true);

      // Create object tree control
      objectTree = new ObjectTreeViewer(this, SWT.VIRTUAL | (multiSelection ? SWT.MULTI : SWT.SINGLE), objectsFullySync);
      contentProvider = new ObjectTreeContentProvider(objectsFullySync, classFilter, objectFilter);
      objectTree.setContentProvider(contentProvider);
      objectTree.setLabelProvider(new DecoratingObjectLabelProvider((object) -> objectTree.update(object, null)));
      objectTree.setComparator(new ObjectTreeComparator());
      filter = new ObjectViewerFilter(null, classFilter);
      objectTree.addFilter(filter);
      objectTree.setInput(session);

      objectTree.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            TreeItem[] items = objectTree.getTree().getSelection();
            if (items.length == 1)
            {
               // Call open handlers. If open event processed by handler, openObject will return true
               if (!openObject((AbstractObject)items[0].getData()))
               {
                  objectTree.toggleItemExpandState(items[0]);
               }
            }
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = (filterText != null) ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      objectTree.getTree().setLayoutData(fd);

      if (filterText != null)
      {
         fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(0, 0);
         fd.right = new FormAttachment(100, 0);
         filterText.setLayoutData(fd);
      }

      // Add client library listener
      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_DELETED)
            {
               synchronized(updatedObjects)
               {
                  updatedObjects.clear();
                  fullRefresh = true;
               }
               refreshTimer.execute();
            }
            else if ((n.getCode() == SessionNotification.OBJECT_CHANGED) &&
                ((classFilter == null) || classFilter.contains(((AbstractObject)n.getObject()).getObjectClass())))
            {
               synchronized(updatedObjects)
               {
                  updatedObjects.put(n.getSubCode(), (AbstractObject)n.getObject());
               }
               refreshTimer.execute();
            }
         }
      };
      session.addListener(sessionListener);

      // Set dispose listener
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if ((session != null) && (sessionListener != null))
               session.removeListener(sessionListener);
         }
      });

      // Set initial focus to filter input line
      if (filterEnabled && (filterText != null))
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      enableStatusIndicator(statusIndicatorEnabled);
   }

   /**
    * Setup filter text control
    */
   private void setupFilterText(boolean addListener)
   {
      FilterText filterText = (view != null) ? view.getFilterTextControl() : this.filterText;
      if (filterText == null)
         return;

      PreferenceStore ps = PreferenceStore.getInstance();
      if (ps.getAsBoolean("ObjectBrowser.UseServerFilterSettings", true))
      {
         filterText.setAutoApply(session.getClientConfigurationHintAsBoolean("ObjectBrowser.AutoApplyFilter", true));
         filterText.setDelay(session.getClientConfigurationHintAsInt("ObjectBrowser.FilterDelay", 300));
         filterText.setMinLength(session.getClientConfigurationHintAsInt("ObjectBrowser.MinFilterStringLength", 1));
      }
      else
      {
         filterText.setAutoApply(ps.getAsBoolean("ObjectBrowser.AutoApplyFilter", true));
         filterText.setDelay(ps.getAsInteger("ObjectBrowser.FilterDelay", 300));
         filterText.setMinLength(ps.getAsInteger("ObjectBrowser.MinFilterStringLength", 1));
      }
      if (addListener)
      {
         ps.addPropertyChangeListener(new IPropertyChangeListener() {
            @Override
            public void propertyChange(PropertyChangeEvent event)
            {
               if (event.getProperty().equals("ObjectBrowser.UseServerFilterSettings")
                     || event.getProperty().equals("ObjectBrowser.AutoApplyFilter")
                     || event.getProperty().equals("ObjectBrowser.FilterDelay")
                     || event.getProperty().equals("ObjectBrowser.MinFilterStringLength"))
               {
                  setupFilterText(false);
               }
            }
         });
      }
   }

   /**
    * Get underlying tree control
    * 
    * @return Underlying tree control
    */
   public Tree getTreeControl()
   {
      return objectTree.getTree();
   }

   /**
    * Get underlying tree viewer
    *
    * @return Underlying tree viewer
    */
   public TreeViewer getTreeViewer()
   {
      return objectTree;
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      if (filterText == null)
         return; // External filter

      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)objectTree.getTree().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      if (statusIndicatorEnabled)
      {
         fd = (FormData)statusIndicator.getLayoutData();
         fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      }
      layout();
      if (enable)
         filterText.setFocus();
      else
         setFilterText("");
   }

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled()
   {
      return filterEnabled;
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilterText(final String text)
   {
      if (view != null)
         view.setFilterText(text);
      else
         filterText.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return (view != null) ? view.getFilterText() : filterText.getText();
   }

   /**
    * Get selected object
    * 
    * @return ID of selected object or 0 if no objects selected
    */
   public long getFirstSelectedObject()
   {
      IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
      if (selection.isEmpty())
         return 0;
      return ((AbstractObject)selection.getFirstElement()).getObjectId();
   }

   /**
    * Get all selected objects
    * 
    * @return ID of selected object or 0 if no objects selected
    */
   public Long[] getSelectedObjects()
   {
      IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
      Set<Long> objects = new HashSet<Long>(selection.size());
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         objects.add(((AbstractObject)it.next()).getObjectId());
      }
      return objects.toArray(new Long[objects.size()]);
   }

   /**
    * Get selected object as object
    * 
    * @return
    */
   public AbstractObject getFirstSelectedObject2()
   {
      IStructuredSelection selection = (IStructuredSelection)objectTree.getSelection();
      return (AbstractObject)selection.getFirstElement();
   }

   /**
    * Refresh object tree
    */
   public void refresh()
   {
      objectTree.refresh();
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
    */
   @Override
   public void setEnabled(boolean enabled)
   {
      super.setEnabled(enabled);
      objectTree.getControl().setEnabled(enabled);
      if (filterText != null)
         filterText.setEnabled(enabled);
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = getFilterText();
      filter.setFilterString(text);
      AbstractObject obj = filter.getLastMatch();
      if (obj != null)
      {
         AbstractObject parent = getParent(obj);
         if (parent != null)
            objectTree.expandToLevel(parent, 1);
         objectTree.setSelection(new StructuredSelection(obj), true);
         objectTree.reveal(obj);
         if (statusIndicatorEnabled)
            updateStatusIndicator();
      }
      objectTree.refresh(false);
   }

   /**
    * Get priority of given parent object
    *
    * @param object parent object
    * @return priority (0 or higher)
    */
   static int getParentPriority(AbstractObject object)
   {
      if ((object instanceof ServiceRoot) || (object instanceof EntireNetwork))
         return 3;
      if ((object instanceof Circuit) || (object instanceof Collector) || (object instanceof Container) || (object instanceof Cluster) || (object instanceof Chassis) || (object instanceof Rack))
         return 2;
      if ((object instanceof Zone) || (object instanceof Subnet))
         return 1;
      return 0;
   }

   /**
    * Get parent for given object prioritizing different sub-trees.
    */
   private AbstractObject getParent(final AbstractObject childObject)
   {
      AbstractObject[] parents = childObject.getParentsAsArray();
      AbstractObject parent = null;
      int parentPriority = -1;
      for(AbstractObject p : parents)
      {
         if (filter.select(objectTree, null, p))
         {   
            int pp = getParentPriority(p);
            if (pp > parentPriority)
            {
               parentPriority = pp;
               parent = p;
            }
         }
      }
      return parent;
   }

   /**
    * @return true if unmanaged objects are hidden
    */
   public boolean isHideUnmanaged()
   {
      return filter.isHideUnmanaged();
   }

   /**
    * Show/hide unmanaged objects
    * 
    * @param hide true to hide unmanaged objects
    */
   public void setHideUnmanaged(boolean hide)
   {
      filter.setHideUnmanaged(hide);
      onFilterModify();
   }

   /**
    * @return true if template checks are hidden
    */
   public boolean isHideTemplateChecks()
   {
      return filter.isHideTemplateChecks();
   }

   /**
    * Show/hide service check template objects
    * 
    * @param hide true to hide service check template objects
    */
   public void setHideTemplateChecks(boolean hide)
   {
      filter.setHideTemplateChecks(hide);
      onFilterModify();
   }

   /**
    * @return true if sub-interfaces are hidden
    */
   public boolean isHideSubInterfaces()
   {
      return filter.isHideSubInterfaces();
   }

   /**
    * Show/hide sub-interfaces
    * 
    * @param hide true to hide sub-interfaces
    */
   public void setHideSubInterfaces(boolean hide)
   {
      filter.setHideSubInterfaces(hide);
      onFilterModify();
   }

   /**
    * Set action to be executed when user press "Close" button in object filter. Default implementation will hide filter area
    * without notifying parent.
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action)
   {
      if (filterText != null)
         filterText.setCloseAction(action);
   }

   /**
    * @param enabled
    */
   public void enableStatusIndicator(boolean enabled)
   {
      if (statusIndicatorEnabled == enabled)
         return;

      statusIndicatorEnabled = enabled;
      if (enabled)
      {
         statusIndicator = new ObjectStatusIndicator(this, SWT.NONE);
         FormData fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = (filterEnabled && (filterText != null)) ? new FormAttachment(filterText) : new FormAttachment(0, 0);
         fd.bottom = new FormAttachment(100, 0);
         statusIndicator.setLayoutData(fd);

         fd = (FormData)objectTree.getTree().getLayoutData();
         fd.left = new FormAttachment(statusIndicator);

         statusIndicatorSelectionListener = new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               updateStatusIndicator();
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               updateStatusIndicator();
            }
         };
         objectTree.getTree().getVerticalBar().addSelectionListener(statusIndicatorSelectionListener);

         statusIndicatorTreeListener = new TreeListener() {
            @Override
            public void treeCollapsed(TreeEvent e)
            {
               updateStatusIndicator();
            }

            @Override
            public void treeExpanded(TreeEvent e)
            {
               updateStatusIndicator();
            }
         };
         objectTree.getTree().addTreeListener(statusIndicatorTreeListener);

         updateStatusIndicator();
      }
      else
      {
         objectTree.getTree().getVerticalBar().removeSelectionListener(statusIndicatorSelectionListener);
         objectTree.getTree().removeTreeListener(statusIndicatorTreeListener);
         statusIndicator.dispose();
         statusIndicator = null;
         statusIndicatorSelectionListener = null;
         statusIndicatorTreeListener = null;

         FormData fd = (FormData)objectTree.getTree().getLayoutData();
         fd.left = new FormAttachment(0, 0);
      }
      layout(true, true);
   }

   /**
    * @return
    */
   public boolean isStatusIndicatorEnabled()
   {
      return statusIndicatorEnabled;
   }

   /**
    * Update status indicator
    */
   private void updateStatusIndicator()
   {
      statusIndicator.refresh(objectTree);
   }

   /**
    * @param listener
    */
   public void addOpenListener(ObjectOpenListener listener)
   {
      openListeners.add(listener);
   }

   /**
    * @param listener
    */
   public void removeOpenListener(ObjectOpenListener listener)
   {
      openListeners.remove(listener);
   }

   /**
    * Open selected object
    * 
    * @param object
    * @return
    */
   private boolean openObject(AbstractObject object)
   {
      for(ObjectOpenListener l : openListeners)
         if (l.openObject(object))
            return true;
      return false;
   }

   /**
    * Enable drag support in object tree
    */
   public void enableDragSupport()
   {
      Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      objectTree.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new DragSourceAdapter() {
         @Override
         public void dragStart(DragSourceEvent event)
         {
            LocalSelectionTransfer.getTransfer().setSelection(objectTree.getSelection());
            event.doit = true;
         }

         @Override
         public void dragSetData(DragSourceEvent event)
         {
            event.data = LocalSelectionTransfer.getTransfer().getSelection();
         }
      });
   }

   /**
    * Enable drop support in object tree
    */
   public void enableDropSupport(final ObjectBrowser objectBrowserView)
   {
      final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      objectTree.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(objectTree) {
         int operation = 0;

         @Override
         public boolean performDrop(Object data)
         {
            TreeSelection selection = (TreeSelection)data;
            List<?> movableSelection = selection.toList();
            for(int i = 0; i < movableSelection.size(); i++)
            {
               AbstractObject movableObject = (AbstractObject)movableSelection.get(i);
               TreePath path = selection.getPaths()[0];
               AbstractObject parent = (AbstractObject)path.getSegment(path.getSegmentCount() - 2);
               objectBrowserView.moveObject((AbstractObject)getCurrentTarget(), parent, movableObject, operation == DND.DROP_MOVE);
            }
            return true;
         }

         @Override
         public boolean validateDrop(Object target, int operation, TransferData transferType)
         {
            this.operation = operation;
            if ((target == null) || !LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
               return false;

            IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
            if (selection.isEmpty())
               return false;

            for(final Object object : selection.toList())
            {
               SubtreeType subtree = null;
               if ((object instanceof AbstractObject))
               {
                  if (objectBrowserView.isValidSelectionForMove(SubtreeType.INFRASTRUCTURE, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.INFRASTRUCTURE;
                  else if (objectBrowserView.isValidSelectionForMove(SubtreeType.TEMPLATES, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.TEMPLATES;
                  else if (objectBrowserView.isValidSelectionForMove(SubtreeType.BUSINESS_SERVICES, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.BUSINESS_SERVICES;
                  else if (objectBrowserView.isValidSelectionForMove(SubtreeType.DASHBOARDS, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.DASHBOARDS;
                  else if (objectBrowserView.isValidSelectionForMove(SubtreeType.MAPS, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.MAPS;
                  else if (objectBrowserView.isValidSelectionForMove(SubtreeType.ASSETS, operation == DND.DROP_MOVE))
                     subtree = SubtreeType.ASSETS;
               }

               if (subtree == null)
                  return false;

               if ((operation == DND.DROP_COPY) && (subtree != SubtreeType.INFRASTRUCTURE))
                  return false;

               Set<Integer> filter;
               switch(subtree)
               {
                  case INFRASTRUCTURE:
                     if (object instanceof Interface)
                        filter = Set.of(Integer.valueOf(AbstractObject.OBJECT_CIRCUIT));
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
                     if (object instanceof DashboardGroup || object instanceof DashboardTemplate)
                        filter = ObjectSelectionDialog.createDashboardGroupSelectionFilter();
                     else
                        filter = ObjectSelectionDialog.createDashboardSelectionFilter();
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

               if ((filter == null) || !filter.contains(((AbstractObject)target).getObjectClass()) || target.equals(object))
                  return false;
            }
            return true;
         }
      });
   }

   /**
    * Disables refresh option in RefreshTimer class. Is used for object name edit.
    */
   public void disableRefresh()
   {
      refreshTimer.disableRefresh();
   }

   /**
    * Enables refresh option in RefreshTimer class. Is used for object name edit.
    */
   public void enableRefresh()
   {
      refreshTimer.enableRefresh();
   }

   /**
    * Get selection provider for this object tree
    *
    * @return selection provider
    */
   public ISelectionProvider getSelectionProvider()
   {
      return objectTree;
   }
   
   /**
    * Select object 
    * 
    * @param objectId id of the object to be selected
    */
   public void selectObject(AbstractObject object)
   {
      AbstractObject parent = getParent(object);
      if (parent != null)
      {
         List<AbstractObject> chain = new ArrayList<>(16);
         while(parent != null)
         {
            chain.add(parent);
            parent = getParent(parent);
         }
         for(int i = chain.size() - 1; i >= 0; i--)
            objectTree.expandToLevel(chain.get(i), 1);
      }
      objectTree.setSelection(new StructuredSelection(object), true);
      objectTree.reveal(object);
   }
}
