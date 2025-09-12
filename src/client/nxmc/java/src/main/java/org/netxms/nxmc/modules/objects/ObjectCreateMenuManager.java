/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AssetGroup;
import org.netxms.client.objects.AssetRoot;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.NetworkMapGroup;
import org.netxms.client.objects.NetworkMapRoot;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.dialogs.CreateAssetDialog;
import org.netxms.nxmc.modules.businessservice.dialogs.CreateBusinessServicePrototype;
import org.netxms.nxmc.modules.objects.dialogs.CreateChassisDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateClusterDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateInterfaceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateMobileDeviceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNetworkMapDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNetworkServiceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNodeDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateObjectDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateRackDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateSensorDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateSubnetDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateZoneDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectCreateMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectCreateMenuManager.class);
   
   private Shell shell;
   private View view;
   private AbstractObject parent;
   private long parentId;

   private Action actionCreateAsset;
   private Action actionCreateAssetGroup;
   private Action actionCreateBusinessService;
   private Action actionCreateBusinessServicePrototype;
   private Action actionCreateChassis;
   private Action actionCreateCollector;
   private Action actionCreateCluster;
   private Action actionCreateCircuit;
   private Action actionCreateCondition;
   private Action actionCreateContainer;
   private Action actionCreateDashboard;
   private Action actionCreateDashboardGroup;
   private Action actionCreateDashboardTemplate;
   private Action actionCreateInterface;
   private Action actionCreateNetworkService;
   private Action actionCreateMobileDevice;
   private Action actionCreateNetworkMap;
   private Action actionCreateNetworkMapGroup;
   private Action actionCreateNode;
   private Action actionCreateRack;
   private Action actionCreateSensor;
   private Action actionCreateSubnet;
   private Action actionCreateTemplate;
   private Action actionCreateTemplateGroup;
   private Action actionCreateVpnConnector;
   private Action actionCreateWirelessDomain;
   private Action actionCreateZone;

   /**
    * Create new menu manager for object's "Create" menu.
    *
    * @param shell parent shell
    * @param view parent view
    * @param parent object to create menu for
    */
   public ObjectCreateMenuManager(Shell shell, View view, AbstractObject parent)
   {
      this.shell = shell;
      this.view = view;
      this.parent = parent;
      parentId = parent.getObjectId();
      setMenuText(i18n.tr("&Create"));     

      createActions();

      addAction(this, actionCreateAsset, (AbstractObject o) -> (o instanceof AssetGroup) || (o instanceof AssetRoot));
      addAction(this, actionCreateAssetGroup, (AbstractObject o) -> (o instanceof AssetGroup) || (o instanceof AssetRoot));
      addAction(this, actionCreateBusinessService, (AbstractObject o) -> (o instanceof BusinessService) || (o instanceof BusinessServiceRoot) && !(o instanceof BusinessServicePrototype));
      addAction(this, actionCreateBusinessServicePrototype, (AbstractObject o) -> (o instanceof BusinessService) || (o instanceof BusinessServiceRoot));
      addAction(this, actionCreateChassis, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCircuit, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCluster, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCondition, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCollector, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateContainer, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateDashboard, (AbstractObject o) -> (o instanceof DashboardGroup) || (o instanceof DashboardRoot));
      addAction(this, actionCreateDashboardGroup, (AbstractObject o) -> (o instanceof DashboardGroup) || (o instanceof DashboardRoot));
      addAction(this, actionCreateDashboardTemplate, (AbstractObject o) -> (o instanceof DashboardGroup) || (o instanceof DashboardRoot));
      addAction(this, actionCreateInterface, (AbstractObject o) -> o instanceof Node);
      addAction(this, actionCreateNetworkService, (AbstractObject o) -> o instanceof Node);
      addAction(this, actionCreateMobileDevice, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateNetworkMap, (AbstractObject o) -> (o instanceof NetworkMapGroup) || (o instanceof NetworkMapRoot));
      addAction(this, actionCreateNetworkMapGroup, (AbstractObject o) -> (o instanceof NetworkMapGroup) || (o instanceof NetworkMapRoot));
      addAction(this, actionCreateNode, (AbstractObject o) -> (o instanceof Cluster) || (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateRack, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateSensor, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateSubnet, (AbstractObject o) -> (o instanceof Zone) || ((o instanceof EntireNetwork) && !Registry.getSession().isZoningEnabled()));
      addAction(this, actionCreateTemplate, (AbstractObject o) -> (o instanceof TemplateGroup) || (o instanceof TemplateRoot));
      addAction(this, actionCreateTemplateGroup, (AbstractObject o) -> (o instanceof TemplateGroup) || (o instanceof TemplateRoot));
      addAction(this, actionCreateVpnConnector, (AbstractObject o) -> o instanceof Node);
      addAction(this, actionCreateWirelessDomain, (AbstractObject o) -> (o instanceof Container) || (o instanceof Collector) || (o instanceof ServiceRoot));
      addAction(this, actionCreateZone, (AbstractObject o) -> (o instanceof EntireNetwork) && Registry.getSession().isZoningEnabled());
   }

   /**
    * Create object actions
    */
   protected void createActions()
   {
      actionCreateAsset = new Action(i18n.tr("&Asset...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateAssetDialog dlg = new CreateAssetDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating asset"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ASSET, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());
                  cd.setAssetProperties(dlg.getProperties());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create asset object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateAssetGroup = new GenericObjectCreationAction(i18n.tr("Asset &group..."), AbstractObject.OBJECT_ASSETGROUP, i18n.tr("Asset Group"));

      actionCreateBusinessService = new GenericObjectCreationAction(i18n.tr("&Business service..."), AbstractObject.OBJECT_BUSINESSSERVICE, i18n.tr("Business Service"));

      actionCreateBusinessServicePrototype = new Action(i18n.tr("Business service &prototype...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateBusinessServicePrototype dlg = new CreateBusinessServicePrototype(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating business service prototype"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE, dlg.getName(), parentId);
                  cd.setInstanceDiscoveryMethod(dlg.getInstanceDiscoveyMethod());
                  cd.setObjectAlias(dlg.getAlias());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create business service prototype object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateChassis = new Action(i18n.tr("C&hassis...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final NXCSession session = Registry.getSession();
            CreateChassisDialog dlg = null;
            do
            {
               dlg = new CreateChassisDialog(shell, dlg);
               if (dlg.open() != Window.OK)
                  return;

               final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CHASSIS, dlg.getObjectName(), parentId);
               cd.setControllerId(dlg.getControllerId());
               cd.setObjectAlias(dlg.getObjectAlias());

               new Job(i18n.tr("Creating chassis"), view, getMessageArea(view)) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.createObject(cd);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format(i18n.tr("Cannot create chassis object %s"), cd.getName());
                  }
               }.start();
            } while(dlg.isShowAgain());
         }
      };

      actionCreateCluster = new Action(i18n.tr("C&luster...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateClusterDialog dlg = new CreateClusterDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating cluster"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CLUSTER, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());
                  cd.setZoneUIN(dlg.getZoneUIN());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create cluster object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateCircuit = new GenericObjectCreationAction(i18n.tr("&Circuit..."), AbstractObject.OBJECT_CIRCUIT, i18n.tr("Circuit"));
      actionCreateCondition = new GenericObjectCreationAction(i18n.tr("C&ondition..."), AbstractObject.OBJECT_CONDITION, i18n.tr("Condition"));
      actionCreateCollector = new GenericObjectCreationAction(i18n.tr("&Collector..."), AbstractObject.OBJECT_COLLECTOR, i18n.tr("Collector"));
      actionCreateContainer = new GenericObjectCreationAction(i18n.tr("&Container..."), AbstractObject.OBJECT_CONTAINER, i18n.tr("Container"));
      actionCreateDashboard = new GenericObjectCreationAction(i18n.tr("&Dashboard..."), AbstractObject.OBJECT_DASHBOARD, i18n.tr("Dashboard"));
      actionCreateDashboardTemplate = new GenericObjectCreationAction(i18n.tr("Dashboard &template..."), AbstractObject.OBJECT_DASHBOARDTEMPLATE, i18n.tr("Dashboard Template"));
      actionCreateDashboardGroup = new GenericObjectCreationAction(i18n.tr("Dashboard &group..."), AbstractObject.OBJECT_DASHBOARDGROUP, i18n.tr("Dashboard Group"));

      actionCreateInterface = new Action(i18n.tr("&Interface...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateInterfaceDialog dlg = new CreateInterfaceDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating interface"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_INTERFACE, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());                  
                  cd.setMacAddress(dlg.getMacAddress());
                  cd.setIpAddress(dlg.getIpAddress());
                  cd.setPhysicalPort(dlg.isPhysicalPort());
                  cd.setModule(dlg.getSlot());
                  cd.setPort(dlg.getPort());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create interface object %s"), dlg.getName());
               }
            }.start();
         }
      };
      
      actionCreateNetworkService = new Action(i18n.tr("&Network service...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateNetworkServiceDialog dlg = new CreateNetworkServiceDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating network service"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NETWORKSERVICE, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());
                  cd.setServiceType(dlg.getServiceType());
                  cd.setIpPort(dlg.getPort());
                  cd.setRequest(dlg.getRequest());
                  cd.setResponse(dlg.getResponse());
                  cd.setCreateStatusDci(dlg.isCreateDci());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create network service object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateMobileDevice = new Action(i18n.tr("&Mobile device...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateMobileDeviceDialog dlg = new CreateMobileDeviceDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating mobile device"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_MOBILEDEVICE, dlg.getName(), parentId);
                  cd.setDeviceId(dlg.getDeviceId());
                  cd.setObjectAlias(dlg.getAlias());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create mobile device object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateNetworkMap = new Action(i18n.tr("Network &map...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateNetworkMapDialog dlg = new CreateNetworkMapDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating network map"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NETWORKMAP, dlg.getName(), parentId);
                  cd.setMapType(dlg.getType());
                  cd.setSeedObjectId(dlg.getSeedObject());
                  cd.setObjectAlias(dlg.getAlias());
                  long newObjectId = session.createObject(cd);

                  long mapTemplateId = dlg.getTemplateMapId();
                  if (mapTemplateId != 0)
                  {
                     NetworkMap templateMap = session.findObjectById(mapTemplateId, NetworkMap.class);
                     NXCObjectModificationData md = new NXCObjectModificationData(newObjectId);
                     templateMap.updateWithTemplateData(md);                     
                     session.modifyObject(md);
                  }
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot create network map object {0}", dlg.getName());
               }
            }.start();
         }
      };

      actionCreateNetworkMapGroup = new GenericObjectCreationAction(i18n.tr("Network map &group..."), AbstractObject.OBJECT_NETWORKMAPGROUP, i18n.tr("Network Map Group"));

      actionCreateNode = new Action(i18n.tr("&Node...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            CreateNodeDialog dlg = null;
            do
            {
               dlg = new CreateNodeDialog(shell, dlg);
               if (dlg.open() != Window.OK)
                  return;

               final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, dlg.getObjectName(), parentId);
               cd.setCreationFlags(dlg.getCreationFlags());
               cd.setPrimaryName(dlg.getHostName());
               cd.setObjectAlias(dlg.getObjectAlias());
               cd.setAgentPort(dlg.getAgentPort());
               cd.setAgentProxyId(dlg.getAgentProxy());
               cd.setSnmpPort(dlg.getSnmpPort());
               cd.setSnmpProxyId(dlg.getSnmpProxy());
               cd.setEtherNetIpPort(dlg.getEtherNetIpPort());
               cd.setEtherNetIpProxyId(dlg.getEtherNetIpProxy());
               cd.setModbusTcpPort(dlg.getModbusTcpPort());
               cd.setModbusUnitId(dlg.getModbusUnitId());
               cd.setModbusProxyId(dlg.getModbusProxy());
               cd.setIcmpProxyId(dlg.getIcmpProxy());
               cd.setWebServiceProxyId(dlg.getWebServiceProxy());
               cd.setSshPort(dlg.getSshPort());
               cd.setSshProxyId(dlg.getSshProxy());
               cd.setSshLogin(dlg.getSshLogin());
               cd.setSshPassword(dlg.getSshPassword());
               cd.setMqttProxyId(dlg.getMqttProxy());
               cd.setZoneUIN(dlg.getZoneUIN());

               final NXCSession session = Registry.getSession();
               new Job(i18n.tr("Creating node"), view, getMessageArea(view)) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.createObject(cd);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format(i18n.tr("Cannot create node object %s"), cd.getName());
                  }
               }.start();
            } while(dlg.isShowAgain());
         }
      };

      actionCreateRack = new Action(i18n.tr("&Rack...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateRackDialog dlg = new CreateRackDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating rack"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_RACK, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());
                  cd.setHeight(dlg.getHeight());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create rack object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateSensor = new Action(i18n.tr("&Sensor...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateSensorDialog dlg = new CreateSensorDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating sensor"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_SENSOR, dlg.getName(), parentId);
                  cd.setObjectAlias(dlg.getAlias());
                  cd.setDeviceClass(dlg.getDeviceClass());
                  cd.setGatewayNodeId(dlg.getGatewayNodeId());
                  cd.setModbusUnitId(dlg.getModbusUnitId());
                  cd.setMacAddress(dlg.getMacAddress());
                  cd.setDeviceAddress(dlg.getDeviceAddress());
                  cd.setVendor(dlg.getVendor());
                  cd.setModel(dlg.getModel());
                  cd.setSerialNumber(dlg.getSerialNumber());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create sensor object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateSubnet = new Action(i18n.tr("&Subnet...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateSubnetDialog dlg = new CreateSubnetDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating subnet"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_SUBNET, dlg.getObjectName(), parentId);
                  cd.setObjectAlias(dlg.getObjectAlias());
                  cd.setIpAddress(dlg.getIpAddress());
                  if (parent instanceof Zone)
                     cd.setZoneUIN(((Zone)parent).getUIN());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  String name = dlg.getObjectName();
                  if (name.isEmpty())
                     name = dlg.getIpAddress().toString();
                  return String.format(i18n.tr("Cannot create subnet object %s"), name);
               }
            }.start();
         }
      };

      actionCreateTemplate = new GenericObjectCreationAction(i18n.tr("&Template..."), AbstractObject.OBJECT_TEMPLATE, i18n.tr("Template"));
      actionCreateTemplateGroup = new GenericObjectCreationAction(i18n.tr("Template &group..."), AbstractObject.OBJECT_TEMPLATEGROUP, i18n.tr("Template Group"));
      actionCreateVpnConnector = new GenericObjectCreationAction(i18n.tr("&VPN connector..."), AbstractObject.OBJECT_VPNCONNECTOR, i18n.tr("VPN Connector"));
      actionCreateWirelessDomain = new GenericObjectCreationAction(i18n.tr("&Wireless domain..."), AbstractObject.OBJECT_WIRELESSDOMAIN, i18n.tr("Wireless Domain"));

      actionCreateZone = new Action(i18n.tr("&Zone...")) {
         @Override
         public void run()
         {
            if (parentId == 0)
               return;

            final CreateZoneDialog dlg = new CreateZoneDialog(shell);
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating zone"), view, getMessageArea(view)) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ZONE, dlg.getName(), parentId);
                  cd.setZoneUIN(dlg.getZoneUIN());
                  cd.setObjectAlias(dlg.getAlias());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create zone object %s"), dlg.getName());
               }
            }.start();
         }
      };
   }

   /**
    * Add action to given menu manager
    *
    * @param manager menu manager
    * @param action action to add
    * @param filter filter for current selection
    */
   protected void addAction(IMenuManager manager, Action action, ObjectFilter filter)
   {
      if (filter.accept(parent))
         manager.add(action);
   }

   /**
    * Action for generic object creation (when only object name is requested)
    */
   private class GenericObjectCreationAction extends Action
   {
      private int objectClass;
      private String className;

      GenericObjectCreationAction(String name, int objectClass, String className)
      {
         super(name);
         this.objectClass = objectClass;
         this.className = className;
      }

      /**
       * @see org.eclipse.jface.action.Action#run()
       */
      @Override
      public void run()
      {
         if (parentId == 0)
            return;

         final CreateObjectDialog dlg = new CreateObjectDialog(shell, className);
         if (dlg.open() != Window.OK)
            return;

         final NXCSession session = Registry.getSession();
         new Job(String.format(i18n.tr("Creating %s"), className), view, getMessageArea(view)) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               NXCObjectCreationData cd = new NXCObjectCreationData(objectClass, dlg.getObjectName(), parentId);
               cd.setObjectAlias(dlg.getObjectAlias());
               session.createObject(cd);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create {0} object \"{1}\"", objectClass, dlg.getObjectName());
            }
         }.start();
      }
   }

   /**
    * Get message area from view
    *
    * @param view view
    * @return message area or null
    */
   private static MessageAreaHolder getMessageArea(View view)
   {
      return ((view != null) && (view.getPerspective() != null)) ? view.getPerspective().getMessageArea() : null;
   }
}
