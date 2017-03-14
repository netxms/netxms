package org.netxms.websvc.handlers;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Map;
import org.json.JSONException;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

public class AlarmsGrafana extends AbstractHandler
{
   private String[] states = { "Outstanding", "Acknowledged", "Resolved", "Terminated" };
   private Logger log = LoggerFactory.getLogger(AlarmsGrafana.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   public Object getCollection(Map<String, String> query) throws Exception
   {
      JsonObject root = new JsonObject();

      JsonArray columns = new JsonArray();
      columns.add(createColumn("Severity", true, true));
      columns.add(createColumn("State", true, false));
      columns.add(createColumn("Source", true, false));
      columns.add(createColumn("Message", true, false));
      columns.add(createColumn("Count", true, false));
      //columns.add(createColumn("Comments", true, false));
      columns.add(createColumn("Helpdesk ID", true, false));
      columns.add(createColumn("Ack/Resolved By", true, false));
      columns.add(createColumn("Created", true, false));
      columns.add(createColumn("Last Change", true, false));
      root.add("columns", columns);
      log.error(root.toString());
      
      JsonArray rows = new JsonArray();
      
      JsonArray r = new JsonArray();
      DateFormat df = new SimpleDateFormat("dd.MM.yyyy HH:mm:ss");
      AbstractObject object = null;
      AbstractUserObject user = null;

      NXCSession session = getSession();
      if (!session.isObjectsSynchronized())
         session.syncObjects();
      
      Map<Long, Alarm> alarms = session.getAlarms();
      for( Alarm a : alarms.values())
      {
         r.add("<td style=\"background:rgb(0, 192, 0)\">" + a.getCurrentSeverity().name() + "</td>");
         r.add(states[a.getState()]);
         object = getSession().findObjectById(a.getSourceObjectId());
         if (object == null)
            r.add(a.getSourceObjectId());
         else
            r.add(object.getObjectName());
         r.add(a.getMessage());
         r.add(a.getRepeatCount());
         //r.add(a.getCommentsCount());
         r.add(a.getHelpdeskReference());
         user = getSession().findUserDBObjectById(a.getAckByUser());
         if (user == null)
            r.add("");
         else
            r.add(user.getName());
         r.add(df.format(a.getCreationTime()));
         r.add(df.format(a.getLastChangeTime()));
         rows.add(r);
         r = new JsonArray();
      }
      
      root.add("rows", rows);
      
      root.addProperty("type", "table");

      JsonArray wrapper = new JsonArray();
      wrapper.add(root);
      log.error(wrapper.toString());
      return wrapper;
   }
   
   /**
    * @param name
    * @param sort
    * @param desc
    * @return
    * @throws JSONException
    */
   private static JsonObject createColumn(String name, boolean sort, boolean desc) throws JSONException
   {
      JsonObject column = new JsonObject();
      column.addProperty("text", name);
      column.addProperty("sort", sort);
      column.addProperty("sort", desc);
      return column;
   }
}
