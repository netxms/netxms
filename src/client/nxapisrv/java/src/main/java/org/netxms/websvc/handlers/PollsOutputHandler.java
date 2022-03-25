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
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
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
