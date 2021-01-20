/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.client.reporting;

import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Report execution result
 */
public class ReportResult
{
   private UUID jobId;
	private Date executionTime;
	private int userId;
   private boolean success;

   /**
    * Create report result object from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public ReportResult(NXCPMessage msg, long baseId)
	{
      jobId = msg.getFieldAsUUID(baseId);
      executionTime = msg.getFieldAsDate(baseId + 1);
      userId = msg.getFieldAsInt32(baseId + 2);
      success = msg.getFieldAsBoolean(baseId + 3);
	}

   /**
    * Get job ID.
    *
    * @return job ID
    */
   public UUID getJobId()
   {
      return jobId;
   }

   /**
    * Get execution time.
    *
    * @return execution time
    */
	public Date getExecutionTime()
	{
		return executionTime;
	}

   /**
    * Get user ID.
    *
    * @return user ID
    */
	public int getUserId()
	{
		return userId;
	}

   /**
    * Get success indicator.
    *
    * @return true if execution was successful
    */
   public boolean isSuccess()
   {
      return success;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportResult [jobId=" + jobId + ", executionTime=" + executionTime + ", userId=" + userId + ", success=" + success + "]";
   }
}
