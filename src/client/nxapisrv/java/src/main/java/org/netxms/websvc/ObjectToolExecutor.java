/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
package org.netxms.websvc;

import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Object tool execution thread
 */
public class ObjectToolExecutor extends Thread
{
   private Logger log = LoggerFactory.getLogger(ObjectToolExecutor.class);
   private ObjectToolDetails details;
   private long objectId;
   private Map<String, String> inputFields;
   private List<String> maskedFields;
   private ServerOutputListener listener;
   private NXCSession session;
   private boolean generateOutput;

   /**
    * Create and start new object tool execution thread
    * 
    * @param details
    * @param objectId
    * @param inputFields
    * @param listener
    * @param session
    */
   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields,
         List<String> maskedFields, ServerOutputListener listener, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.maskedFields = maskedFields;
      this.listener = listener;
      this.session = session;
      generateOutput = (details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0;
      setDaemon(true);
      start();
   }

   /**
    * Create and start new object tool execution thread
    * 
    * @param details
    * @param objectId
    * @param inputFields
    * @param session
    */
   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields, List<String> maskedFields, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.maskedFields = maskedFields;
      this.listener = null;
      this.session = session;
      generateOutput = (details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0;
      setDaemon(true);
      start();
   }

   /**
    * @see java.lang.Thread#run()
    */
   @Override
   public void run()
   {
      try
      {
         switch(details.getToolType())
         {
            case ObjectTool.TYPE_ACTION:
               session.executeActionWithExpansion(objectId, 0, details.getData(), generateOutput, inputFields, maskedFields, listener, null);
               break;
            /*case ObjectTool.TYPE_FILE_DOWNLOAD:
               executeFileDownload(node, tool, inputValues);
               break;*/
            case ObjectTool.TYPE_SERVER_COMMAND:
               session.executeServerCommand(objectId, 0, details.getData(), inputFields, maskedFields, generateOutput, listener, null);
               break;
            /*case ObjectTool.TYPE_SERVER_SCRIPT:
               executeServerScript(node, tool, inputValues);
               break;
            case ObjectTool.TYPE_TABLE_AGENT:
            case ObjectTool.TYPE_TABLE_SNMP:
               executeTableTool(node, tool);
               break;
            case ObjectTool.TYPE_URL:
               openURL(node, tool, inputValues, expandedValue);
               break;*/
         }

         if (generateOutput && (listener != null))
         {
            listener.onSuccess();
         }
      }
      catch(Exception e)
      {
         log.error("Object tool execution error", e);
         if (generateOutput && (listener != null))
         {
            listener.onFailure(e);
         }
      }
   }
}
