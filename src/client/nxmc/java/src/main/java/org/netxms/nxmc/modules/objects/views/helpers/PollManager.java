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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.HashMap;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Poll manager 
 */
public class PollManager
{
   private HashMap<String, PollerProxy> pollersMap = new HashMap<String, PollerProxy>();
   
   /**
    * Start new poll or connect to existing 
    * 
    * @param target polling target
    * @param pollType poll type
    * @param listener new listener
    */
   public void startNewPoll(PollingTarget target, ObjectPollType pollType, TextOutputListener listener)
   {
      synchronized(pollersMap)
      {
         String key = String.format("%d_%d", target.getObjectId(), pollType.getValue());
         PollerProxy proxy = pollersMap.get(key);
         if (proxy != null)
         {
            proxy.addListener(listener, true);
            proxy.startPoll();
         }
         else
         {
            pollersMap.put(key, new PollerProxy(target, pollType, listener)); 
         }
      }    
   }
   
   /**
    * Follow current poll by another listener if poll exists
    * 
    * @param target polling target
    * @param pollType poll type
    * @param listener new listener
    */
   public void followPoll(PollingTarget target, ObjectPollType pollType, TextOutputListener listener)
   {
      synchronized(pollersMap)
      {
         String key = String.format("%d_%d", target.getObjectId(), pollType.getValue());
         PollerProxy proxy = pollersMap.get(key);
         if (proxy != null)
         {
            proxy.addListener(listener, false);
         }
      }
   }
   
   
   /**
    * Remove polling listener 
    * 
    * @param target polling target
    * @param pollType poll type
    * @param listener listener to remove
    */
   public void removePollListener(PollingTarget target, ObjectPollType pollType, TextOutputListener listener)
   {
      synchronized(pollersMap)
      {
         String key = String.format("%d_%d", target.getObjectId(), pollType.getValue());
         PollerProxy proxy = pollersMap.get(key);
         if (proxy != null)
         {
            if(proxy.removeListenerAndCheck(listener))
               pollersMap.remove(key);
         }
      }      
   }

}
