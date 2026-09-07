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
package org.netxms.client.ai;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Standing instructions history record of an AI operator instance: instructions text as it was before a change.
 */
public class AiOperatorInstructionsHistoryRecord
{
   private long id;
   private int iteration;
   private Date timestamp;
   private String previousText;

   /**
    * Create record from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiOperatorInstructionsHistoryRecord(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      iteration = msg.getFieldAsInt32(baseId + 1);
      timestamp = msg.getFieldAsDate(baseId + 2);
      previousText = msg.getFieldAsString(baseId + 3);
   }

   /**
    * @return record ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return instance iteration at which the instructions changed
    */
   public int getIteration()
   {
      return iteration;
   }

   /**
    * @return time of the change
    */
   public Date getTimestamp()
   {
      return timestamp;
   }

   /**
    * @return instructions text before the change
    */
   public String getPreviousText()
   {
      return previousText;
   }
}
