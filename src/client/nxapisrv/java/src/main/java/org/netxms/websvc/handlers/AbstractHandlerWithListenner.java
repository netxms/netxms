package org.netxms.websvc.handlers;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.netxms.websvc.ServerOutputListener;

/**
 * Abstract handler class with option to save listeners  
 */
public class AbstractHandlerWithListenner extends AbstractHandler
{
   protected static Map<UUID, ServerOutputListener> listenerMap = new HashMap<>();

   public static void addListener(UUID uuid, ServerOutputListener listener)
   {
      listenerMap.put(uuid, listener);
   }

   public static void removeListener(UUID uuid)
   {
      listenerMap.remove(uuid);
   }
}
