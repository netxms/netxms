/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.client;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.netxms.base.MacAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * OUI to vendor mapping cache
 */
final class OUICache
{
   private static Logger logger = LoggerFactory.getLogger(OUICache.class);

   private HashMap<MacAddress, String> ouiToVendorMap;
   private Set<MacAddress> syncList;
   private List<Runnable> callbackList;
   private NXCSession session;

   /**
    * Constructor
    * 
    * @param session NXCP session 
    */
   public OUICache(NXCSession session)
   {
      ouiToVendorMap = new HashMap<MacAddress, String>();
      syncList = new HashSet<MacAddress>();
      callbackList = new ArrayList<Runnable>();
      this.session = session;
      new BackgroundSync();
   }

   /**
    * Get MAC from list or request missing form server and refresh once received
    *
    * @param mac MAC address to search
    * @param callback callback to call when result comes form server
    * 
    * @return vendor name or null
    */
   public String getVendor(MacAddress mac, Runnable callback)
   {
      if (mac == null || mac.isNull())
         return null;

      String name = null;
      boolean needSync = false;
      synchronized(ouiToVendorMap)
      {
         MacAddress macPart = new MacAddress(mac.getValue(), 3);   
         name = ouiToVendorMap.get(macPart);  

         if (name == null)
         {
            byte[] oui28 = new byte[4];
            for (int i = 0; i < 4; i++)
               oui28[i] = mac.getValue()[i];
            oui28[3] &= 0xF0;

            macPart = new MacAddress(oui28);   
            name = ouiToVendorMap.get(macPart);            
         }
         if (name == null)
         {
            byte[] oui32 = new byte[5];
            for (int i = 0; i < 5; i++)
               oui32[i] = mac.getValue()[i];
            oui32[4] &= 0xF0;

            macPart = new MacAddress(oui32);   
            name = ouiToVendorMap.get(macPart);                
         }
         if (name == null)
            needSync = true;
      }
      if (needSync)
      {
         synchronized(syncList)
         {
            syncList.add(mac);
            if (callback != null)
               callbackList.add(callback);
            syncList.notifyAll();
         }
      }
      return name;    
   }

   /**
    * OUI database synchronization thread
    */
   class BackgroundSync extends Thread
   {
      public BackgroundSync()
      {
         setName("BackgroundOUISync");
         setDaemon(true);
         start();
      }

      /**
       * @see java.lang.Thread#run()
       */
      @Override
      public void run()
      {
         while(true)
         {
            synchronized(syncList)
            {
               while(syncList.isEmpty())
               {
                  try
                  {
                     syncList.wait();
                  }
                  catch(InterruptedException e)
                  {
                  }
               }

            }

            // Delay actual sync in case more synchronization requests will come
            try
            {
               Thread.sleep(200);
            }
            catch(InterruptedException e)
            {
            }

            Set<MacAddress> syncListCopy;
            List<Runnable> callbackListCopy;
            synchronized(syncList)
            {
               syncListCopy = syncList;
               syncList = new HashSet<MacAddress>();
               callbackListCopy = callbackList;
               callbackList = new ArrayList<Runnable>();
            }

            try
            {
               Map<MacAddress, String> updatedElements = session.getVendorByMac(syncListCopy);
               synchronized(ouiToVendorMap)
               {
                  ouiToVendorMap.putAll(updatedElements);
               }
               for(Runnable cb : callbackListCopy)
                  cb.run();
            }
            catch(Exception e)
            {
               logger.error("Exception while synchronizing OUI cache", e);
            }
         }         
      }
   }
}
