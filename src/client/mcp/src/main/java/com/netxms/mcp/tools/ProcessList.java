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

import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * 
 */
public class ProcessList extends ObjectServerTool
{
   private static final int COLUMN_PID = 0;
   private static final int COLUMN_NAME = 1;
   private static final int COLUMN_USER = 2;
   private static final int COLUMN_THREADS = 3;
   private static final int COLUMN_HANDLES = 4;
   private static final int COLUMN_VMSIZE = 5;
   private static final int COLUMN_RSS = 6;
   private static final int COLUMN_MEMORY_USAGE = 7;
   private static final int COLUMN_PAGE_FAULTS = 8;
   private static final int COLUMN_KTIME = 9;
   private static final int COLUMN_UTIME = 10;
   private static final int COLUMN_CMDLINE = 11;

   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "process-list";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of processes running on given node (server, workstation, or device).\nThis tool requires a node object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the node object to retrieve process list from", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      Table processTable = session.queryAgentTable(object.getObjectId(), "System.Processes");

      int[] indexes = new int[12];
      indexes[COLUMN_PID] = processTable.getColumnIndex("PID");
      indexes[COLUMN_NAME] = processTable.getColumnIndex("NAME");
      indexes[COLUMN_USER] = processTable.getColumnIndex("USER");
      indexes[COLUMN_THREADS] = processTable.getColumnIndex("THREADS");
      indexes[COLUMN_HANDLES] = processTable.getColumnIndex("HANDLES");
      indexes[COLUMN_VMSIZE] = processTable.getColumnIndex("VMSIZE");
      indexes[COLUMN_RSS] = processTable.getColumnIndex("RSS");
      indexes[COLUMN_MEMORY_USAGE] = processTable.getColumnIndex("MEMORY_USAGE");
      indexes[COLUMN_PAGE_FAULTS] = processTable.getColumnIndex("PAGE_FAULTS");
      indexes[COLUMN_KTIME] = processTable.getColumnIndex("KTIME");
      indexes[COLUMN_UTIME] = processTable.getColumnIndex("UTIME");
      indexes[COLUMN_CMDLINE] = processTable.getColumnIndex("CMDLINE");

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();
      for(TableRow p : processTable.getAllRows())
      {
         ObjectNode entry = mapper.createObjectNode();
         entry.put("pid", p.getValueAsLong(indexes[COLUMN_PID]));
         entry.put("name", p.getValue(indexes[COLUMN_NAME]));
         entry.put("user", p.getValue(indexes[COLUMN_USER]));
         entry.put("threads", p.getValueAsLong(indexes[COLUMN_THREADS]));
         entry.put("handles", p.getValueAsLong(indexes[COLUMN_HANDLES]));
         entry.put("vmsize", p.getValueAsLong(indexes[COLUMN_VMSIZE]));
         entry.put("rss", p.getValueAsLong(indexes[COLUMN_RSS]));
         entry.put("memory_usage", p.getValueAsLong(indexes[COLUMN_MEMORY_USAGE]));
         entry.put("page_faults", p.getValueAsLong(indexes[COLUMN_PAGE_FAULTS]));
         entry.put("kernel_time", p.getValueAsLong(indexes[COLUMN_KTIME]));
         entry.put("user_time", p.getValueAsLong(indexes[COLUMN_UTIME]));
         entry.put("command_line", p.getValue(indexes[COLUMN_CMDLINE]));
         list.add(entry);
      }
      return mapper.writeValueAsString(list);
   }
}
