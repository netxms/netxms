/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * State of object poll
 */
public class PollState
{
   private String pollType;
   private boolean pending;
   private Date lastCompleted;
   private int lastRunTime;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public PollState(NXCPMessage msg, long baseId)
   {
      pollType = msg.getFieldAsString(baseId);
      pending = msg.getFieldAsBoolean(baseId + 1);
      lastCompleted = msg.getFieldAsDate(baseId + 2);
      lastRunTime = msg.getFieldAsInt32(baseId + 3);
   }

   /**
    * @return the pollType
    */
   public String getPollType()
   {
      return pollType;
   }

   /**
    * @return the pending
    */
   public boolean isPending()
   {
      return pending;
   }

   /**
    * @return the lastCompleted
    */
   public Date getLastCompleted()
   {
      return lastCompleted;
   }

   /**
    * @return the lastRunTime
    */
   public int getLastRunTime()
   {
      return lastRunTime;
   }
}
