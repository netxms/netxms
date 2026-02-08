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
import com.netxms.mcp.Startup;

/**
 * Get source code of NXSL library script
 */
public class GetLibraryScript extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "get-library-script";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Get source code of NXSL library script.\nThis tool requires script name or ID as parameter.\nTool output is the script source code.";
   }
   
   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("script_name", "string", "Name of the library script", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      String scriptName = (String)args.get("script_name");

      long scriptId;
      try
      {
         scriptId = Long.parseLong(scriptName);
      }
      catch(NumberFormatException e)
      {
         scriptId = Startup.getSession().getScriptLibrary().stream()
               .filter(s -> s.getName().equalsIgnoreCase(scriptName))
               .map(s -> s.getId())
               .findFirst()
               .orElse(0L);
      }

      if (scriptId == 0)
         throw new Exception("Library script not found");

      return Startup.getSession().getScript(scriptId).getSource();
   }
}
