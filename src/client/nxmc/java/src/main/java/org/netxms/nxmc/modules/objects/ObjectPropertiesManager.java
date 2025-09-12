/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.propertypages.DCIAutoBind;
import org.netxms.nxmc.modules.businessservice.propertypages.InstanceDiscovery;
import org.netxms.nxmc.modules.businessservice.propertypages.ObjectAutoBind;
import org.netxms.nxmc.modules.objects.propertypages.AccessControl;
import org.netxms.nxmc.modules.objects.propertypages.Agent;
import org.netxms.nxmc.modules.objects.propertypages.AutoApply;
import org.netxms.nxmc.modules.objects.propertypages.AutoBind;
import org.netxms.nxmc.modules.objects.propertypages.ClusterNetworks;
import org.netxms.nxmc.modules.objects.propertypages.ClusterResources;
import org.netxms.nxmc.modules.objects.propertypages.Comments;
import org.netxms.nxmc.modules.objects.propertypages.Communication;
import org.netxms.nxmc.modules.objects.propertypages.ConditionData;
import org.netxms.nxmc.modules.objects.propertypages.ConditionEvents;
import org.netxms.nxmc.modules.objects.propertypages.ConditionScript;
import org.netxms.nxmc.modules.objects.propertypages.CustomAttributes;
import org.netxms.nxmc.modules.objects.propertypages.DashboardElements;
import org.netxms.nxmc.modules.objects.propertypages.DashboardObjectContext;
import org.netxms.nxmc.modules.objects.propertypages.DashboardTemplateConfiguration;
import org.netxms.nxmc.modules.objects.propertypages.Dashboards;
import org.netxms.nxmc.modules.objects.propertypages.EtherNetIP;
import org.netxms.nxmc.modules.objects.propertypages.ExpectedCapabilities;
import org.netxms.nxmc.modules.objects.propertypages.ExternalResources;
import org.netxms.nxmc.modules.objects.propertypages.General;
import org.netxms.nxmc.modules.objects.propertypages.ICMP;
import org.netxms.nxmc.modules.objects.propertypages.InterfacePolling;
import org.netxms.nxmc.modules.objects.propertypages.Location;
import org.netxms.nxmc.modules.objects.propertypages.LocationControl;
import org.netxms.nxmc.modules.objects.propertypages.MQTT;
import org.netxms.nxmc.modules.objects.propertypages.MapAppearance;
import org.netxms.nxmc.modules.objects.propertypages.MapBackground;
import org.netxms.nxmc.modules.objects.propertypages.MapObjectContext;
import org.netxms.nxmc.modules.objects.propertypages.MapObjectFilter;
import org.netxms.nxmc.modules.objects.propertypages.MapLinkStylingScript;
import org.netxms.nxmc.modules.objects.propertypages.MapOptions;
import org.netxms.nxmc.modules.objects.propertypages.MapSeedNodes;
import org.netxms.nxmc.modules.objects.propertypages.Modbus;
import org.netxms.nxmc.modules.objects.propertypages.NetworkServicePolling;
import org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage;
import org.netxms.nxmc.modules.objects.propertypages.PhysicalContainerPlacement;
import org.netxms.nxmc.modules.objects.propertypages.Polling;
import org.netxms.nxmc.modules.objects.propertypages.RackPassiveElements;
import org.netxms.nxmc.modules.objects.propertypages.RackProperties;
import org.netxms.nxmc.modules.objects.propertypages.ResponsibleUsers;
import org.netxms.nxmc.modules.objects.propertypages.SNMP;
import org.netxms.nxmc.modules.objects.propertypages.SSH;
import org.netxms.nxmc.modules.objects.propertypages.SensorProperties;
import org.netxms.nxmc.modules.objects.propertypages.StatusCalculation;
import org.netxms.nxmc.modules.objects.propertypages.Syslog;
import org.netxms.nxmc.modules.objects.propertypages.TrustedObjects;
import org.netxms.nxmc.modules.objects.propertypages.VNC;
import org.netxms.nxmc.modules.objects.propertypages.VPNSubnets;
import org.netxms.nxmc.modules.objects.propertypages.WebServices;
import org.netxms.nxmc.modules.objects.propertypages.ZoneAgentCredentials;
import org.netxms.nxmc.modules.objects.propertypages.ZoneCommunications;
import org.netxms.nxmc.modules.objects.propertypages.ZoneSNMPCredentials;
import org.netxms.nxmc.modules.objects.propertypages.ZoneSSHCredentials;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manager for object properties
 */
