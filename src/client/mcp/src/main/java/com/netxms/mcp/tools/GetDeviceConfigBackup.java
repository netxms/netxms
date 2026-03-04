/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Reden Solutions
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

import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.Map;
import org.netxms.client.DeviceConfigBackup;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Get specific device configuration backup content
 */
public class GetDeviceConfigBackup extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "get-device-config-backup";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Get device configuration backup content by backup ID, or get the latest backup if no backup ID is specified.\nReturns both running and startup configuration content.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the node", true)
         .addArgument("backup_id", "string", "Backup ID to retrieve (optional, defaults to latest backup)", false)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      String backupIdStr = (String)args.get("backup_id");
      DeviceConfigBackup backup;
      if (backupIdStr != null && !backupIdStr.trim().isEmpty())
      {
         long backupId = Long.parseLong(backupIdStr.trim());
         backup = session.getDeviceConfigBackup(object.getObjectId(), backupId);
      }
      else
      {
         backup = session.getLastDeviceConfigBackup(object.getObjectId());
      }

      ObjectMapper mapper = new ObjectMapper();
      ObjectNode result = mapper.createObjectNode();
      result.put("id", backup.getId());
      result.put("timestamp", backup.getTimestamp().getTime());
      result.put("isBinary", backup.isBinary());
      addConfig(result, "runningConfig", backup.getRunningConfig(), backup.isBinary());
      addConfig(result, "startupConfig", backup.getStartupConfig(), backup.isBinary());
      return mapper.writeValueAsString(result);
   }

   /**
    * Add configuration content to JSON node
    *
    * @param node JSON node
    * @param fieldName field name
    * @param config configuration content
    * @param isBinary true if binary content
    */
   private static void addConfig(ObjectNode node, String fieldName, byte[] config, boolean isBinary)
   {
      if (config == null || config.length == 0)
      {
         node.putNull(fieldName);
         return;
      }

      if (isBinary)
      {
         node.put(fieldName, Base64.getEncoder().encodeToString(config));
      }
      else
      {
         node.put(fieldName, new String(config, StandardCharsets.UTF_8));
      }
   }
}
