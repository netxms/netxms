/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Status of agent data reconciliation (offline data collection catch-up) for a node. Reflects how much locally cached data
 * is still pending delivery to the server, how fast it is being delivered, and the estimated completion time.
 */
public class ReconciliationStatus
{
   private final boolean active;
   private final long pendingDataPoints;
   private final Date oldestDataTimestamp;
   private final double rate;
   private final Date lastReport;

   /**
    * Create reconciliation status from NXCP message.
    *
    * @param msg NXCP message
    */
   public ReconciliationStatus(NXCPMessage msg)
   {
      active = msg.getFieldAsBoolean(NXCPCodes.VID_RECONCILIATION_ACTIVE);
      pendingDataPoints = msg.getFieldAsInt64(NXCPCodes.VID_RECONCILIATION_QUEUE_SIZE);
      long oldest = msg.getFieldAsInt64(NXCPCodes.VID_RECONCILIATION_OLDEST_DATA);
      oldestDataTimestamp = (oldest > 0) ? new Date(oldest) : null;
      rate = msg.getFieldAsDouble(NXCPCodes.VID_RECONCILIATION_RATE);
      long report = msg.getFieldAsInt64(NXCPCodes.VID_RECONCILIATION_LAST_REPORT);
      lastReport = (report > 0) ? new Date(report) : null;
   }

   /**
    * Check if reconciliation is currently in progress (agent still has cached data to send).
    *
    * @return true if reconciliation is active
    */
   public boolean isActive()
   {
      return active;
   }

   /**
    * Get number of data points still pending reconciliation as last reported by agent.
    *
    * @return number of pending data points
    */
   public long getPendingDataPoints()
   {
      return pendingDataPoints;
   }

   /**
    * Get timestamp of the oldest cached data point still pending reconciliation.
    *
    * @return timestamp of oldest pending data point, or null if not available
    */
   public Date getOldestDataTimestamp()
   {
      return oldestDataTimestamp;
   }

   /**
    * Get reconciliation drain rate in data points per second (0 if not yet known).
    *
    * @return reconciliation rate in data points per second
    */
   public double getRate()
   {
      return rate;
   }

   /**
    * Get time when reconciliation status was last reported by agent.
    *
    * @return time of last status report, or null if not available
    */
   public Date getLastReport()
   {
      return lastReport;
   }

   /**
    * Get estimated time remaining until reconciliation completes, based on the number of pending data points and current
    * drain rate.
    *
    * @return estimated remaining time in seconds, or -1 if it cannot be calculated
    */
   public long getEstimatedSecondsRemaining()
   {
      if (!active || (pendingDataPoints <= 0) || (rate <= 0))
         return -1;
      return (long)Math.ceil(pendingDataPoints / rate);
   }
}
