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
package org.netxms.nxmc.modules.logviewer;

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.services.LogDescriptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Registry for log descriptors
 */
public final class LogDescriptorRegistry
{
   private static final Logger logger = LoggerFactory.getLogger(LogDescriptorRegistry.class);

   private final I18n i18n = LocalizationHelper.getI18n(LogDescriptorRegistry.class);

   private List<LogDescriptor> descriptors = new ArrayList<>();

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(Display display, NXCSession session)
   {
      LogDescriptorRegistry instance = new LogDescriptorRegistry(session);
      Registry.setSingleton(display, LogDescriptorRegistry.class, instance);
   }

   /**
    * Create new descriptor registry
    * 
    * @param session current user session
    */
   private LogDescriptorRegistry(NXCSession session)
   {
      descriptors.add(new LogDescriptor("AlarmLog", i18n.tr("Alarms"), null, "source_object_id") {
         @Override
         public boolean isApplicableForObject(AbstractObject object)
         {
            return super.isApplicableForObject(object) || (object instanceof BusinessService); 
         }
      });
      descriptors.add(new LogDescriptor("AssetChangeLog", i18n.tr("Asset Changes"), i18n.tr("Asset changes"), "linked_object_id"));
      descriptors.add(new LogDescriptor("AuditLog", i18n.tr("Audit"), null, "object_id") {
         @Override
         public boolean isApplicableForObject(AbstractObject object)
         {
            return true; // Audit is available for all objects
         }
      });
      descriptors.add(new LogDescriptor("EventLog", i18n.tr("Events"), null, "event_source") {
         @Override
         public boolean isApplicableForObject(AbstractObject object)
         {
            return super.isApplicableForObject(object) || (object instanceof BusinessService); 
         }
      });
      descriptors.add(new LogDescriptor("MaintenanceJournal", i18n.tr("Maintenance Journal"), i18n.tr("Maintenance journal"), "object_id"));
      descriptors.add(new LogDescriptor("NotificationLog", i18n.tr("Notifications"), null, null));
      descriptors.add(new LogDescriptor("ServerActionExecutionLog", i18n.tr("Server Action Executions"), null, null));
      descriptors.add(new LogDescriptor("SnmpTrapLog", i18n.tr("SNMP Traps"), i18n.tr("SNMP traps"), "object_id"));
      descriptors.add(new LogDescriptor("syslog", i18n.tr("Syslog"), null, "source_object_id"));
      descriptors.add(new LogDescriptor("WindowsEventLog", i18n.tr("Windows Events"), i18n.tr("Windows events"), "node_id") {
         @Override
         public boolean isApplicableForObject(AbstractObject object)
         {
            return ObjectTool.isContainerObject(object) || ((object instanceof Node) && (((Node)object).getPlatformName().startsWith("windows")));
         }
      });

      ServiceLoader<LogDescriptor> loader = ServiceLoader.load(LogDescriptor.class, getClass().getClassLoader());
      for(LogDescriptor d : loader)
      {
         if (d.isValidForSession(session))
         {
            descriptors.add(d);
            logger.debug("Log descriptor " + d.getLogName() + " registered");
         }
         else
         {
            logger.debug("Log descriptor " + d.getLogName() + " is not valid for current session");
         }
      }

      descriptors.sort((LogDescriptor l1, LogDescriptor l2) -> l1.getViewTitle().compareToIgnoreCase(l2.getViewTitle()));
   }

   /**
    * Get registered log descriptors.
    *
    * @return registered log descriptors
    */
   public List<LogDescriptor> getDescriptors()
   {
      return descriptors;
   }

   public LogDescriptor get(String logName)
   {
      for (LogDescriptor d: descriptors)
      {
         if (d.getLogName().equals(logName))
            return d;
      }
      return null;
   }
}
