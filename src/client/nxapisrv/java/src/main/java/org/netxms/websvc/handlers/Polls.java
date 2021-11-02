/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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

import java.util.UUID;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.NodePollType;
import org.netxms.websvc.ServerOutputListener;
import org.netxms.websvc.json.JsonTools;

/**
 * Handler for object tool management
 */
public class Polls extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      String type = JsonTools.getStringFromJson(data, "type", "");
      
      NodePollType pollType = NodePollType.UNKNOWN;
      if (type.equalsIgnoreCase("configuration full"))
      {
         pollType = NodePollType.CONFIGURATION_FULL;
      }
      else if (type.equals("configuration"))
      {
         pollType = NodePollType.CONFIGURATION_NORMAL;
      }
      else if (type.equals("discovery"))
      {
         pollType = NodePollType.INSTANCE_DISCOVERY;
      }
      else if (type.equals("interface"))
      {
         pollType = NodePollType.INTERFACES;
      }
      else if (type.equals("status"))
      {
         pollType = NodePollType.STATUS;
      }
      else if (type.equals("topology"))
      {
         pollType = NodePollType.TOPOLOGY;
      }
      
      final ServerOutputListener listener = new ServerOutputListener();
      UUID uuid = UUID.randomUUID();
      PollsOutputHandler.addListener(uuid, listener);
      
      final NodePollType tmpType = pollType;
      NXCSession session = getSession();
      Thread pollingThread = new Thread(() -> {
         try
         {
            session.pollObject(getObjectId(), tmpType, listener);
         }
         catch(Exception e)
         {
            listener.onError();
         }
         finally
         {
            listener.onComplete();
         }
      });
      pollingThread.setDaemon(true);
      pollingThread.start();
      
      JSONObject response = new JSONObject();
      response.put("UUID", uuid);
      response.put("type", type);
      return response;
   }  
   
   
}
