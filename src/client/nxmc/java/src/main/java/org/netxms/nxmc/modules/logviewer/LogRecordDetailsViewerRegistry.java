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

import java.util.HashMap;
import java.util.Map;
import org.netxms.nxmc.modules.logviewer.views.helpers.AlarmLogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.views.helpers.AuditLogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.views.helpers.EventLogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.views.helpers.NotificationLogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.views.helpers.WindowsEventLogRecordDetailsViewer;

/**
 * Registry for log record details viewers
 */
public final class LogRecordDetailsViewerRegistry
{
   private static Map<String, LogRecordDetailsViewer> registry = new HashMap<String, LogRecordDetailsViewer>();

   /**
    * Register default detail viewers at initialization
    */
   static
   {
      registry.put("AuditLog", new AuditLogRecordDetailsViewer());
      registry.put("EventLog", new EventLogRecordDetailsViewer());
      registry.put("NotificationLog", new NotificationLogRecordDetailsViewer());
      registry.put("WindowsEventLog", new WindowsEventLogRecordDetailsViewer());
      registry.put("AlarmLog", new AlarmLogRecordDetailsViewer());
   }

   /**
    * Private constructor to prevent instance creation
    */
   private LogRecordDetailsViewerRegistry()
   {
   }

   /**
    * Register details viewer for given log
    * 
    * @param logName log name
    * @param viewer details viewer
    */
   public static void register(String logName, LogRecordDetailsViewer viewer)
   {
      synchronized(registry)
      {
         registry.put(logName, viewer);
      }
   }

   /**
    * Unregister details viewer for given log
    * 
    * @param logName log name
    */
   public static void unregister(String logName)
   {
      synchronized(registry)
      {
         registry.remove(logName);
      }
   }

   /**
    * Get details viewer for given log
    * 
    * @param logName log name
    * @return details viewer or null
    */
   public static LogRecordDetailsViewer get(String logName)
   {
      synchronized(registry)
      {
         return registry.get(logName);
      }
   }
}
