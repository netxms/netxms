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

import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objects.AbstractObject;
import com.netxms.mcp.Startup;

/**
 * 
 */
public class ExecuteScript extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "execute-script";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Execute NXSL script in context of given object.\nThis tool requires an object ID or name and script source code as parameters.\nTool output is the combined text output of the script.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the node object to rename", true)
            .addArgument("script_source_code", "string", "Script source code", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      String scriptSourceCode = (String)args.get("script_source_code");
      final StringBuilder output = new StringBuilder();
      session.executeScript(object.getObjectId(), scriptSourceCode, List.of(), new TextOutputListener() {
         @Override
         public void messageReceived(String text)
         {
            output.append(text);
         }

         @Override
         public void setStreamId(long streamId)
         {
         }

         @Override
         public void onSuccess()
         {
         }

         @Override
         public void onFailure(Exception exception)
         {
         }
      });
      return output.toString();
   }
}
