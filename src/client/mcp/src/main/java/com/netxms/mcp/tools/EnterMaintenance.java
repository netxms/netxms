/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
package com.netxms.mcp.tools;

import java.util.Date;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import com.netxms.mcp.Startup;

/**
 * Tool to enter maintenance mode for a given object
 */
public class EnterMaintenance extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "enter-maintenance";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Enter maintenance mode for a given object. This tool requires an object ID and maintenance reason as parameters.\nMaintenance duration can be specified using 'duration' parameter (in minutes). If omitted, maintenance mode will be entered without time limit.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the object to put to maintenance", true)
         .addArgument("reason", "string", "Maintenance reason", true)
            .addArgument("duration", "integer", "Maintenance duration in minutes (0 for unlimited)", false)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      String reason = (String)args.get("reason");
      if ((reason == null) || reason.isEmpty())
         throw new Exception("Maintenance reason not specified");

      session.setObjectMaintenanceMode(object.getObjectId(), true, reason);

      int duration = (args.get("duration") != null) ? (Integer)args.get("duration") : 0;
      if (duration > 0)
      {
         long endTime = System.currentTimeMillis() + duration * 60000L;
         ScheduledTask taskEnd = new ScheduledTask("Maintenance.Leave", "", "", reason, new Date(endTime), ScheduledTask.SYSTEM, object.getObjectId());
         session.addScheduledTask(taskEnd);
      }

      return "Successfully entered maintenance mode";
   }
}
