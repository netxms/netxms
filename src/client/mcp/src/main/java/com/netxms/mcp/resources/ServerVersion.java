/**
 * 
 */
package com.netxms.mcp.resources;

import com.netxms.mcp.Startup;
import io.modelcontextprotocol.spec.McpSchema.ReadResourceRequest;

/**
 * 
 */
public class ServerVersion extends ServerResource
{
   /**
    * @see com.netxms.mcp.resources.ServerResource#getName()
    */
   @Override
   public String getName()
   {
      return "server-version";
   }

   /**
    * @see com.netxms.mcp.resources.ServerResource#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns the version of the NetXMS server.\nThis resource does not require any parameters.";
   }

   /**
    * @see com.netxms.mcp.resources.ServerResource#execute(io.modelcontextprotocol.spec.McpSchema.ReadResourceRequest)
    */
   @Override
   public String execute(ReadResourceRequest request) throws Exception
   {
      return Startup.getSession().getServerVersion();
   }
}
