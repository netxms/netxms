/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;

/**
 * Base class for all data collection targets
 */
public class DataCollectionTarget extends GenericObject
{
   protected List<DciValue> overviewDciData;

   /**
    * Create new object.
    * 
    * @param id
    * @param session
    */
   public DataCollectionTarget(long id, NXCSession session)
   {
      super(id, session);
      overviewDciData = new ArrayList<DciValue>(0);
   }

   /**
    * Create object from NXCP message.
    * 
    * @param msg
    * @param session
    */
   public DataCollectionTarget(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      
      int count = msg.getFieldAsInt32(NXCPCodes.VID_OVERVIEW_DCI_COUNT);
      overviewDciData = new ArrayList<DciValue>(count);
      long fieldId = NXCPCodes.VID_OVERVIEW_DCI_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         overviewDciData.add(DciValue.createFromMessage(objectId, msg, fieldId));
         fieldId += 50;
      }
   }

   /**
    * @return the overviewDciData
    */
   public List<DciValue> getOverviewDciData()
   {
      return overviewDciData;
   }
}
