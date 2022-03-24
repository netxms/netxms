package org.netxms.websvc.handlers;

import java.util.Map;
import java.util.UUID;
import org.json.JSONObject;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.ServerOutputListener;

/**
 * Poll output handler class
 */
public class PollsOutputHandler extends AbstractHandlerWithListenner
{
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      ServerOutputListener listener = listenerMap.get(UUID.fromString(id));
      if (listener != null)
      {
         String msg = listener.readOutput();
         JSONObject response = new JSONObject();
         response.put("message", msg);
         response.put("streamId", listener.getStreamId());
         response.put("completed", listener.isCompleted());
         
         if (listener.isCompleted()) //delete listener on complete response sent
         {
            listenerMap.remove(UUID.fromString(id));
         }
         
         return response;
      }
      return createErrorResponseRepresentation(RCC.ACCESS_DENIED);
   }
}
