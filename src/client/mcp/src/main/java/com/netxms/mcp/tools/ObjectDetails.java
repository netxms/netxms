/**
 * 
 */
package com.netxms.mcp.tools;

import java.util.Map;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * 
 */
public class ObjectDetails extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "object-details";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns details of a specific object from the NetXMS server.\nThis tool requires an object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the object to retrieve details for", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      String objectIdParam = (String) args.get("object_id");
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
      catch (NumberFormatException e)
      {
         // If parsing as long fails, try to find by name
         object = session.findObjectByName(objectIdParam.trim());
      }

      if (object == null)
      {
         throw new Exception("Object not found: " + objectIdParam);
      }

      // Create JSON response with basic object details
      ObjectMapper mapper = new ObjectMapper();
      ObjectNode result = mapper.createObjectNode();
      
      result.put("id", object.getObjectId());
      result.put("name", object.getObjectName());
      result.put("alias", object.getAlias() != null ? object.getAlias() : "");
      result.put("comments", object.getComments() != null ? object.getComments() : "");
      result.put("class", object.getObjectClassName());
      result.put("status", object.getStatus().toString());

      // Add Node-specific details if object is a Node
      if (object instanceof Node)
      {
         Node node = (Node) object;
         result.put("ip_address", node.getPrimaryIP().getHostAddress());
         result.put("vendor", node.getHardwareVendor());
         result.put("model", node.getHardwareProductName());
         result.put("serial_number", node.getHardwareSerialNumber());
      }

      return mapper.writeValueAsString(result);
   }
}