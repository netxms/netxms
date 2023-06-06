/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Registry for log descriptors
 */
public final class LogDescriptorRegistry
{
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
      descriptors.add(new LogDescriptor(i18n.tr("Alarms"), "AlarmLog", "source_object_id"));
      descriptors.add(new LogDescriptor(i18n.tr("Events"), "EventLog", "event_source"));
      descriptors.add(new LogDescriptor(i18n.tr("SNMP Traps"), "SnmpTrapLog", "object_id"));
      descriptors.add(new LogDescriptor(i18n.tr("Syslog"), "syslog", "source_object_id"));
      descriptors.add(new LogDescriptor(i18n.tr("Windows Events"), "WindowsEventLog", "node_id") {
         @Override
         public boolean isApplicableForObject(AbstractObject object)
         {
            return ObjectTool.isContainerObject(object) || ((object instanceof Node) && (((Node)object).getPlatformName().startsWith("windows")));
         }
      });

      ServiceLoader<LogDescriptor> loader = ServiceLoader.load(LogDescriptor.class, getClass().getClassLoader());
      for(LogDescriptor d : loader)
         if (d.isValidForSession(session))
            descriptors.add(d);
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
}
