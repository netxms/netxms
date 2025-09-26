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
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.AlarmManager;
import com.netxms.mcp.Startup;

/**
 * Implementation of MCP tool to retrieve active alarms from the NetXMS server.
 */
public class Alarms extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "alarms";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns a list of active alarms from the NetXMS server. Accepts optional parameters to filter the results.\n";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder().addArgument("min_severity", "string", "Filter by minimal alarm severity (one of critical, major, minor, warning, informational)", false)
            .addArgument("source", "string", "Filter by alarm source (e.g., node name or ID)", false)
            .addArgument("state", "string", "Filter by alarm state (one of outstanding, acknowledged, resolved)", false).addArgument("limit", "integer", "Maximum number of alarms to return", false)
            .build();
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      AlarmManager alarmManager = AlarmManager.getInstance();

      // Parse parameters
      String minSeverityStr = (String)args.get("min_severity");
      String sourceStr = (String)args.get("source");
      String stateStr = (String)args.get("state");
      Integer limit = args.get("limit") != null ? ((Number)args.get("limit")).intValue() : null;

      // Convert min_severity string to Severity enum
      Severity minSeverity = null;
      if (minSeverityStr != null)
      {
         switch(minSeverityStr.toLowerCase())
         {
            case "critical":
               minSeverity = Severity.CRITICAL;
               break;
            case "major":
               minSeverity = Severity.MAJOR;
               break;
            case "minor":
               minSeverity = Severity.MINOR;
               break;
            case "warning":
               minSeverity = Severity.WARNING;
               break;
            case "informational":
            case "normal":
               minSeverity = Severity.NORMAL;
               break;
            default:
               throw new IllegalArgumentException("Invalid severity: " + minSeverityStr);
         }
      }

      // Convert state string to state value
      int state = -1; // -1 means all states
      if (stateStr != null)
      {
         switch(stateStr.toLowerCase())
         {
            case "outstanding":
            case "active":
               state = Alarm.STATE_OUTSTANDING;
               break;
            case "acknowledged":
               state = Alarm.STATE_ACKNOWLEDGED;
               break;
            case "resolved":
               state = Alarm.STATE_RESOLVED;
               break;
            case "terminated":
            case "closed":
               state = Alarm.STATE_TERMINATED;
               break;
            default:
               throw new IllegalArgumentException("Invalid state: " + stateStr);
         }
      }

      // Parse source (could be node name or object ID)
      long sourceObjectId = 0;
      if (sourceStr != null)
      {
         try
         {
            sourceObjectId = Long.parseLong(sourceStr);
         }
         catch(NumberFormatException e)
         {
            AbstractObject object = session.findObjectByName(sourceStr);
            if (object != null)
            {
               sourceObjectId = object.getObjectId();
            }
            else
            {
               throw new IllegalArgumentException("Source object not found: " + sourceStr);
            }
         }
      }

      // Get alarms using AlarmManager
      List<Alarm> alarms;
      if (sourceObjectId > 0 || minSeverity != null || state != -1 || limit != null)
      {
         alarms = alarmManager.getAlarms(sourceObjectId, minSeverity, state, limit != null ? limit : 0);
      }
      else
      {
         alarms = alarmManager.getAllAlarms();
      }

      // Format response as JSON
      ObjectMapper mapper = new ObjectMapper();
      ObjectNode response = mapper.createObjectNode();
      ArrayNode alarmsArray = mapper.createArrayNode();

      for(Alarm alarm : alarms)
      {
         ObjectNode alarmNode = mapper.createObjectNode();
         alarmNode.put("id", alarm.getId());
         alarmNode.put("message", alarm.getMessage());
         alarmNode.put("severity", alarm.getCurrentSeverity().name());
         alarmNode.put("state", getStateString(alarm.getState()));
         alarmNode.put("source_object_id", alarm.getSourceObjectId());
         alarmNode.put("source_object_name", session.getObjectName(alarm.getSourceObjectId()));
         alarmNode.put("creation_time", alarm.getCreationTime().toString());
         alarmNode.put("last_change_time", alarm.getLastChangeTime().toString());
         alarmNode.put("repeat_count", alarm.getRepeatCount());
         alarmNode.put("key", alarm.getKey());
         alarmNode.put("sticky", alarm.isSticky());

         if (alarm.getState() == Alarm.STATE_ACKNOWLEDGED)
         {
            alarmNode.put("acknowledged_by_user", alarm.getAcknowledgedByUser());
         }
         if (alarm.getState() == Alarm.STATE_RESOLVED)
         {
            alarmNode.put("resolved_by_user", alarm.getResolvedByUser());
         }
         if (alarm.getState() == Alarm.STATE_TERMINATED)
         {
            alarmNode.put("terminated_by_user", alarm.getTerminatedByUser());
         }

         alarmsArray.add(alarmNode);
      }

      response.set("alarms", alarmsArray);
      response.put("total_count", alarms.size());

      return mapper.writeValueAsString(response);
   }

   /**
    * Convert alarm state to string representation
    * 
    * @param state alarm state value
    * @return string representation of the state
    */
   private String getStateString(int state)
   {
      switch(state)
      {
         case Alarm.STATE_OUTSTANDING:
            return "outstanding";
         case Alarm.STATE_ACKNOWLEDGED:
            return "acknowledged";
         case Alarm.STATE_RESOLVED:
            return "resolved";
         case Alarm.STATE_TERMINATED:
            return "terminated";
         default:
            return "unknown";
      }
   }
}
