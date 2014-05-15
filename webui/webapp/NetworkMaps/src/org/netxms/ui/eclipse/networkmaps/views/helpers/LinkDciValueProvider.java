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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.service.UISessionEvent;
import org.eclipse.rap.rwt.service.UISessionListener;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.MapDCIInstance;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * DCI last value provider for map links
 */
public class LinkDciValueProvider
{
	private Set<MapDCIInstance> dciIDList = Collections.synchronizedSet(new HashSet<MapDCIInstance>());
	private Map<Long, DciValue> cachedDciValues = new HashMap<Long, DciValue>();
	private NXCSession session = null;
	private Thread syncThread = null;
	private volatile boolean syncRunning = true;
	
   /**
    * Get instance
    */
   public static LinkDciValueProvider getInstance()
	{
      LinkDciValueProvider instance = (LinkDciValueProvider)RWT.getUISession().getAttribute("LinkDciValueProvider.instance");
	   if(instance == null)
	   {
	      instance = new LinkDciValueProvider();
         RWT.getUISession().setAttribute("LinkDciValueProvider.instance", instance);
         RWT.getUISession().addUISessionListener(new UISessionListener() {
            @Override
            public void beforeDestroy(UISessionEvent event)
            {
               LinkDciValueProvider instance = (LinkDciValueProvider)RWT.getUISession().getAttribute(
                     "LinkDciValueProvider.instance");
               if (instance != null)
                  instance.dispose();
            }
         });
	   }
	   
      return instance;
	}
	
	  /**
    * Constructor
    */
   public LinkDciValueProvider()
   {
      this.session = (NXCSession)ConsoleSharedData.getSession();
      syncThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            syncLastValues();
         }
      });
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
					   if(dciIDList.size() > 0)
					   {
   						DciValue[] values = session.getLastValues(dciIDList); 
   						for(DciValue v : values)
   						   cachedDciValues.put(v.getId(), v);
					   }
					}
					catch(Exception e2)
					{
						e2.printStackTrace();	// for debug
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
	 * 
	 */
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
	public void addDci(long nodeID, long dciID, NetworkMapPage mapPage)
	{
	   synchronized(dciIDList)
      {
         boolean exists = false;
         for(MapDCIInstance item : dciIDList)
         {
            if(item.getDciID() == dciID)
            {
               item.addMap(mapPage.getId());
               exists = true;
               break;
            }
         }
         if(!exists)
         {
            dciIDList.add(new MapDCIInstance(dciID, nodeID, DataCollectionItem.DCO_TYPE_ITEM, mapPage.getId()));
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
   public void addDci(long nodeID, long dciID, String column, String instance, NetworkMapPage mapPage)
   {
      synchronized(dciIDList)
      {
         boolean exists = false;
         for(MapDCIInstance item : dciIDList)
         {
            if(item.getDciID() == dciID)
            {
               item.addMap(mapPage.getId());
               exists = true;
               break;
            }
         }
         if(!exists)
         {
            dciIDList.add(new MapDCIInstance(dciID, nodeID, column, instance, DataCollectionItem.DCO_TYPE_TABLE, mapPage.getId()));
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
            if(item.removeMap(mapPage.getId()))
               forRemove.add(item);
	      }
	      for(MapDCIInstance item : forRemove)
	      {
            dciIDList.remove(item);
	      }
      }
	}

	
   public String getDciDataAsString(NetworkMapLink link)
   {
      if (!link.hasDciData())
         return "";
      String result = "";
      SingleDciConfig[] dciList =  link.getDciList();
      for(int i = 0; i < dciList.length;)
      {
         DciValue v = getDciLastValue(dciList[i].dciId); 
         if(v != null)
         {
            String formatString = dciList[i].getFormatString();
            result += v.format(formatString.isEmpty() ? "%s" : formatString);
         }
         if(++i != dciList.length)
            result += "\n";
      }
      return result;
   }
   
   public String getDciDataAsString(List<SingleDciConfig> DCIList)
   {
      String result = "";
      for(int i = 0; i < DCIList.size();)
      {
         DciValue v = getDciLastValue(DCIList.get(i).dciId); 
         if(v != null)
         {
            String formatString = DCIList.get(i).getFormatString();
            result += v.format(formatString.isEmpty() ? "%s" : formatString);
         }
         if(++i != DCIList.size())
            result += "\n";
      }
      return result;
   }
   
   public DciValue getLastDciData(SingleDciConfig dci)
   {
      return getDciLastValue(dci.dciId); 
   }
}