public class ObjectPropertiesManager
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectPropertiesManager.class);

   private static Set<Class<? extends ObjectPropertyPage>> pageClasses = new HashSet<Class<? extends ObjectPropertyPage>>();
   static
   {
      pageClasses.add(AccessControl.class);
      pageClasses.add(Agent.class);
      pageClasses.add(AutoApply.class);
      pageClasses.add(AutoBind.class);
      pageClasses.add(ObjectAutoBind.class);
      pageClasses.add(DCIAutoBind.class);
      pageClasses.add(ClusterNetworks.class);
      pageClasses.add(ClusterResources.class);
      pageClasses.add(Comments.class);
      pageClasses.add(Communication.class);
      pageClasses.add(ConditionEvents.class);
      pageClasses.add(ConditionData.class);
      pageClasses.add(ConditionScript.class);
      pageClasses.add(CustomAttributes.class);
      pageClasses.add(Dashboards.class);
      pageClasses.add(DashboardElements.class);
      pageClasses.add(DashboardObjectContext.class);
      pageClasses.add(DashboardTemplateConfiguration.class);
      pageClasses.add(EtherNetIP.class);
      pageClasses.add(ExpectedCapabilities.class);
      pageClasses.add(ExternalResources.class);
      pageClasses.add(General.class);
      pageClasses.add(ICMP.class);
      pageClasses.add(InstanceDiscovery.class);
      pageClasses.add(InterfacePolling.class);
      pageClasses.add(Location.class);
      pageClasses.add(LocationControl.class);
      pageClasses.add(MapAppearance.class);
      pageClasses.add(MapBackground.class);
      pageClasses.add(MapLinkStylingScript.class);
      pageClasses.add(MapObjectContext.class);
      pageClasses.add(MapObjectFilter.class);
      pageClasses.add(MapOptions.class);
      pageClasses.add(MapSeedNodes.class);
      pageClasses.add(Modbus.class);
      pageClasses.add(MQTT.class);
      pageClasses.add(NetworkServicePolling.class);
      pageClasses.add(PhysicalContainerPlacement.class);
      pageClasses.add(Polling.class);
      pageClasses.add(RackPassiveElements.class);
      pageClasses.add(RackProperties.class);
      pageClasses.add(ResponsibleUsers.class);
      pageClasses.add(SensorProperties.class);
      pageClasses.add(SNMP.class);
      pageClasses.add(SSH.class);
      pageClasses.add(StatusCalculation.class);
      pageClasses.add(Syslog.class);
      pageClasses.add(TrustedObjects.class);
      pageClasses.add(VNC.class);
      pageClasses.add(VPNSubnets.class);
      pageClasses.add(WebServices.class);
      pageClasses.add(ZoneAgentCredentials.class);
      pageClasses.add(ZoneCommunications.class);
      pageClasses.add(ZoneSNMPCredentials.class);
      pageClasses.add(ZoneSSHCredentials.class);
   }

   /**
    * Open object properties dialog.
    *
    * @param object object to open properties for
    * @param shell parent shell
    * @param messageArea message area for error reporting
    * @return true if dialog was closed by pressing OK button
    */
   public static boolean openObjectPropertiesDialog(AbstractObject object, Shell shell, MessageAreaHolder messageArea)
   {
      List<ObjectPropertyPage> pages = new ArrayList<ObjectPropertyPage>(pageClasses.size());
      for(Class<? extends ObjectPropertyPage> c : pageClasses)
      {
         try
         {
            ObjectPropertyPage p = c.getConstructor(AbstractObject.class).newInstance(object);
            if (p.isVisible())
            {
               p.setMessageArea(messageArea);
               pages.add(p);
            }
         }
         catch(Exception e)
         {
            logger.error("Error instantiating object property page", e);
         }
      }
      pages.sort((p1, p2) -> {
         int rc = p1.getPriority() - p2.getPriority();
         return (rc != 0) ? rc : p1.getTitle().compareToIgnoreCase(p2.getTitle());
      });

      PreferenceManager pm = new PreferenceManager();
      for(ObjectPropertyPage p : pages)
      {
         if (p.getParentId() == null)
            pm.addToRoot(new PreferenceNode(p.getId(), p));
      }
      for(ObjectPropertyPage p : pages)
      {
         String parentId = p.getParentId();
         if (parentId != null)
         {
            pm.addTo(parentId, new PreferenceNode(p.getId(), p));
         }
      }

      PreferenceDialog dlg = new PreferenceDialog(shell, pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(LocalizationHelper.getI18n(ObjectPropertiesManager.class).tr("Object Properties - {0}", object.getObjectName()));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }
}
