package org.netxms.websvc.handlers;

import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;
import org.netxms.websvc.ServerOutputListener;

/**
 * Abstract handler class with option to save listeners  
 */
public class AbstractHandlerWithListenner extends AbstractHandler
{
   private static final long RENENTION_TIME = 600000L; //10 min
   protected static Map<UUID, ServerOutputListener> listenerMap = new HashMap<>();
   
   public AbstractHandlerWithListenner()
   {
      Thread clearOldPollsThread = new Thread(() -> {         
         while(true)
         {
            try
            {
               Thread.sleep(60000); // 1 min
            }
            catch(InterruptedException e)
            {
            }
            
            long now = new Date().getTime();
            Iterator<ServerOutputListener> it = listenerMap.values().iterator();
            while(it.hasNext())
            {
               ServerOutputListener value = it.next();
               if ((now - value.getLastUpdateTime().getTime()) > RENENTION_TIME) 
               {
                  it.remove();
               }
               
            }
         }
      }, "ClearOldPollsThread"); //$NON-NLS-1$
      clearOldPollsThread.setDaemon(true);
      clearOldPollsThread.start();
   }

   public static void addListener(UUID uuid, ServerOutputListener listener)
   {
      listenerMap.put(uuid, listener);
   }

   public static void removeListener(UUID uuid)
   {
      listenerMap.remove(uuid);
   }
}
