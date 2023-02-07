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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.helpers.MenuContributionItem;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.PackageDeployment;
import org.netxms.nxmc.modules.agentmanagement.dialogs.SelectDeployPackage;
import org.netxms.nxmc.modules.agentmanagement.views.AgentConfigurationEditor;
import org.netxms.nxmc.modules.agentmanagement.views.PackageDeploymentMonitor;
import org.netxms.nxmc.modules.nxsl.views.ScriptExecutorView;
import org.netxms.nxmc.modules.objects.dialogs.MaintanenceScheduleDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.ScreenshotView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectContextMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectContextMenuManager.class);

   private View view;
   private ISelectionProvider selectionProvider;
   private ColumnViewer objectViewer;
   private Action actionManage;
   private Action actionUnmanage;
   private Action actionRename;
   private Action actionDelete;
   private Action actionDeployPackage;
   private Action actionEnterMaintenance;
   private Action actionLeaveMaintenance;
   private Action actionScheduleMaintenance;
   private Action actionProperties;
   private Action actionTakeScreenshot;
   private Action actionEditAgentConfig;
   private Action actionExecuteScript;
   private Action actionBind;
   private Action actionUnbind;
   private Action actionBindTo;
   private Action actionUnbindFrom;
   private Action actionApply;
   private Action actionRemove;
   private Action actionAddNode;
   private Action actionRemoveNode;

   /**
    * Create new object context menu manager.
    *
    * @param view owning view
    * @param selectionProvider selection provider
    */
   public ObjectContextMenuManager(View view, ISelectionProvider selectionProvider, ColumnViewer objectViewer)
   {
      this.view = view;
      this.selectionProvider = selectionProvider;
      this.objectViewer = objectViewer;
      setRemoveAllWhenShown(true);
      addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu();
         }
      });
      createActions();
   }

   /**
    * Create object actions
    */
   private void createActions()
   {
      actionManage = new Action(i18n.tr("&Manage")) {
         @Override
         public void run()
         {
            changeObjectManagementState(true);
         }
      };

      actionUnmanage = new Action(i18n.tr("&Unmanage")) {
         @Override
         public void run()
         {
            changeObjectManagementState(false);
         }
      };
      
      actionDeployPackage = new Action(i18n.tr("D&eploy package...")) {
         @Override
         public void run()
         {
            deployPackage();
         }
      };

      if (objectViewer != null)
      {
         actionRename = new Action(i18n.tr("Rename")) {
            @Override
            public void run()
            {
               Object element = null;
               Control control = objectViewer.getControl();
               if (control instanceof Tree)
               {
                  TreeItem[] selection = ((Tree)control).getSelection();
                  if (selection.length != 1)
                     return;
                  element = selection[0].getData();
               }
               else if (control instanceof Table)
               {
                  TableItem[] selection = ((Table)control).getSelection();
                  if (selection.length != 1)
                     return;
                  element = selection[0].getData();
               }
               if (element != null)
               {
                  objectViewer.editElement(element, 0);
               }
            }
         };
         view.addKeyBinding("F2", actionRename);
      }

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteObject();
         }
      };

      actionEnterMaintenance = new Action(i18n.tr("&Enter maintenance mode...")) {
         @Override
         public void run()
         {
            changeObjectMaintenanceState(true);
         }
      };

      actionLeaveMaintenance = new Action(i18n.tr("&Leave maintenance mode")) {
         @Override
         public void run()
         {
            changeObjectMaintenanceState(false);
         }
      };

      actionScheduleMaintenance = new Action(i18n.tr("&Schedule maintenance...")) {
         @Override
         public void run()
         {
            scheduleMaintenance();
         }
      };

      actionProperties = new Action(i18n.tr("&Properties..."), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(getObjectFromSelection(), getShell());
         }
      };

      actionTakeScreenshot = new Action(i18n.tr("&Take screenshot"), ResourceManager.getImageDescriptor("icons/screenshot.png")) {
         @Override
         public void run()
         {
            openScreenshotView();
         }
      };

      actionEditAgentConfig = new Action(i18n.tr("Edit agent configuration"), ResourceManager.getImageDescriptor("icons/object-views/agent-config.png")) {
         @Override
         public void run()
         {
            openAgentConfigEditor();
         }
      };

      actionExecuteScript = new Action(i18n.tr("E&xecute script"), ResourceManager.getImageDescriptor("icons/object-views/script-executor.png")) {
         @Override
         public void run()
         {
            executeScript();
         }
      };

      actionBind = new Action(i18n.tr("&Bind...")) {
         @Override
         public void run()
         {
            bindObjects();
         }
      };

      actionUnbind = new Action(i18n.tr("U&nbind...")) {
         @Override
         public void run()
         {
            unbindObjects();
         }
      };

      actionBindTo = new Action(i18n.tr("&Bind to...")) {
         @Override
         public void run()
         {
            bindToObject();
         }
      };

      actionUnbindFrom = new Action(i18n.tr("U&nbind from...")) {
         @Override
         public void run()
         {
            unbindFromObjects();
         }
      };

      actionApply = new Action(i18n.tr("&Apply")) {
         @Override
         public void run()
         {
            bindObjects();
         }
      };

      actionRemove = new Action(i18n.tr("&Remove...")) {
         @Override
         public void run()
         {
            unbindObjects();
         }
      };

      actionAddNode = new Action(i18n.tr("&Add node...")) {
         @Override
         public void run()
         {
            addNodeToCluster();
         }
      };

      actionRemoveNode = new Action(i18n.tr("&Remove node...")) {
         @Override
         public void run()
         {
            removeNodeFromCluster();
         }
      };
   }

   /**
    * Fill object context menu
    */
   protected void fillContextMenu()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.isEmpty())
         return;

      boolean singleObject = (selection.size() == 1);

      if (singleObject)
      {
         AbstractObject object = getObjectFromSelection();
         MenuManager createMenu = new ObjectCreateMenuManager(getShell(), view, object);
         if (!createMenu.isEmpty())
         {
            add(createMenu);
            add(new Separator());
         }
         if ((object instanceof Container) || (object instanceof ServiceRoot))
         {
            add(actionBind);
            add(actionUnbind);
            add(new Separator());
         }
         if (object instanceof Template)
         {
            add(actionApply);
            add(actionRemove);
            add(new Separator());
         }
         if (object instanceof Cluster)
         {
            add(actionAddNode);
            add(actionRemoveNode);
            add(new Separator());
         }
      }
      if (isBindToMenuAllowed(selection))
      {
         add(actionBindTo);
         if (singleObject)
            add(actionUnbindFrom);
         add(new Separator());
      }
      if (isMaintenanceMenuAllowed(selection))
      {
         MenuManager maintenanceMenu = new MenuManager(i18n.tr("M&aintenance"));
         maintenanceMenu.add(actionEnterMaintenance);
         maintenanceMenu.add(actionLeaveMaintenance);
         maintenanceMenu.add(actionScheduleMaintenance);
         add(maintenanceMenu);
      }
      add(actionManage);
      add(actionUnmanage);
      if (singleObject && (objectViewer != null))
      {
         add(actionRename);
      }
      add(actionDelete);
      add(new Separator());

      // Agent/package management
      if (singleObject)
      {
         AbstractObject object = getObjectFromSelection();
         if ((object instanceof Node) && ((Node)object).hasAgent())
         {
            add(actionEditAgentConfig);
            add(actionDeployPackage);
         }
      }
      else
      {
         boolean nodesWithAgent = false;
         for(Object o : selection.toList())
         {
            if (o instanceof Node)
            {
               if (((Node)o).hasAgent())
               {
                  nodesWithAgent = true;
                  break;
               }
            }
            else
            {
               for(AbstractObject n : ((AbstractObject)o).getAllChildren(AbstractObject.OBJECT_NODE))
               {
                  if (((Node)n).hasAgent())
                  {
                     nodesWithAgent = true;
                     break;
                  }
               }
               if (nodesWithAgent)
                  break;
            }
         }
         if (nodesWithAgent)
            add(actionDeployPackage);
      }

      // Screenshots, etc. for single node
      if (singleObject)
      {
         AbstractObject object = getObjectFromSelection();
         if ((object instanceof Node) && ((Node)object).hasAgent() && ((Node)object).getPlatformName().startsWith("windows-"))
         {
            add(new Separator());
            add(actionTakeScreenshot);
         }
         add(actionExecuteScript);
      }

      final Menu toolsMenu = ObjectMenuFactory.createToolsMenu(selection, getMenu(), null, new ViewPlacement(view));
      if (toolsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("&Tools"), toolsMenu));
      }

      final Menu pollsMenu = ObjectMenuFactory.createPollMenu(selection, getMenu(), null, new ViewPlacement(view));
      if (pollsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("P&oll"), pollsMenu));
      }

      final Menu graphTemplatesMenu = ObjectMenuFactory.createGraphTemplatesMenu(selection, getMenu(), null, new ViewPlacement(view));
      if (graphTemplatesMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("&Graphs"), graphTemplatesMenu));
      }

      final Menu summaryTableMenu = ObjectMenuFactory.createSummaryTableMenu(selection, getMenu(), null, new ViewPlacement(view));
      if (summaryTableMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("S&ummary tables"), summaryTableMenu));
      }

      if (singleObject)
      {
         add(new Separator());
         add(actionProperties);
      }
   }

   /**
    * Check if maintenance menu is allowed.
    *
    * @param selection current object selection
    * @return true if maintenance menu is allowed
    */
   private static boolean isMaintenanceMenuAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof AbstractObject))
            return false;
         int objectClass = ((AbstractObject)o).getObjectClass();
         if ((objectClass == AbstractObject.OBJECT_BUSINESSSERVICE) || (objectClass == AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE) || (objectClass == AbstractObject.OBJECT_BUSINESSSERVICEROOT) ||
             (objectClass == AbstractObject.OBJECT_DASHBOARD) || (objectClass == AbstractObject.OBJECT_DASHBOARDGROUP) || (objectClass == AbstractObject.OBJECT_DASHBOARDROOT) ||
             (objectClass == AbstractObject.OBJECT_NETWORKMAP) || (objectClass == AbstractObject.OBJECT_NETWORKMAPGROUP) || (objectClass == AbstractObject.OBJECT_NETWORKMAPROOT) ||
             (objectClass == AbstractObject.OBJECT_TEMPLATE) || (objectClass == AbstractObject.OBJECT_TEMPLATEGROUP) || (objectClass == AbstractObject.OBJECT_TEMPLATEROOT))
            return false;
      }
      return true;
   }

   /**
    * Check if "bind to" / "unbind from" menu is allowed.
    *
    * @param selection current object selection
    * @return true if "bind to" / "unbind from" menu is allowed
    */
   private static boolean isBindToMenuAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof AbstractObject))
            return false;
         int objectClass = ((AbstractObject)o).getObjectClass();
         if ((objectClass != AbstractObject.OBJECT_CHASSIS) && (objectClass != AbstractObject.OBJECT_CLUSTER) && (objectClass != AbstractObject.OBJECT_MOBILEDEVICE) &&
             (objectClass != AbstractObject.OBJECT_NODE) && (objectClass != AbstractObject.OBJECT_RACK) && (objectClass != AbstractObject.OBJECT_SENSOR) &&
             (objectClass != AbstractObject.OBJECT_SUBNET))
            return false;
      }
      return true;
   }

   /**
    * Get parent shell for dialog windows.
    *
    * @return parent shell for dialog windows
    */
   protected Shell getShell()
   {
      return Registry.getMainWindow().getShell();
   }

   /**
    * Get object from current selection
    *
    * @return object or null
    */
   protected AbstractObject getObjectFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return null;
      return (AbstractObject)selection.getFirstElement();
   }

   /**
    * Get object ID from selection
    *
    * @return object ID or 0
    */
   protected long getObjectIdFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return 0;
      return ((AbstractObject)selection.getFirstElement()).getObjectId();
   }

   /**
    * Change management status for selected objects
    *
    * @param managed true to manage objects
    */
   private void changeObjectManagementState(final boolean managed)
   {
      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Change object management status"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
                  session.setObjectManaged(((AbstractObject)o).getObjectId(), managed);
               else if (o instanceof ObjectWrapper)
                  session.setObjectManaged(((ObjectWrapper)o).getObjectId(), managed);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change object management status");
         }
      }.start();
   }

   /**
    * Change maintenance state for selected objects
    *
    * @param enter true to enter maintenance
    */
   private void changeObjectMaintenanceState(final boolean enter)
   {
      final String comments;
      if (enter)
      {
         InputDialog dlg = new InputDialog(null, i18n.tr("Enter Maintenance"), i18n.tr("Additional comments"), "", null);
         if (dlg.open() != Window.OK)
            return;
         comments = dlg.getValue().trim();
      }
      else
      {
         comments = null;
      }

      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Change object maintenance state"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
                  session.setObjectMaintenanceMode(((AbstractObject)o).getObjectId(), enter, comments);
               else if (o instanceof ObjectWrapper)
                  session.setObjectMaintenanceMode(((ObjectWrapper)o).getObjectId(), enter, comments);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change object maintenance state");
         }
      }.start();
   }

   /**
    * Schedule maintenance
    */
   private void scheduleMaintenance()
   {
      final MaintanenceScheduleDialog dialog = new MaintanenceScheduleDialog(view.getWindow().getShell());
      if (dialog.open() != Window.OK)
         return;

      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Schedule maintenance"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
               {
                  ScheduledTask taskStart = new ScheduledTask("Maintenance.Enter", "", "", dialog.getComments(), dialog.getStartTime(), ScheduledTask.SYSTEM, ((AbstractObject)o).getObjectId());
                  ScheduledTask taskEnd = new ScheduledTask("Maintenance.Leave", "", "", dialog.getComments(), dialog.getEndTime(), ScheduledTask.SYSTEM, ((AbstractObject)o).getObjectId());
                  session.addScheduledTask(taskStart);
                  session.addScheduledTask(taskEnd);
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot schedule maintenance");
         }
      }.start();
   }
   
   /**
    * Deploy package on node
    */
   private void deployPackage()
   {
      final SelectDeployPackage dialog = new SelectDeployPackage(view.getWindow().getShell());
      if (dialog.open() != Window.OK)
         return;
      
      final Object[] objectList = ((IStructuredSelection)selectionProvider.getSelection()).toArray();      
      final Set<Long> objects = new HashSet<Long>();
      for(Object o : objectList)
      {
         if (o instanceof AbstractObject)
         {
            objects.add(((AbstractObject)o).getObjectId());
         }
      }

      PackageDeploymentMonitor monitor = new PackageDeploymentMonitor();  
      monitor.setPackageId(dialog.getSelectedPackageId());
      monitor.setApplicableObjects(objects);       
      PackageDeployment deployment = new PackageDeployment(monitor);
      monitor.setPackageDeploymentListener(deployment);
      Perspective p = view.getPerspective();   
      if (p != null)
      {
         p.addMainView(monitor, true, false);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(monitor);
         window.open();
      }
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Deploy agent package"), monitor) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deployPackage(dialog.getSelectedPackageId(), objects.toArray(new Long[objects.size()]), deployment);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start package deployment");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Delete selected objects
    */
   private void deleteObject()
   {
      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();  
      String question = (objects.length == 1) ?
         String.format(i18n.tr("Are you sure you want to delete \"%s\"?"), ((AbstractObject)objects[0]).getObjectName()) :
         String.format(i18n.tr("Are you sure you want to delete %d objects?"), objects.length);
      if (!MessageDialogHelper.openConfirm(view.getWindow().getShell(), i18n.tr("Confirm Delete"), question))
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Delete objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
               session.deleteObject(((AbstractObject)objects[i]).getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete object");
         }
      }.start();
   }

   /**
    * Open screenshot view
    */
   private void openScreenshotView()
   {
      AbstractObject object = getObjectFromSelection();
      if (!(object instanceof Node))
         return;

      ScreenshotView screenshotView = new ScreenshotView((Node)object, null, null);
      if (view.getPerspective() != null)
      {
         view.getPerspective().addMainView(screenshotView, true, false);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(screenshotView);
         window.open();
      }
   }

   /**
    * Open agent configuration editor
    */
   private void openAgentConfigEditor()
   {
      AbstractObject object = getObjectFromSelection();
      if (!(object instanceof Node))
         return;

      AgentConfigurationEditor editor = new AgentConfigurationEditor((Node)object);
      if (view.getPerspective() != null)
      {
         view.getPerspective().addMainView(editor, true, false);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(editor);
         window.open();
      }
   }

   /**
    * Execute script on object
    */
   private void executeScript()
   {
      AbstractObject object = getObjectFromSelection();
      ScriptExecutorView executor = new ScriptExecutorView(object.getObjectId());
      if (view.getPerspective() != null)
      {
         view.getPerspective().addMainView(executor, true, false);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(executor);
         window.open();
      }
   }

   /**
    * Bind objects to selected object
    */
   private void bindObjects()
   {
      final long parentId = getObjectIdFromSelection();
      if (parentId == 0)
         return;

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createDataCollectionTargetSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Binding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.bindObject(parentId, o.getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot bind objects");
         }
      }.start();
   }

   /**
    * Unbind objects from selected object
    */
   private void unbindObjects()
   {
      final long parentId = getObjectIdFromSelection();
      if (parentId == 0)
         return;

      final RelatedObjectSelectionDialog dlg = new RelatedObjectSelectionDialog(view.getWindow().getShell(), parentId, RelatedObjectSelectionDialog.RelationType.DIRECT_SUBORDINATES, null);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Unbinding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.unbindObject(parentId, o.getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unbind objects");
         }
      }.start();
   }

   /**
    * Bind selected object to another object
    */
   private void bindToObject()
   {
      final List<Long> childIdList = new ArrayList<>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {
         if ((o instanceof DataCollectionTarget) || (o instanceof Rack) || (o instanceof Chassis) || (o instanceof Subnet))
            childIdList.add(((AbstractObject)o).getObjectId());
      }

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createContainerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Binding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> parents = dlg.getSelectedObjects();
            for(AbstractObject o : parents)
            {
               for(Long childId : childIdList)
                  session.bindObject(o.getObjectId(), childId);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot bind objects");
         }
      }.start();
   }

   /**
    * Unbind selected objects from one or more containers
    */
   private void unbindFromObjects()
   {
      final long childId = getObjectIdFromSelection();
      if (childId == 0)
         return;

      final RelatedObjectSelectionDialog dlg = new RelatedObjectSelectionDialog(view.getWindow().getShell(), childId, RelatedObjectSelectionDialog.RelationType.DIRECT_SUPERORDINATES,
            ObjectSelectionDialog.createContainerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Unbinding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.unbindObject(o.getObjectId(), childId);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unbind objects");
         }
      }.start();
   }

   /**
    * Add node(s) to cluster
    */
   private void addNodeToCluster()
   {
      final long clusterId = getObjectIdFromSelection();
      if (clusterId == 0)
         return;

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();      
      new Job(i18n.tr("Add node to cluster"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
            {
               try 
               {
                  session.addClusterNode(clusterId, o.getObjectId());                     
               }
               catch (NXCException exception)
               {
                  if(exception.getErrorCode() != RCC.CLUSTER_MEMBER_ALREADY)
                  {
                     throw(exception);
                  }
               }              
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add node to cluster");
         }
      }.start();
   }

   /**
    * Remove node(s) from cluster
    */
   private void removeNodeFromCluster()
   {
      final long clusterId = getObjectIdFromSelection();
      if (clusterId == 0)
         return;

      final RelatedObjectSelectionDialog dlg = new RelatedObjectSelectionDialog(view.getWindow().getShell(), clusterId, RelatedObjectSelectionDialog.RelationType.DIRECT_SUBORDINATES,
            RelatedObjectSelectionDialog.createClassFilter(AbstractObject.OBJECT_NODE));
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Remove cluster node"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(int i = 0; i < objects.size(); i++)
               session.removeClusterNode(clusterId, objects.get(i).getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot remove node from cluster");
         }
      }.start();
   }
}
