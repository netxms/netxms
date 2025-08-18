/**
 * 
 */
package com.netxms.mcp.resources;

import io.modelcontextprotocol.spec.McpSchema.ReadResourceRequest;

/**
 * 
 */
public abstract class ServerResource
{
   public abstract String getName();

   public abstract String getDescription();

   /**
    * Returns the MIME type of the resource.
    * 
    * @return MIME type
    */
   public String getMimeType()
   {
      return "text/plain";
   }

   /**
    * Returns the URI of the resource.
    *
    * @return URI of the resource in the format "netxms://resource-name"
    */
   public String getUri()
   {
      return "netxms://" + getName();
   }

   public abstract String execute(ReadResourceRequest request) throws Exception;
}
