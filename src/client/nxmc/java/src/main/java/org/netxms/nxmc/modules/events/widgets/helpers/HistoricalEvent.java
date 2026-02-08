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
package org.netxms.nxmc.modules.events.widgets.helpers;

import java.util.Date;
import org.netxms.client.TableRow;
import org.netxms.client.constants.Severity;

/**
 * Historical event loaded from event log.
 * Provides Event-compatible interface for display in EventTraceWidget.
 */
public class HistoricalEvent
{
   private final long id;
   private final int code;
   private final Date timeStamp;
   private final long sourceId;
   private final long dciId;
   private final Severity severity;
   private final String message;

   /**
    * Create historical event from log table row.
    *
    * @param row table row from log query
    * @param idxId column index for event_id
    * @param idxTimestamp column index for event_timestamp
    * @param idxSource column index for event_source
    * @param idxCode column index for event_code
    * @param idxSeverity column index for event_severity
    * @param idxMessage column index for event_message
    * @param idxDciId column index for dci_id (-1 if not available)
    */
   public HistoricalEvent(TableRow row, int idxId, int idxTimestamp, int idxSource, int idxCode, int idxSeverity, int idxMessage, int idxDciId)
   {
      this.id = row.getValueAsLong(idxId);
      this.timeStamp = new Date(row.getValueAsLong(idxTimestamp) * 1000);
      this.sourceId = row.getValueAsLong(idxSource);
      this.code = (int)row.getValueAsLong(idxCode);
      this.severity = Severity.getByValue((int)row.getValueAsLong(idxSeverity));
      this.message = row.get(idxMessage).getValue();
      this.dciId = (idxDciId >= 0) ? row.getValueAsLong(idxDciId) : 0;
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the code
    */
   public int getCode()
   {
      return code;
   }

   /**
    * @return the timeStamp
    */
   public Date getTimeStamp()
   {
      return timeStamp;
   }

   /**
    * @return the sourceId
    */
   public long getSourceId()
   {
      return sourceId;
   }

   /**
    * @return the dciId
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return the severity
    */
   public Severity getSeverity()
   {
      return severity;
   }

   /**
    * @return the message
    */
   public String getMessage()
   {
      return message;
   }
}
