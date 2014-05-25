/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.api.client.services;

import java.util.HashMap;
import java.util.Map;

/**
 * Manager for client services
 */
public final class ServiceManager
{
   private static Map<String, ServiceHandler> servicesByName = new HashMap<String, ServiceHandler>();
   private static Map<Class<? extends ServiceHandler>, ServiceHandler> servicesByClass = new HashMap<Class<? extends ServiceHandler>, ServiceHandler>();
   
   /**
    * Register service
    * 
    * @param name
    * @param handler
    * @return
    */
   public static synchronized boolean registerService(ServiceHandler handler)
   {
      if (servicesByName.containsKey(handler.getServiceName()) || servicesByClass.containsKey(handler.getClass()))
         return false;
      servicesByName.put(handler.getServiceName(), handler);
      servicesByClass.put(handler.getClass(), handler);
      return true;
   }
   
   /**
    * Unregister service
    * 
    * @param name
    */
   public static synchronized void unregisterService(String name)
   {
      ServiceHandler h = servicesByName.get(name);
      if (h != null)
      {
         servicesByName.remove(name);
         servicesByClass.remove(h.getClass());
      }
   }

   /**
    * Get service handler by name and check if handler class is correct.
    * 
    * @param name
    * @param serviceClass
    * @return
    */
   public static synchronized ServiceHandler getServiceHandler(String name, Class<? extends ServiceHandler> serviceClass)
   {
      ServiceHandler h = servicesByName.get(name);
      return serviceClass.isInstance(h) ? h : null;
   }
   
   /**
    * Get service handler by name
    * 
    * @param name
    * @return
    */
   public static synchronized ServiceHandler getServiceHandler(String name)
   {
      return servicesByName.get(name);
   }
   
   /**
    * Get service handler by class
    * 
    * @param serviceClass
    * @return
    */
   public static synchronized ServiceHandler getServiceHandler(Class<? extends ServiceHandler> serviceClass)
   {
      return servicesByClass.get(serviceClass);
   }
}
