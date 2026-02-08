/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.TimeFormatter;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.MapDCIInstance;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.MapLinkDataSource;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.nxmc.DisposableSingleton;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.tools.FontTools;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * DCI last value provider for map links
 */
public class LinkDciValueProvider implements DisposableSingleton
{
   private static Logger logger = LoggerFactory.getLogger(FontTools.class);
   
	private Set<MapDCIInstance> dciIDList = Collections.synchronizedSet(new HashSet<MapDCIInstance>());
	private Map<Long, DciValue> cachedDciValues = new HashMap<Long, DciValue>();
   private NXCSession session = Registry.getSession();
	private Thread syncThread = null;
	private volatile boolean syncRunning = true;

   /**
    * Create instance of map object syncer for given display and session.
    *
    * @param display owning display
    * @param session communication session
    */
   public static LinkDciValueProvider getInstance()
   {
      LinkDciValueProvider instance = Registry.getSingleton(LinkDciValueProvider.class);
      if (instance == null)
      {
         instance = new LinkDciValueProvider();
         Registry.setSingleton(LinkDciValueProvider.class, instance);
      }
      return instance;
   }

   /**
    * Constructor
    */
   public LinkDciValueProvider()
   {
      syncThread = new Thread(() -> syncLastValues(), "LinkDciValueProvider");
      syncThread.setDaemon(true);
      syncThread.start();
   }

	/**
	 * Synchronize last values in background
	 */
	private void syncLastValues()
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
			synchronized(cachedDciValues)
			{
			   synchronized(dciIDList)
	         {
					try
					{
                  if (dciIDList.size() > 0)
					   {
   						DciValue[] values = session.getLastValues(dciIDList); 
   						for(DciValue v : values)
   						   cachedDciValues.put(v.getId(), v);
					   }
					}
					catch(Exception e2)
					{
			         logger.error("Exception in sync last values thread - " + e2.getMessage(), e2);
					}
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
      logger.debug("LinkDciValueProvider thread stopped");
	}

	/**
	 * Get last DCI value for given DCI id
	 * 
	 * @param dciID
	 * @return
	 */
	public DciValue getDciLastValue(long dciID)
	{
		DciValue value;
		synchronized(cachedDciValues)
		{
			value = cachedDciValues.get(dciID);
		}
		return value;
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
	 * Adds DCI to value request list. Should be used for DCO_TYPE_ITEM
	 * 
	 * @param nodeID
	 * @param dciID
	 * @param mapPage
	 */
	public void addDci(long nodeID, long dciID, NetworkMapPage mapPage, int initialCount)
	{
	   synchronized(dciIDList)
      {
         boolean exists = false;
         for(MapDCIInstance item : dciIDList)
         {
            if(item.getDciID() == dciID)
            {
               item.addMap(mapPage.getMapObjectId(), initialCount);
               exists = true;
               break;
            }
         }
         if(!exists)
         {
            dciIDList.add(new MapDCIInstance(dciID, nodeID, DataCollectionItem.DCO_TYPE_ITEM, mapPage.getMapObjectId(), initialCount));
            syncThread.interrupt();
         }
      }
	}

	/**
	 * Adds DCI to value request list. Should be used for DCO_TYPE_TABLE
    * 
	 * @param nodeID
	 * @param dciID
	 * @param column
	 * @param instance
	 * @param mapPage
	 */
   public void addDci(long nodeID, long dciID, String column, String instance, NetworkMapPage mapPage, int initialCount)
   {
      synchronized(dciIDList)
      {
         boolean exists = false;
         for(MapDCIInstance item : dciIDList)
         {
            if(item.getDciID() == dciID)
            {
               item.addMap(mapPage.getMapObjectId(), initialCount);
               exists = true;
               break;
            }
         }
         if(!exists)
         {
            dciIDList.add(new MapDCIInstance(dciID, nodeID, column, instance, DataCollectionItem.DCO_TYPE_TABLE, mapPage.getMapObjectId(), initialCount));
            syncThread.interrupt();
         }
      }
   }

	/**
	 * Removes node from request list
	 * @param nodeID
	 */
	public void removeDcis(NetworkMapPage mapPage)
	{
	   synchronized(dciIDList)
      {
	      List<MapDCIInstance> forRemove = new ArrayList<MapDCIInstance>();
	      for(MapDCIInstance item : dciIDList)
	      {
            if (item.removeMap(mapPage.getMapObjectId()))
               forRemove.add(item);
	      }
	      for(MapDCIInstance item : forRemove)
	      {
            dciIDList.remove(item);
	      }
      }
	}

   /**
    * @param link
    * @return
    */
   public String getDciDataAsString(NetworkMapLink link, LinkDataLocation location)
   {
      if (!link.hasDciData())
         return "";

      MapLinkDataSource[] dciList =  link.getDciList();
      TimeFormatter timeFormatter = DateFormatFactory.getTimeFormatter();
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < dciList.length; i++)
      {
         if (dciList[i].getLocation() != location)
            continue;

         if (!sb.isEmpty())
            sb.append('\n');
         
         DciValue v = getDciLastValue(dciList[i].getDciId());
         if (v != null)
            sb.append(v.getFormattedValue(dciList[i].getFormatString(), timeFormatter));
      }
      return sb.toString();
   }

   /**
    * @param dciList
    * @return
    */
   public String getDciDataAsString(List<MapDataSource> dciList)
   {
      TimeFormatter timeFormatter = DateFormatFactory.getTimeFormatter();
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < dciList.size();)
      {
         DciValue v = getDciLastValue(dciList.get(i).getDciId()); 
         if (v != null)
            sb.append(v.getFormattedValue(dciList.get(i).getFormatString(), timeFormatter));
         if (++i != dciList.size())
            sb.append('\n');
      }
      return sb.toString();
   }

   /**
    * @param dciList
    * @return
    */
   public List<DciValue> getDciData(List<? extends MapDataSource> dciList)
   {
      List<DciValue> result = new ArrayList<DciValue>();
      for(int i = 0; i < dciList.size();i++)
      {
         DciValue value = getDciLastValue(dciList.get(i).getDciId());
         if (value != null)
            result.add(value); 
      }
      return result;
   }

   /**
    * @param dci
    * @return
    */
   public DciValue getLastDciData(MapDataSource dci)
   {
      return getDciLastValue(dci.getDciId()); 
   }
}
