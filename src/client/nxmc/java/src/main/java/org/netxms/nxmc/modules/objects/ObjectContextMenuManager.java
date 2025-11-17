/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import java.util.ServiceLoader;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DashboardBase;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.helpers.MenuContributionItem;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.SendUserAgentNotificationAction;
import org.netxms.nxmc.modules.agentmanagement.dialogs.PackageSelectionDialog;
import org.netxms.nxmc.modules.agentmanagement.views.AgentConfigurationEditor;
import org.netxms.nxmc.modules.assetmanagement.LinkAssetToObjectAction;
import org.netxms.nxmc.modules.assetmanagement.LinkObjectToAssetAction;
import org.netxms.nxmc.modules.assetmanagement.UnlinkAssetFromObjectAction;
import org.netxms.nxmc.modules.assetmanagement.UnlinkObjectFromAssetAction;
import org.netxms.nxmc.modules.dashboards.AssignDashboardAction;
import org.netxms.nxmc.modules.dashboards.CloneAsDashboardAction;
import org.netxms.nxmc.modules.dashboards.CloneAsDashboardTemplateAction;
import org.netxms.nxmc.modules.dashboards.ExportDashboardAction;
import org.netxms.nxmc.modules.dashboards.ImportDashboardAction;
import org.netxms.nxmc.modules.filemanager.UploadFileToAgent;
import org.netxms.nxmc.modules.networkmaps.views.IPTopologyMapView;
import org.netxms.nxmc.modules.networkmaps.views.InternalTopologyMapView;
import org.netxms.nxmc.modules.networkmaps.views.L2TopologyMapView;
import org.netxms.nxmc.modules.nxsl.views.ScriptExecutorView;
import org.netxms.nxmc.modules.objects.actions.ChangeInterfaceExpectedStateAction;
import org.netxms.nxmc.modules.objects.actions.ChangeMacAddressAction;
import org.netxms.nxmc.modules.objects.actions.ChangeZoneAction;
import org.netxms.nxmc.modules.objects.actions.ClearInterfacePeerInformation;
import org.netxms.nxmc.modules.objects.actions.CloneNetworkMap;
import org.netxms.nxmc.modules.objects.actions.CreateInterfaceDciAction;
import org.netxms.nxmc.modules.objects.actions.EnableNetworkMapPublicAccess;
import org.netxms.nxmc.modules.objects.actions.ForcedPolicyDeploymentAction;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.actions.SetInterfacePeerInformation;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog.RelationType;
import org.netxms.nxmc.modules.objects.dialogs.RelatedTemplateObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.RemoteControlView;
import org.netxms.nxmc.modules.objects.views.RouteView;
import org.netxms.nxmc.modules.objects.views.ScreenshotView;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.services.ObjectActionDescriptor;
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
   private Action actionProperties;
   private Action actionTakeScreenshot;
   private Action actionOpenRemoteControlView;
   private Action actionEditAgentConfig;
   private Action actionExecuteScript;
   private Action actionOpenMibExprorer;   
   private Action actionBind;
   private Action actionUnbind;
   private Action actionBindTo;
   private Action actionUnbindFrom;
   private Action actionAddToCircuit;
   private Action actionRemoveFromCircuit;
   private Action actionApplyTemplate;
   private Action actionRemoveTemplate;
   private Action actionApplyNodeTemplate;
   private Action actionRemoveObjectsTemplate;
   private Action actionAddClusterNode;
   private Action actionRemoveClusterNode;
   private Action actionAddWirelessController;
   private Action actionRemoveWirelessController;
   private Action actionRouteFrom;
   private Action actionRouteTo;
   private Action actionRouteFromMgmtNode;
   private Action actionLayer2Topology;
   private Action actionIPTopology;
   private Action actionInternalTopology;
   private ObjectAction<?> actionCreateInterfaceDCI;
   private ObjectAction<?> actionForcePolicyInstall;
   private ObjectAction<?> actionLinkAssetToObject;
   private ObjectAction<?> actionUnlinkAssetFromObject;
   private ObjectAction<?> actionLinkObjectToAsset;
   private ObjectAction<?> actionUnlinkObjectFromAsset;
   private ObjectAction<?> actionCloneAsDashboard;
   private ObjectAction<?> actionCloneAsDashboardTemplate;
   private ObjectAction<?> actionChangeZone;
   private ObjectAction<?> actionSendUserAgentNotification;
   private ObjectAction<?> actionExportDashboard;
   private ObjectAction<?> actionImportDashboard;
   private ObjectAction<?> actionAssignDashboard;
   private ObjectAction<?> actionCloneNetworkMap;
   private ObjectAction<?> actionNetworkMapPublicAccess;
   private ObjectAction<?> actionChangeInterfaceExpectedState;
   private ObjectAction<?> actionChangeMacAddress;
   private ObjectAction<?> actionUploadFileToAgent;
   private ObjectAction<?> actionSetInterfacePeer;
   private ObjectAction<?> actionClearInterfacePeer;
   private List<ObjectAction<?>> actionContributions = new ArrayList<>();

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
      addMenuListener((m) -> fillContextMenu());
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

      actionProperties = new Action(i18n.tr("&Properties..."), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(getObjectFromSelection(), getShell(), view);
         }
      };

      actionTakeScreenshot = new Action(i18n.tr("&Take screenshot"), ResourceManager.getImageDescriptor("icons/screenshot.png")) {
         @Override
         public void run()
         {
            openScreenshotView();
         }
      };

      actionOpenRemoteControlView = new Action(i18n.tr("&Remote control"), ResourceManager.getImageDescriptor("icons/object-views/remote-desktop.png")) {
         @Override
         public void run()
         {
            openRemoteControlView();
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

      actionOpenMibExprorer = new Action(i18n.tr("&MIB Explorer"), ResourceManager.getImageDescriptor("icons/object-views/mibexplorer.gif")) {
         @Override
         public void run()
         {
            openMibExplorer();
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

      actionBindTo = new Action(i18n.tr("&Bind to container...")) {
         @Override
         public void run()
         {
            bindToContainer();
         }
      };

      actionUnbindFrom = new Action(i18n.tr("U&nbind from container...")) {
         @Override
         public void run()
         {
            unbindFromParents();
         }
      };

      actionAddToCircuit = new Action(i18n.tr("&Add to circuit...")) {
         @Override
         public void run()
         {
            addToCircuit();
         }
      };

      actionRemoveFromCircuit = new Action(i18n.tr("&Remove from circuit...")) {
         @Override
         public void run()
         {
            unbindFromParents();
         }
      };

      actionApplyTemplate = new Action(i18n.tr("&Apply to...")) {
         @Override
         public void run()
         {
            bindObjects();
         }
      };

      actionRemoveTemplate = new Action(i18n.tr("&Remove from...")) {
         @Override
         public void run()
         {
            removeTemplate();
         }
      };

      actionApplyNodeTemplate = new Action(i18n.tr("&Apply template...")) {
         @Override
         public void run()
         {
            selectAndApplyTemplate();
         }
      };

      actionRemoveObjectsTemplate = new Action(i18n.tr("&Remove template...")) {
         @Override
         public void run()
         {
            selectTemplateToRemove();
         }
      };

      actionAddClusterNode = new Action(i18n.tr("&Add node...")) {
         @Override
         public void run()
         {
            addNodeToCluster();
         }
      };

      actionRemoveClusterNode = new Action(i18n.tr("&Remove node...")) {
         @Override
         public void run()
         {
            removeNodeFromCluster();
         }
      };

      actionAddWirelessController = new Action(i18n.tr("&Add controller node...")) {
         @Override
         public void run()
         {
            addNodeToWirelessDomain();
         }
      };

      actionRemoveWirelessController = new Action(i18n.tr("&Remove controller node...")) {
         @Override
         public void run()
         {
            removeNodeFromWirelessDomain();
         }
      };

      actionRouteFrom = new Action(i18n.tr("Route from...")) {
         @Override
         public void run()
         {
            showRoute(true);
         }
      };

      actionRouteTo = new Action(i18n.tr("Route to...")) {
         @Override
         public void run()
         {
            showRoute(false);
         }
      };

      actionRouteFromMgmtNode = new Action(i18n.tr("Route from NetXMS server")) {
         @Override
         public void run()
         {
            showRouteFromManagementNode();
         }
      };

      actionLayer2Topology = new Action(i18n.tr("&Layer 2 topology")) {
         @Override
         public void run()
         {
            view.openView(new L2TopologyMapView(getObjectIdFromSelection()));
         }
      };

      actionIPTopology = new Action(i18n.tr("&IP topology")) {
         @Override
         public void run()
         {
            view.openView(new IPTopologyMapView(getObjectIdFromSelection()));
         }
      };

      actionInternalTopology = new Action(i18n.tr("Internal &communication topology")) {
         @Override
         public void run()
         {
            view.openView(new InternalTopologyMapView(getObjectIdFromSelection()));
         }
      };

      ViewPlacement viewPlacement = new ViewPlacement(view);
      actionCreateInterfaceDCI = new CreateInterfaceDciAction(viewPlacement, selectionProvider);
      actionForcePolicyInstall = new ForcedPolicyDeploymentAction(viewPlacement, selectionProvider);
      actionLinkAssetToObject = new LinkAssetToObjectAction(viewPlacement, selectionProvider);
      actionUnlinkAssetFromObject = new UnlinkAssetFromObjectAction(viewPlacement, selectionProvider);
      actionLinkObjectToAsset = new LinkObjectToAssetAction(viewPlacement, selectionProvider);
      actionUnlinkObjectFromAsset = new UnlinkObjectFromAssetAction(viewPlacement, selectionProvider);
      actionCloneAsDashboard = new CloneAsDashboardAction(viewPlacement, selectionProvider);
      actionCloneAsDashboardTemplate = new CloneAsDashboardTemplateAction(viewPlacement, selectionProvider);
      actionChangeZone = new ChangeZoneAction(viewPlacement, selectionProvider);
      actionSendUserAgentNotification = new SendUserAgentNotificationAction(viewPlacement, selectionProvider);
      actionExportDashboard = new ExportDashboardAction(viewPlacement, selectionProvider);
      actionImportDashboard = new ImportDashboardAction(viewPlacement, selectionProvider);
      actionAssignDashboard = new AssignDashboardAction(viewPlacement, selectionProvider);
      actionCloneNetworkMap = new CloneNetworkMap(viewPlacement, selectionProvider);
      actionNetworkMapPublicAccess = new EnableNetworkMapPublicAccess(viewPlacement, selectionProvider);
      actionChangeInterfaceExpectedState = new ChangeInterfaceExpectedStateAction(viewPlacement, selectionProvider);
      actionChangeMacAddress = new ChangeMacAddressAction(viewPlacement, selectionProvider);
      actionSetInterfacePeer = new SetInterfacePeerInformation(viewPlacement, selectionProvider);
      actionClearInterfacePeer = new ClearInterfacePeerInformation(viewPlacement, selectionProvider);
      actionUploadFileToAgent = new UploadFileToAgent(viewPlacement, selectionProvider);

      NXCSession session = Registry.getSession();
      ServiceLoader<ObjectActionDescriptor> actionLoader = ServiceLoader.load(ObjectActionDescriptor.class, getClass().getClassLoader());
      for(ObjectActionDescriptor a : actionLoader)
      {
         String componentId = a.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
            actionContributions.add(a.createAction(viewPlacement, selectionProvider));
      }
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
            if ((object instanceof DashboardGroup) || (object instanceof DashboardRoot))
               add(actionImportDashboard);
            add(new Separator());
         }
         if (object instanceof Asset)
         {
            add(actionLinkAssetToObject);
            add(actionUnlinkAssetFromObject);
            add(new Separator());
         }
         if ((object instanceof Circuit) || (object instanceof Collector) || (object instanceof Container) || (object instanceof ServiceRoot))
         {
            add(actionBind);
            add(actionUnbind);
            //separator will be added with move callback
         }
         if (object instanceof Template)
         {
            add(actionApplyTemplate);
            add(actionRemoveTemplate);
            add(actionForcePolicyInstall);
            add(new Separator());
         }
         if (object instanceof Cluster)
         {
            add(actionAddClusterNode);
            add(actionRemoveClusterNode);
            add(new Separator());
         }
         if (actionLinkObjectToAsset.isValidForSelection(selection))
         {
            add(actionLinkObjectToAsset);
            if (object.getAssetId() != 0)
               add(actionUnlinkObjectFromAsset);
            add(new Separator());            
         }
         if (object instanceof DashboardBase)
         {
            add(actionCloneAsDashboard);
            add(actionCloneAsDashboardTemplate);
            add(actionExportDashboard);
            add(new Separator());           
         }
         if (object instanceof WirelessDomain)
         {
            add(actionAddWirelessController);
            add(actionRemoveWirelessController);
            add(new Separator());
         }
      }
      else
      {
         if (actionUnlinkAssetFromObject.isValidForSelection(selection))
         {
            add(actionUnlinkAssetFromObject);
            add(new Separator());
         }
         else if (actionUnlinkObjectFromAsset.isValidForSelection(selection))
         {
            add(actionUnlinkObjectFromAsset);
            add(new Separator());
         }

         if (actionForcePolicyInstall.isValidForSelection(selection))
         {
            add(actionForcePolicyInstall);
            add(new Separator());
         }         
      }

      if (isBindToMenuAllowed(selection))
      {
         add(actionBindTo);
         if (singleObject)
            add(actionUnbindFrom);
         addObjectMoveActions(selection);
         add(new Separator());
      }
      else if (isAddToCircuitMenuAllowed(selection))
      {
         add(actionAddToCircuit);
         if (singleObject)
            add(actionRemoveFromCircuit);
         addObjectMoveActions(selection);
         add(new Separator());
      }
      else if (isDashboardOnlySelection(selection))
      {
         add(actionAssignDashboard);
         addObjectMoveActions(selection);
         add(new Separator());
      }
      else if (!containsRootObject(selection))
      {
         addObjectMoveActions(selection);
         add(new Separator()); 
      }

      if (isTemplateManagementAllowed(selection))
      {
         add(actionApplyNodeTemplate);
         add(actionRemoveObjectsTemplate);
         add(new Separator());
      }
      if (isMaintenanceMenuAllowed(selection))
      {
         MenuManager maintenanceMenu = new MaintenanceMenuManager(view, selectionProvider);
         if (!maintenanceMenu.isEmpty())
         {
            add(maintenanceMenu);
         }
      }
      if (isManagedMenuAllowed(selection))
      {
         add(actionManage);
         add(actionUnmanage);
      }
      if (singleObject && (objectViewer != null))
      {
         add(actionRename);
      }
      if (!containsRootObject(selection))
      {
         add(actionDelete);
      }
      if (singleObject && isChangeZoneMenuAllowed(selection))
      {
         add(actionChangeZone);
      }
      add(new Separator());
      
      if (isSendUANotificationMenuAllowed(selection))
      {
         add(actionSendUserAgentNotification);
      }

      // Agent management
      if (singleObject)
      {
         AbstractObject object = getObjectFromSelection();
         if ((object instanceof Node) && ((Node)object).hasAgent())
         {
            add(actionEditAgentConfig);
         }
      }

      // Package management
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

      if (actionUploadFileToAgent.isValidForSelection(selection))
      {
         add(actionUploadFileToAgent);
      }

      // Screenshots, etc. for single node
      if (singleObject)
      {
         add(new Separator());
         AbstractObject object = getObjectFromSelection();
         if (object instanceof Node)
         {
            if (((Node)object).hasVNC())
            {
               add(actionOpenRemoteControlView);
            }
            if (((Node)object).hasAgent() && ((Node)object).getPlatformName().startsWith("windows-"))
            {
               add(actionTakeScreenshot);
            }
            add(new Separator());
            if (((Node)object).hasSnmpAgent())
            {
               add(actionOpenMibExprorer);
            }
            if (((Node)object).getPrimaryIP().isValidUnicastAddress() || ((Node)object).isManagementServer())
            {
               add(new Separator());
               if (!((Node)object).isManagementServer())
                  add(actionRouteFromMgmtNode);
               add(actionRouteFrom);
               add(actionRouteTo);
               MenuManager topologyMapMenu = new MenuManager(i18n.tr("Topology maps"));
               topologyMapMenu.add(actionLayer2Topology);
               topologyMapMenu.add(actionIPTopology);
               topologyMapMenu.add(actionInternalTopology);
               add(topologyMapMenu);
            }
            add(new Separator());
         }
         add(actionExecuteScript);  
      }

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      if (singleObject)
      {
         AbstractObject object = getObjectFromSelection();
         MenuManager logMenu = new ObjectLogMenuManager(object, contextId, new ViewPlacement(view));
         if (!logMenu.isEmpty())
         {
            add(new Separator());
            add(logMenu);
         }
      }

      final Menu toolsMenu = ObjectMenuFactory.createToolsMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (toolsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("&Tools"), toolsMenu));
      }

      final Menu pollsMenu = ObjectMenuFactory.createPollMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (pollsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("P&oll"), pollsMenu));
      }

      add(new Separator());
      final Menu summaryTableMenu = ObjectMenuFactory.createSummaryTableMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (summaryTableMenu != null)
      {
         add(new MenuContributionItem(i18n.tr("S&ummary tables"), summaryTableMenu));
      }

      final Menu graphTemplatesMenu = ObjectMenuFactory.createGraphTemplatesMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (graphTemplatesMenu != null)
      {
         add(new MenuContributionItem(i18n.tr("&Graphs"), graphTemplatesMenu));
      }

      final Menu dashboardsMenu = ObjectMenuFactory.createDashboardsMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (dashboardsMenu != null)
      {
         add(new MenuContributionItem(i18n.tr("&Dashboards"), dashboardsMenu));
      }

      final Menu mapsMenu = ObjectMenuFactory.createMapsMenu(selection, contextId, getMenu(), null, new ViewPlacement(view));
      if (mapsMenu != null)
      {
         add(new MenuContributionItem(i18n.tr("Network &maps"), mapsMenu));
      }

      if (actionCloneNetworkMap.isValidForSelection(selection))
      {
         add(new Separator());
         add(actionCloneNetworkMap);
         add(actionNetworkMapPublicAccess);
      }

      if (actionChangeInterfaceExpectedState.isValidForSelection(selection))
      {
         add(new Separator());
         add(actionChangeInterfaceExpectedState);
      }

      if (actionSetInterfacePeer.isValidForSelection(selection))
      {
         add(actionSetInterfacePeer);
      }

      if (actionClearInterfacePeer.isValidForSelection(selection))
      {
         add(actionClearInterfacePeer);
      }

      if (actionChangeMacAddress.isValidForSelection(selection))
      {
         add(actionChangeMacAddress);
      }

      if (actionCreateInterfaceDCI.isValidForSelection(selection))
      {
         add(actionCreateInterfaceDCI);
      }

      if (!actionContributions.isEmpty())
      {
         boolean first = true;
         for(ObjectAction<?> a : actionContributions)
         {
            if (a.isValidForSelection(selection))
            {
               if (first)
               {
                  add(new Separator());
                  first = false;
               }
               add(a);
            }
         }
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
             (objectClass == AbstractObject.OBJECT_DASHBOARDTEMPLATE) ||
             (objectClass == AbstractObject.OBJECT_NETWORKMAP) || (objectClass == AbstractObject.OBJECT_NETWORKMAPGROUP) || (objectClass == AbstractObject.OBJECT_NETWORKMAPROOT) ||
             (objectClass == AbstractObject.OBJECT_TEMPLATE) || (objectClass == AbstractObject.OBJECT_TEMPLATEGROUP) || (objectClass == AbstractObject.OBJECT_TEMPLATEROOT) ||
             (objectClass == AbstractObject.OBJECT_ASSET) || (objectClass == AbstractObject.OBJECT_ASSETGROUP) || (objectClass == AbstractObject.OBJECT_ASSETROOT) ||
             (objectClass == AbstractObject.OBJECT_NETWORKSERVICE))
            return false;
      }
      return true;
   }

   /**
    * Check if manage/unmanage menu items are allowed.
    *
    * @param selection current object selection
    * @return true if manage/unmanage menu items are allowed
    */
   private static boolean isManagedMenuAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof AbstractObject))
            return false;
         int objectClass = ((AbstractObject)o).getObjectClass();
         if ((objectClass == AbstractObject.OBJECT_ASSET) || (objectClass == AbstractObject.OBJECT_ASSETGROUP) || (objectClass == AbstractObject.OBJECT_ASSETROOT))
            return false;
      }
      return true;
   }

   /**
    * Check if delete menu item is allowed.
    *
    * @param selection current object selection
    * @return true if maintenance menu is allowed
    */
   private static boolean containsRootObject(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof AbstractObject))
            return false;
         int objectClass = ((AbstractObject)o).getObjectClass();
         if ((objectClass == AbstractObject.OBJECT_BUSINESSSERVICEROOT) || (objectClass == AbstractObject.OBJECT_DASHBOARDROOT) || (objectClass == AbstractObject.OBJECT_NETWORKMAPROOT) || 
               (objectClass == AbstractObject.OBJECT_TEMPLATEROOT) || (objectClass == AbstractObject.OBJECT_ASSETROOT))
            return true;
      }
      return false;
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
             (objectClass != AbstractObject.OBJECT_SUBNET) && (objectClass != AbstractObject.OBJECT_ACCESSPOINT))
            return false;
      }
      return true;
   }

   /**
    * Check if "add to circuit" / "remove from circuit" menu is allowed.
    *
    * @param selection current object selection
    * @return true if "add to circuit" / "remove from circuit" menu is allowed
    */
   private static boolean isAddToCircuitMenuAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof Interface))
            return false;
      }
      return true;
   }

   /**
    * Check if current selection contains only dashboards.
    *
    * @param selection current object selection
    * @return true if only dashboards are selected
    */
   private static boolean isDashboardOnlySelection(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof DashboardBase))
            return false;
      }
      return true;
   }

   /**
    * Check if template remove/apply available
    *
    * @param selection current object selection
    * @return true template management is allowed
    */
   private static boolean isTemplateManagementAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof DataCollectionTarget))
            return false;
      }
      return true;
   }

   /**
    * Check if change zone menu items are allowed.
    *
    * @param selection current object selection
    * @return true if change zone menu items are allowed
    */
   private static boolean isChangeZoneMenuAllowed(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
      {
         if (!(o instanceof AbstractObject))
            return false;
         int objectClass = ((AbstractObject)o).getObjectClass();
         if ((objectClass == AbstractObject.OBJECT_NODE) || (objectClass == AbstractObject.OBJECT_CLUSTER))
            return true;
      }
      return false;
   }

   /**
    * Check if send user agent notification menu items are allowed.
    *
    * @param selection current object selection
    * @return true if send user agent notification menu items are allowed
    */
   private static boolean isSendUANotificationMenuAllowed(IStructuredSelection selection)
   {
      if (!selection.isEmpty())
      {
         for (Object obj : ((IStructuredSelection)selection).toList())
         {
            if (!(((obj instanceof AbstractNode) && ((AbstractNode)obj).hasAgent()) || 
                (obj instanceof Collector) || (obj instanceof Container) || (obj instanceof ServiceRoot) || (obj instanceof Rack) ||
                (obj instanceof Cluster)))
            {
               return false;
            }
         }
      }
      else
      {
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
      return view.getWindow().getShell();
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
    * Deploy package on node
    */
   private void deployPackage()
   {
      final PackageSelectionDialog dialog = new PackageSelectionDialog(view.getWindow().getShell());
      if (dialog.open() != Window.OK)
         return;

      final Set<Long> objects = new HashSet<Long>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {
         if (o instanceof AbstractObject)
         {
            objects.add(((AbstractObject)o).getObjectId());
         }
      }

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Starting agent package deployment"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deployPackage(dialog.getSelectedPackageId(), objects);
            runInUIThread(() -> view.addMessage(MessageArea.SUCCESS, i18n.tr("Package deployment started")));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start package deployment");
         }
      };
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

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new ScreenshotView((Node)object, null, null, contextId));
   }

   /**
    * Open VNC viewer
    */
   private void openRemoteControlView()
   {
      AbstractObject object = getObjectFromSelection();
      if (!(object instanceof Node))
         return;

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new RemoteControlView((Node)object, contextId));
   }

   /**
    * Open agent configuration editor
    */
   private void openAgentConfigEditor()
   {
      AbstractObject object = getObjectFromSelection();
      if (!(object instanceof Node))
         return;

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new AgentConfigurationEditor((Node)object, contextId));
   }

   /**
    * Execute script on object
    */
   private void executeScript()
   {
      AbstractObject object = getObjectFromSelection();
      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new ScriptExecutorView(object.getObjectId(), contextId));
   }

   /**
    * Open MIB explorer
    */
   private void openMibExplorer()
   {
      AbstractObject object = getObjectFromSelection();
      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new MibExplorer(object.getObjectId(), contextId, false));
   }

   /**
    * Apply selected in dialog object to selected in tree data collection targets
    */
   private void selectAndApplyTemplate()
   {
      final List<Long> targetsId = new ArrayList<>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {
         if (o instanceof DataCollectionTarget)
            targetsId.add(((AbstractObject)o).getObjectId());
      }
      
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
      dlg.enableMultiSelection(false);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Binding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(Long target : targetsId)
               session.bindObject(objects.get(0).getObjectId(), target);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot bind objects");
         }
      }.start();
   }


   /**
    * Remove selected in dialog templates from selected in tree data collection targets
    */
   private void selectTemplateToRemove()
   {
      final Set<Long> targetsId = new HashSet<Long>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {
         if (o instanceof DataCollectionTarget)
            targetsId.add(((AbstractObject)o).getObjectId());
      }
      
      final RelatedTemplateObjectSelectionDialog dlg = new RelatedTemplateObjectSelectionDialog(view.getWindow().getShell(), targetsId, RelationType.DIRECT_SUPERORDINATES, ObjectSelectionDialog.createTemplateSelectionFilter());
      dlg.setShowObjectPath(true);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Binding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for (AbstractObject object : objects)
               for(Long target : targetsId)
                  session.removeTemplate(object.getObjectId(), target, dlg.isRemoveDci());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot bind objects");
         }
      }.start();
   }

   /**
    * Bind objects to selected object
    */
   private void bindObjects()
   {
      final long parentId = getObjectIdFromSelection();
      if (parentId == 0)
         return;

      Set<Integer> filter;
      if (getObjectFromSelection() instanceof Circuit)
         filter = ObjectSelectionDialog.createInterfaceSelectionFilter();
      else
         filter = ObjectSelectionDialog.createDataCollectionTargetSelectionFilter();
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), filter);
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
    * Remove template from data collection target
    */
   private void removeTemplate()
   {
      final long parentId = getObjectIdFromSelection();
      if (parentId == 0)
         return;

      final RelatedTemplateObjectSelectionDialog dlg = new RelatedTemplateObjectSelectionDialog(view.getWindow().getShell(), parentId, RelatedObjectSelectionDialog.RelationType.DIRECT_SUBORDINATES, null);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Remove template"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.removeTemplate(parentId, o.getObjectId(), dlg.isRemoveDci());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot remove template");
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
    * Bind selected objects to container
    */
   private void bindToContainer()
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
    * Bind selected interfaces to circuit
    */
   private void addToCircuit()
   {
      final List<Long> childIdList = new ArrayList<>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {
         if (o instanceof Interface)
            childIdList.add(((AbstractObject)o).getObjectId());
      }

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createCircuitSelectionFilter());
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
   private void unbindFromParents()
   {
      final AbstractObject childObject = getObjectFromSelection();
      if (childObject == null)
         return;

      final RelatedObjectSelectionDialog dlg = new RelatedObjectSelectionDialog(view.getWindow().getShell(), childObject.getObjectId(),
            RelatedObjectSelectionDialog.RelationType.DIRECT_SUPERORDINATES,
            (childObject.getObjectClass() == AbstractObject.OBJECT_INTERFACE) ? ObjectSelectionDialog.createCircuitSelectionFilter() : ObjectSelectionDialog.createContainerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Unbinding objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.unbindObject(o.getObjectId(), childObject.getObjectId());
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
      new Job(i18n.tr("Adding node to cluster"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.addClusterNode(clusterId, o.getObjectId());          
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
      new Job(i18n.tr("Removing node from cluster"), view) {
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

   /**
    * Add node(s) to wireless domain
    */
   private void addNodeToWirelessDomain()
   {
      final long wirelessDomainId = getObjectIdFromSelection();
      if (wirelessDomainId == 0)
         return;

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Adding node to wireless domain"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(AbstractObject o : objects)
               session.addWirelessDomainController(wirelessDomainId, o.getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add node to wireless domain");
         }
      }.start();
   }

   /**
    * Remove node(s) from wireless domain
    */
   private void removeNodeFromWirelessDomain()
   {
      final long wirelessDomainId = getObjectIdFromSelection();
      if (wirelessDomainId == 0)
         return;

      final RelatedObjectSelectionDialog dlg = new RelatedObjectSelectionDialog(view.getWindow().getShell(), wirelessDomainId, RelatedObjectSelectionDialog.RelationType.DIRECT_SUBORDINATES,
            RelatedObjectSelectionDialog.createClassFilter(AbstractObject.OBJECT_NODE));
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Removing node from wireless domain"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AbstractObject> objects = dlg.getSelectedObjects();
            for(int i = 0; i < objects.size(); i++)
               session.removeWirelessDomainController(wirelessDomainId, objects.get(i).getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot remove node from wireless domain");
         }
      }.start();
   }

   /**
    * Show route between current node and another node selected by user
    * 
    * @param swap true to swap source and destination (current node is source by default)
    */
   private void showRoute(boolean swap)
   {
      AbstractObject source = getObjectFromSelection();
      if (!(source instanceof Node))
         return;

      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
      dlg.enableMultiSelection(false);
      if (dlg.open() != Window.OK)
         return;

      AbstractObject destination = dlg.getSelectedObjects().get(0);
      if (!(destination instanceof Node))
         return;

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : source.getObjectId();
      view.openView(
            swap ?
               new RouteView((Node)destination, (Node)source, contextId) :
               new RouteView((Node)source, (Node)destination, contextId));
   }

   /**
    * Show route from management server to selected node
    */
   private void showRouteFromManagementNode()
   {
      AbstractObject destination = getObjectFromSelection();
      if (!(destination instanceof Node))
         return;

      NXCSession session = Registry.getSession();
      Node source = (Node)session.findObject((o) -> (o instanceof Node) && ((Node)o).isManagementServer());
      if (source == null)
      {
         view.addMessage(MessageArea.ERROR, i18n.tr("NetXMS server node is not accessible"));
         return;
      }

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : destination.getObjectId();
      view.openView(new RouteView(source, (Node)destination, contextId));
   }

   /**
    * Add menu items for moving objects. Default implementation does nothing.
    * 
    * @param selection current selection
    */
   protected void addObjectMoveActions(IStructuredSelection selection)
   {
   }
}
