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
package org.netxms.client.services;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.ServiceLoader;
import java.util.Set;
import org.netxms.client.ModuleDataProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manager for client services
 */
public final class ServiceManager
{
   private static final Logger logger = LoggerFactory.getLogger(ServiceManager.class);
   private static final Map<ClassLoader, ServiceLoader<ServiceHandler>> serviceLoaders = new HashMap<ClassLoader, ServiceLoader<ServiceHandler>>();
   private static final Map<String, ModuleDataProvider> dataProviders = new HashMap<>();
   private static final Set<String> missingDataProviders = new HashSet<>();

   /**
    * Reload service providers
    * 
    * @param classLoader class loader to use
    */
   public static synchronized void registerClassLoader(ClassLoader classLoader)
   {
      ServiceLoader<ServiceHandler> serviceLoader = serviceLoaders.get(classLoader);
      if (serviceLoader != null)
      {
         serviceLoader.reload();
      }
      else
      {
         serviceLoaders.put(classLoader, ServiceLoader.load(ServiceHandler.class, classLoader));
      }
   }

   /**
    * Get module data provider for given module.
    * 
    * @param name module name
    * @return module data provider for given module or null
    */
   public static synchronized ModuleDataProvider getModuleDataProvider(String name)
   {
      if (missingDataProviders.contains(name))
         return null;

      ModuleDataProvider p = dataProviders.get(name);
      if (p != null)
         return p;

      for(ServiceLoader<ServiceHandler> loader : serviceLoaders.values())
      {
         for(ServiceHandler s : loader)
            if (s.getServiceName().equals(name) && (s instanceof ModuleDataProvider))
            {
               dataProviders.put(name, (ModuleDataProvider)s);
               return (ModuleDataProvider)s;
            }
      }

      logger.warn("Unable to find data provider for module " + name);
      missingDataProviders.add(name);
      return null;
   }

   /**
    * Get service handler by name
    * 
    * @param name service name
    * @return service handler for given service
    */
   public static synchronized ServiceHandler getServiceHandler(String name)
   {
      for(ServiceLoader<ServiceHandler> loader : serviceLoaders.values())
      {
         for(ServiceHandler s : loader)
            if (s.getServiceName().equals(name))
               return s;
      }
      return null;
   }

   /**
    * Get service handler by class
    * 
    * @param serviceClass service handler class
    * @return service handler for given service
    */
   public static synchronized ServiceHandler getServiceHandler(Class<? extends ServiceHandler> serviceClass)
   {
      for(ServiceLoader<ServiceHandler> loader : serviceLoaders.values())
      {
         for(ServiceHandler s : loader)
            if (serviceClass.isInstance(s))
               return s;
      }
      return null;
   }

   /**
    * Debug method to dump all registered services
    */
   public static synchronized void dump()
   {
      for(ServiceLoader<ServiceHandler> loader : serviceLoaders.values())
      {
         for(ServiceHandler s : loader)
            System.out.println(s.getServiceName());
      }
   }
}
