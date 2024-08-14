package org.netxms.websvc.handlers;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;
import com.google.gson.Gson;
import com.google.gson.JsonObject;

public class FindMacAddress extends AbstractHandler
{

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {    
      NXCSession session = getSession();
      session.syncObjects();
      
      MacAddress macAddress = MacAddress.parseMacAddress(query.get("macAddress"), false);
      int searchLimit = 100;

      String limit = query.get("searchLimit");
      if (limit != null)
         searchLimit = Integer.parseInt(limit);
      

      boolean includeObjects = Boolean.parseBoolean(query.getOrDefault("includeObjects", "false"));
      
      final List<ConnectionPoint> cpList = session.findConnectionPoints(macAddress.getValue(), searchLimit);
      List<JsonObject> searchResults = new ArrayList<JsonObject>();
      Gson gson = JsonTools.createGsonInstance();
      for(ConnectionPoint cp : cpList)
      {
         JsonObject json = (JsonObject)gson.toJsonTree(cp);
         if (includeObjects)
         {
            AbstractObject object = session.findObjectById(cp.getNodeId());
            if (object != null)
            {
               json.add("node", gson.toJsonTree(object));
               if (session.isZoningEnabled() && (object instanceof Node))
               {
                  json.addProperty("nodesZoneUIN", ((Node)object).getZoneId());
                  json.addProperty("nodesZoneName", ((Node)object).getZoneName());
               }
            }
            object = session.findObjectById(cp.getInterfaceId());
            if (object != null)
            {
               json.add("interface", gson.toJsonTree(object));
            }
            object = session.findObjectById(cp.getLocalNodeId());
            if (object != null)
            {
               json.add("localNode", gson.toJsonTree(object));
               if (session.isZoningEnabled() && (object instanceof Node))
               {
                  json.addProperty("localNodeZoneUIN", ((Node)object).getZoneId());
                  json.addProperty("localNodeZoneName", ((Node)object).getZoneName());
               }
            }
            object = session.findObjectById(cp.getLocalInterfaceId());
            if (object != null)
            {
               json.add("localInterface", gson.toJsonTree(object));
            }
         }
         searchResults.add(json);
      }
      return new ResponseContainer("searchResults", searchResults);
   }

}
