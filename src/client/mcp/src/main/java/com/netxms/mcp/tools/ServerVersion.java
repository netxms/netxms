/**
 * 
 */
package com.netxms.mcp.tools;

import java.util.Map;
import com.netxms.mcp.Startup;

/**
 * 
 */
public class ServerVersion extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "server-version";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns the version of the NetXMS server.\nThis tool does not require any parameters.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      return Startup.getSession().getServerVersion();
   }
}
