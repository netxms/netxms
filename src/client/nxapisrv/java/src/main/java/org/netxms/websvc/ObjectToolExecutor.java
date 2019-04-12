package org.netxms.websvc;

import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.util.Map;

public class ObjectToolExecutor extends Thread
{
   private Logger log = LoggerFactory.getLogger(ObjectToolExecutor.class);
   private ObjectToolDetails details;
   private long objectId;
   private Map<String, String> inputFields;
   private ObjectToolOutputListener listener;
   private NXCSession session;

   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields,
         ObjectToolOutputListener listener, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.listener = listener;
      this.session = session;
      setDaemon(true);
      start();
   }

   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.listener = null;
      this.session = session;
      setDaemon(true);
      start();
   }

   /* (non-Javadoc)
    * @see java.lang.Thread#run()
    */
   @Override public void run()
   {
      switch(details.getToolType())
      {
         case ObjectTool.TYPE_ACTION:
            executeAgentAction();
         /*case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool, inputValues);
            break;*/
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand();
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
   }

   /**
    * Execute agent action
    */
   private void executeAgentAction()
   {
      if ((details.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         try
         {
            final String s = session.executeActionWithExpansion(objectId, 0, details.getData(), inputFields);
         }
         catch(Exception e)
         {
            log.error(e.getLocalizedMessage());
         }
      }
      else
      {
         try
         {
            final String s = session.executeActionWithExpansion(objectId, 0, details.getData(), true, inputFields, listener, null);
         }
         catch(Exception e)
         {
            log.error(e.getLocalizedMessage());
         }
      }
   }

   /**
    * Execute server command
    */
   private void executeServerCommand()
   {
      if ((details.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         try
         {
            session.executeServerCommand(objectId, details.getData(), inputFields);
         }
         catch(Exception e)
         {
            log.error(e.getLocalizedMessage());
         }
      }
      else
      {
         try
         {
            session.executeServerCommand(objectId, details.getData(), inputFields, true, listener, null);
         }
         catch(Exception e)
         {
            log.error(e.getLocalizedMessage());
         }
         listener.onComplete();
      }
   }
}
