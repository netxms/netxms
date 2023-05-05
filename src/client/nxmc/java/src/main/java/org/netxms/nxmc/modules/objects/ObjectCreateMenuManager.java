/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.BusinessServiceRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DashboardGroup;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.EntireNetwork;
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
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.dialogs.CreateBusinessServicePrototype;
import org.netxms.nxmc.modules.objects.dialogs.CreateChassisDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateInterfaceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateMobileDeviceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNetworkMapDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateNodeDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateObjectDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateSubnetDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateZoneDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectCreateMenuManager extends MenuManager
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectCreateMenuManager.class);
   
   private Shell shell;
   private View view;
   private AbstractObject parent;
   private long parentId;

   private Action actionCreateBusinessService;
   private Action actionCreateBusinessServicePrototype;
   private Action actionCreateChassis;
   private Action actionCreateCluster;
   private Action actionCreateCondition;
   private Action actionCreateContainer;
   private Action actionCreateDashboard;
   private Action actionCreateDashboardGroup;
   private Action actionCreateInterface;
   private Action actionCreateMobileDevice;
   private Action actionCreateNetworkMap;
   private Action actionCreateNetworkMapGroup;
   private Action actionCreateNode;
   private Action actionCreateRack;
   private Action actionCreateSubnet;
   private Action actionCreateTemplate;
   private Action actionCreateTemplateGroup;
   private Action actionCreateVpnConnector;
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

      addAction(this, actionCreateBusinessService, (AbstractObject o) -> (o instanceof BusinessService) || (o instanceof BusinessServiceRoot) && !(o instanceof BusinessServicePrototype));
      addAction(this, actionCreateBusinessServicePrototype, (AbstractObject o) -> (o instanceof BusinessService) || (o instanceof BusinessServiceRoot));
      addAction(this, actionCreateChassis, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCluster, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateCondition, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateContainer, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateDashboard, (AbstractObject o) -> (o instanceof DashboardGroup) || (o instanceof DashboardRoot));
      addAction(this, actionCreateDashboardGroup, (AbstractObject o) -> (o instanceof DashboardGroup) || (o instanceof DashboardRoot));
      addAction(this, actionCreateInterface, (AbstractObject o) -> o instanceof Node);
      addAction(this, actionCreateMobileDevice, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateNetworkMap, (AbstractObject o) -> (o instanceof NetworkMapGroup) || (o instanceof NetworkMapRoot));
      addAction(this, actionCreateNetworkMapGroup, (AbstractObject o) -> (o instanceof NetworkMapGroup) || (o instanceof NetworkMapRoot));
      addAction(this, actionCreateNode, (AbstractObject o) -> (o instanceof Cluster) || (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateRack, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(this, actionCreateSubnet, (AbstractObject o) -> (o instanceof Zone) || ((o instanceof EntireNetwork) && !Registry.getSession().isZoningEnabled()));
      addAction(this, actionCreateTemplate, (AbstractObject o) -> (o instanceof TemplateGroup) || (o instanceof TemplateRoot));
      addAction(this, actionCreateTemplateGroup, (AbstractObject o) -> (o instanceof TemplateGroup) || (o instanceof TemplateRoot));
      addAction(this, actionCreateVpnConnector, (AbstractObject o) -> o instanceof Node);
      addAction(this, actionCreateZone, (AbstractObject o) -> (o instanceof EntireNetwork) && Registry.getSession().isZoningEnabled());
   }

   /**
    * Create object actions
    */
   protected void createActions()
   {
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
            new Job(i18n.tr("Creating business service prototype"), view, null) {
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

               new Job(i18n.tr("Creating chassis"), view) {
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

      actionCreateCluster = new GenericObjectCreationAction(i18n.tr("C&luster..."), AbstractObject.OBJECT_CLUSTER, i18n.tr("Cluster"));
      actionCreateCondition = new GenericObjectCreationAction(i18n.tr("C&ondition..."), AbstractObject.OBJECT_CONDITION, i18n.tr("Condition"));
      actionCreateContainer = new GenericObjectCreationAction(i18n.tr("&Container..."), AbstractObject.OBJECT_CONTAINER, i18n.tr("Container"));
      actionCreateDashboard = new GenericObjectCreationAction(i18n.tr("&Dashboard..."), AbstractObject.OBJECT_DASHBOARD, i18n.tr("Dashboard"));
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
            new Job(i18n.tr("Creating interface"), view, null) {
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
            new Job(i18n.tr("Creating mobile device"), view, null) {
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
            new Job(i18n.tr("Creating network map"), view) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NETWORKMAP, dlg.getName(), parentId);
                  cd.setMapType(dlg.getType());
                  cd.setSeedObjectId(dlg.getSeedObject());
                  cd.setObjectAlias(dlg.getAlias());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create network map object %s"), dlg.getName());
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
               cd.setSnmpPort(dlg.getSnmpPort());
               cd.setEtherNetIpPort(dlg.getEtherNetIpPort());
               cd.setSshPort(dlg.getSshPort());
               cd.setAgentProxyId(dlg.getAgentProxy());
               cd.setSnmpProxyId(dlg.getSnmpProxy());
               cd.setEtherNetIpProxyId(dlg.getEtherNetIpProxy());
               cd.setIcmpProxyId(dlg.getIcmpProxy());
               cd.setSshProxyId(dlg.getSshProxy());
               cd.setWebServiceProxyId(dlg.getWebServiceProxy());
               cd.setZoneUIN(dlg.getZoneUIN());
               cd.setSshLogin(dlg.getSshLogin());
               cd.setSshPassword(dlg.getSshPassword());
               cd.setObjectAlias(dlg.getObjectAlias());

               final NXCSession session = Registry.getSession();
               new Job(i18n.tr("Creating node"), view, null) {
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

      actionCreateRack = new GenericObjectCreationAction(i18n.tr("&Rack..."), AbstractObject.OBJECT_RACK, i18n.tr("Rack"));

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
            new Job(i18n.tr("Creating zone"), view, null) {
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
         new Job(String.format(i18n.tr("Creating %s"), className), view, null) {
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
               return String.format(i18n.tr("Cannot create %s object %s"), objectClass, dlg.getObjectName());
            }
         }.start();
      }
   }
}
