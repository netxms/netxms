/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
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

import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import com.netxms.mcp.Startup;

/**
 * Tool that operates on a specific object
 */
public abstract class ObjectServerTool extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public final String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      String objectIdParam = (String)args.get("object_id");
      if (objectIdParam == null || objectIdParam.trim().isEmpty())
      {
         throw new IllegalArgumentException("object_id parameter is required");
      }

      AbstractObject object = null;

      // Try to parse as long integer first
      try
      {
         long objectId = Long.parseLong(objectIdParam.trim());
         object = session.findObjectById(objectId);
      }
      catch(NumberFormatException e)
      {
         // If parsing as long fails, try to find by name
         object = session.findObjectByName(objectIdParam.trim());
      }

      if (object == null)
      {
         throw new Exception("Object not found: " + objectIdParam);
      }

      return execute(object, args);
   }

   /**
    * Execute tool for given object
    *
    * @param object object
    * @param args tool arguments
    * @return tool output
    * @throws Exception on error
    */
   protected abstract String execute(AbstractObject object, Map<String, Object> args) throws Exception;
}
