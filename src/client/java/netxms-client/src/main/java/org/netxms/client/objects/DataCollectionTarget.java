/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.GeoLocationControlMode;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.interfaces.Assets;

/**
 * Base class for all data collection targets
 */
public class DataCollectionTarget extends GenericObject implements Assets
{   
   public static final int DCF_DISABLE_STATUS_POLL    = 0x00000001;
   public static final int DCF_DISABLE_CONF_POLL      = 0x00000002;
   public static final int DCF_DISABLE_DATA_COLLECT   = 0x00000004;
   public static final int DCF_LOCATION_CHANGE_EVENT  = 0x00000008;

   public static final int DCSF_UNREACHABLE          = 0x000000001;
   public static final int DCSF_NETWORK_PATH_PROBLEM = 0x000000002;

   protected List<DciValue> overviewDciData;
   protected List<DciValue> tooltipDciData;
   protected int numDataCollectionItems;
   protected GeoLocationControlMode geoLocationControlMode;
   protected long[] geoAreas;
   protected long webServiceProxyId;
   private Map<String, String> assetList;

   /**
    * Create new object.
    * 
    * @param id object ID
    * @param session client session where this object was received
    */
   public DataCollectionTarget(long id, NXCSession session)
   {
      super(id, session);
      numDataCollectionItems = 0;
      overviewDciData = new ArrayList<DciValue>(0);
      tooltipDciData = new ArrayList<DciValue>(0);
      geoLocationControlMode = GeoLocationControlMode.NO_CONTROL;
      geoAreas = new long[0];
      webServiceProxyId = 0;
   }

   /**
    * Create object from NXCP message.
    * 
    * @param msg NXCP message
    * @param session client session where this object was received
    */
   public DataCollectionTarget(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      
      int count = msg.getFieldAsInt32(NXCPCodes.VID_OVERVIEW_DCI_COUNT);
      overviewDciData = new ArrayList<DciValue>(count);
      long fieldId = NXCPCodes.VID_OVERVIEW_DCI_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         overviewDciData.add(DciValue.createFromMessage(msg, fieldId));
         fieldId += 50;
      }

      count = msg.getFieldAsInt32(NXCPCodes.VID_TOOLTIP_DCI_COUNT);
      tooltipDciData = new ArrayList<DciValue>(count);
      fieldId = NXCPCodes.VID_TOOLTIP_DCI_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         tooltipDciData.add(DciValue.createFromMessage(msg, fieldId));
         fieldId += 50;
      }

      numDataCollectionItems = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      geoLocationControlMode = GeoLocationControlMode.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_GEOLOCATION_CTRL_MODE));
      geoAreas = msg.getFieldAsUInt32Array(NXCPCodes.VID_GEO_AREAS);
      webServiceProxyId = msg.getFieldAsInt64(NXCPCodes.VID_WEB_SERVICE_PROXY);
      assetList = msg.getStringMapFromFields(NXCPCodes.VID_AM_DATA_BASE, NXCPCodes.VID_AM_COUNT);
   }

   /**
    * @return the overviewDciData
    */
   public List<DciValue> getOverviewDciData()
   {
      return overviewDciData;
   }

   /**
    * @return the tooltipDciData
    */
   public List<DciValue> getTooltipDciData()
   {
      return tooltipDciData;
   }

   /**
    * Get number of data collection items configured on this object.
    *
    * @return number of data collection items configured on this object
    */
   public int getNumDataCollectionItems()
   {
      return numDataCollectionItems;
   }

   /**
    * Get geolocation control mode
    *
    * @return geolocation control mode
    */
   public GeoLocationControlMode getGeoLocationControlMode()
   {
      return geoLocationControlMode;
   }

   /**
    * Get geo areas configured for that object
    *
    * @return list of geo area IDs
    */
   public long[] getGeoAreas()
   {
      return geoAreas;
   }

   /**
    * Check if generation of location change event is turned on
    * 
    * @return true if location change event should be generated for this object
    */
   public boolean isLocationChageEventGenerated()
   {
      return (flags & DCF_LOCATION_CHANGE_EVENT) != 0;
   }

   /**
    * Get ID of web service proxy node.
    *
    * @return ID of web service proxy node
    */
   public long getWebServiceProxyId()
   {
      return webServiceProxyId;
   }

   /**
    * @see org.netxms.client.objects.interfaces.Assets#getAssets()
    */
   @Override
   public Map<String, String> getAssets()
   {
      return assetList;
   }
}
