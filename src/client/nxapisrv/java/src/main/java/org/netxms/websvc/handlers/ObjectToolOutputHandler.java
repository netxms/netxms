package org.netxms.websvc.handlers;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.ObjectToolOutputListener;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Get;
import org.restlet.resource.Post;

public class ObjectToolOutputHandler extends AbstractHandler
{
   private static Map<UUID, ObjectToolOutputListener> listenerMap = new HashMap<>();

   @Get
   public Representation onGet() throws Exception
   {
      if (!attachToSession())
         return createErrorResponseRepresentation(RCC.ACCESS_DENIED);

      String id = getEntityId();
      ObjectToolOutputListener listener = listenerMap.get(UUID.fromString(id));
      if (listener != null)
      {
         String msg = listener.readOutput();
         JSONObject response = new JSONObject();
         response.put("message", msg);
         response.put("streamId", listener.getStreamId());
         response.put("completed", listener.isCompleted());
         return new StringRepresentation(response.toString(), MediaType.APPLICATION_JSON);
      }
      return createErrorResponseRepresentation(RCC.ACCESS_DENIED);
   }

   public static void addListener(UUID uuid, ObjectToolOutputListener listener)
   {
      listenerMap.put(uuid, listener);
   }

   public static void removeListener(UUID uuid)
   {
      listenerMap.remove(uuid);
   }

   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      if (entity == null || !attachToSession())
      {
         return createErrorResponseRepresentation(RCC.ACCESS_DENIED);
      }

      JSONObject json = new JsonRepresentation(entity).getJsonObject();
      if (json.has("streamId") && json.has("uuid"))
      {
         long streamId = json.getLong("streamId");
         UUID uuid = UUID.fromString(json.getString("uuid"));
         ObjectToolOutputHandler.removeListener(uuid);

         NXCSession session = getSession();
         session.stopServerCommand(streamId);
         return createErrorResponseRepresentation(RCC.SUCCESS);
      }
      else
         return new StringRepresentation(createErrorResponse(RCC.INTERNAL_ERROR).toString(), MediaType.APPLICATION_JSON);
   }
}
