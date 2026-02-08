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

import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.netxms.mcp.Startup;

/**
 * Tool to list all library scripts on the server
 */
public class ListLibraryScripts extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "list-library-scripts";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Get list of all library scripts on the server.\nThis tool does not require any parameters.\nTool output is a JSON array with script names.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      List<Script> scripts = session.getScriptLibrary();
      ObjectMapper mapper = new ObjectMapper();
      ArrayNode values = mapper.createArrayNode();
      for(Script s : scripts)
         values.add(s.getName());
      return mapper.writeValueAsString(values);
   }
}
