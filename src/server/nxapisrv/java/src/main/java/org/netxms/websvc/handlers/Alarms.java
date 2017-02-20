/**
 * 
 */
package org.netxms.websvc.handlers;

import java.util.Map;
import org.netxms.client.events.Alarm;
import org.netxms.websvc.json.ResponseContainer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Handler for alarm management
 */
public class Alarms extends AbstractHandler
{
   private Logger log = LoggerFactory.getLogger(Alarms.class);

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(org.json.JSONObject)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      Map<Long, Alarm> alarms = getSession().getAlarms();
      return new ResponseContainer("alarms", alarms.values());
   }
}
