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

import java.util.Map;
import org.netxms.client.NXCSession;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.netxms.mcp.Startup;

/**
 * Tool to get NetXMS server internal statistics
 */
public class GetServerStats extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "get-server-stats";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Get internal NetXMS server statistics.\nThis tool does not require any parameters.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      ObjectMapper mapper = new ObjectMapper();
      return mapper.writeValueAsString(session.getServerStats());
   }
}
