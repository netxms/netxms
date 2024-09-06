/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.DisposableSingleton;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.tools.FontTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Object synchronization helper for network maps
 */
public class MapObjectSyncer implements DisposableSingleton
{
   private static Logger logger = LoggerFactory.getLogger(FontTools.class);

   private Map<Long, Set<Long>> objectIdToMap = new HashMap<Long, Set<Long>>();
   private Map<Long, Set<Long>> requestData = new HashMap<Long, Set<Long>>();
   private Map<Long, Integer> mapReferenceCount = new HashMap<Long, Integer>();
   private NXCSession session = Registry.getSession();
	private Thread syncThread = null;
	private volatile boolean syncRunning = true;

   /**
    * Create instance of map object syncer for given display and session.
    *
    * @param display owning display
    * @param session communication session
    */
   public static MapObjectSyncer getInstance()
   {
      MapObjectSyncer instance = Registry.getSingleton(MapObjectSyncer.class);
      if (instance == null)
      {
         instance = new MapObjectSyncer();
         Registry.setSingleton(MapObjectSyncer.class, instance);
      }
      return instance;
   }

   /**
    * Constructor
    * @param session 
    */
   private MapObjectSyncer()
   {
      syncThread = new Thread(() -> syncObjects(), "MapObjectSyncer");
      syncThread.setDaemon(true);
      syncThread.start();
   }

	/**
	 * Synchronize last values in background
	 */
	private void syncObjects()
	{
		try
		{
			Thread.sleep(1000);
		}
		catch(InterruptedException e3)
		{
		}
		while(syncRunning)
		{
			synchronized(requestData)
			{
				try
				{
               if (requestData.size() > 0)
				   {
                  for (Entry<Long, Set<Long>> entry : requestData.entrySet())
                  { 
                     session.syncObjectSet(entry.getValue(), entry.getKey(), NXCSession.OBJECT_SYNC_NOTIFY | NXCSession.OBJECT_SYNC_ALLOW_PARTIAL);
                  }
				   }
				}
				catch(Exception e2)
				{
               logger.error("Exception in sync last objects thread - " + e2.getMessage(), e2);
				}
			}
			try
			{
				Thread.sleep(30000); 
			}
			catch(InterruptedException e1)
			{
			}
		}
      session = null;
      logger.debug("MapObjectSyncer thread stopped");
	}

   /**
    * @see org.netxms.nxmc.DisposableSingleton#dispose()
    */
   @Override
	public void dispose()
	{
		syncRunning = false;
		syncThread.interrupt();
	}

   /**
    * Add objects for synchronization.
    *
    * @param mapId map object ID
    * @param mapObjectIds objects on map to synchronize
    * @param utilizationInterfaces interface objects used for displaying utilization information
    */
	public void addObjects(long mapId, final Set<Long> mapObjectIds, Set<Long> utilizationInterfaces)
	{	   
	   Set<Long> uniqueObjects = new HashSet<Long>(); // Objects not referenced already by any other map
	   synchronized(requestData)
      {
	      Integer count = mapReferenceCount.get(mapId);
	      if (count != null)
	      {
	         mapReferenceCount.put(mapId, count + 1);
	         return;
	      }
	      mapReferenceCount.put(mapId, 1); 

         for(Long objectId : mapObjectIds)
         {
            AbstractObject object = session.findObjectById(objectId, true);
            if ((object == null) || (!object.isPartialObject() && !utilizationInterfaces.contains(objectId)))
            {
               continue;
            }

            Set<Long> objectMaps = objectIdToMap.get(objectId); 
            if (objectMaps == null || objectMaps.isEmpty())
            {
               objectMaps = new HashSet<Long>();
               objectIdToMap.put(objectId, objectMaps);
               uniqueObjects.add(objectId);
            }
            objectMaps.add(mapId);
         }
         
         if (uniqueObjects.size() > 0)
         {
            requestData.put(mapId, uniqueObjects);
         }
      }
	}

   /**
    * Remove objects from synchronization.
    *
    * @param mapId map object ID
    * @param mapObjectIds set of objects to remove
    */
   public void removeObjects(long mapId, final Set<Long> mapObjectIds)
   {
      synchronized(requestData)
      {
         Integer count = mapReferenceCount.get(mapId);
         if (count == null)
            return;  // Map was never added
         if (count > 1)
         {
            mapReferenceCount.put(mapId, count - 1);
            return;  // Map still referenced 
         }
         mapReferenceCount.remove(mapId);    

         if (mapReferenceCount.isEmpty())
         {
            objectIdToMap.clear();
            requestData.clear();
            return;  // no more open maps
         }

         Set<Long> mapSpecificObjects = requestData.remove(mapId);   // All objects synchronized by this map, can be null if no requests done on behalf of this map
         for(Long objectId : mapObjectIds)
         {
            Set<Long> maps = objectIdToMap.get(objectId);
            if (maps == null)
               continue;   // can be null if object is not partially synchronized
            
            maps.remove(mapId);
            if (!maps.isEmpty() && (mapSpecificObjects != null) && mapSpecificObjects.contains(objectId))
            {
               boolean added = false;
               // Attempt to add this object to other existing request
               for (Long otherMapId : maps)
               {
                  Set<Long> otherMapObjects = requestData.get(otherMapId);
                  if (otherMapObjects != null)
                  {
                     otherMapObjects.add(objectId);
                     added = true;
                     break;
                  }
               }
               if (!added)
               {
                  // No other map references this object in request
                  HashSet<Long> newSet = new HashSet<Long>();
                  newSet.add(objectId);
                  requestData.put(maps.iterator().next(), newSet);                
               }
            }
         }
      }
   }
}
