/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.websvc.handlers;

import java.util.Date;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Business service check request handler
 */
public class BusinessServiceTickets extends AbstractObjectHandler
{   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {      
      NXCSession session = getSession();

      Date periodStart = parseTimestamp(query.get("start"));
      Date periodEnd = parseTimestamp(query.get("end"));

      int timeRange = parseInt(query.get("timeRage"), 0);
      TimeUnit timeUnit = null;
      try
      {
         if ((query.get("timeUnit") != null) && !query.get("timeUnit").isEmpty())
            timeUnit = TimeUnit.valueOf(query.get("timeUnit").trim().toUpperCase());         
      }
      catch (Exception e)
      {
         throw new NXCException(RCC.INVALID_TIME_INTERVAL);         
      }
      
      if ((periodStart == null || periodEnd == null) && 
            (timeUnit == null || timeRange == 0))
         throw new NXCException(RCC.INVALID_TIME_INTERVAL); 
      
      TimePeriod tp = new TimePeriod((periodStart != null && periodEnd != null) ? TimeFrameType.FIXED : TimeFrameType.BACK_FROM_NOW, 
            timeRange, timeUnit, periodStart, periodEnd);
      
      List<BusinessServiceTicket> tickets = session.getBusinessServiceTickets(getObjectId(), tp);
      return new ResponseContainer("tickets", tickets);
   }
}
