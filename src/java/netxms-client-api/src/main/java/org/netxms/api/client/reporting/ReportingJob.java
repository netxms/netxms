/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 NetXMS Team
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
package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Reporting job
 */
public class ReportingJob
{
   public static final int EXECUTE_ONCE = 0;
   public static final int EXECUTE_DAILY = 1;
   public static final int EXECUTE_WEEKLY = 2;
   public static final int EXECUTE_MONTHLY = 3;
   
	private UUID reportId;
	private UUID jobId;
   private int userId;
   private int type = 0;
	private Date startTime;
	private int daysOfWeek;
	private int daysOfMonth;
   private long startTimeOffset;
   private long endTimeOffset;
   private Date timeFrom = null;
   private Date timeTo = null;

   /**
    * Create reportingJob object from NXCP message
    * 
    * @param msg
    * @param baseId
    */
   public ReportingJob(NXCPMessage msg, long varId)
   {
      jobId = msg.getVariableAsUUID(varId);
      reportId = msg.getVariableAsUUID(varId + 1);
      userId = msg.getVariableAsInteger(varId + 2);
      startTimeOffset = msg.getVariableAsInt64(varId + 3);
      endTimeOffset = msg.getVariableAsInt64(varId + 4);
      daysOfWeek = msg.getVariableAsInteger(varId + 6);
      daysOfMonth = msg.getVariableAsInteger(varId + 7);
      type = msg.getVariableAsInteger(varId + 8);
      startTime = new Date(msg.getVariableAsInt64(varId + 5));
      timeFrom = new Date(msg.getVariableAsInt64(varId + 9));
      timeTo = new Date(msg.getVariableAsInt64(varId + 10));
   }
   
   /**
    * @param reportId
    * @param userId
    * @param daysOfWeek
    * @param daysOfMonth
    * @param startTimeOffset
    * @param endTimeOffset
    * @param timeFrom
    * @param timeTo
    */
   public ReportingJob(UUID reportId, int userId, int daysOfWeek, int daysOfMonth, long startTimeOffset, long endTimeOffset, Date timeFrom, Date timeTo)
   {
      setJobId(UUID.randomUUID());
      setReportId(reportId);
      this.userId = userId;
      this.startTimeOffset = startTimeOffset;
      this.endTimeOffset = endTimeOffset;
      type = EXECUTE_ONCE;
      startTime = new Date();
      this.daysOfWeek = daysOfWeek;
      this.daysOfMonth = daysOfMonth;
      this.timeFrom = timeFrom;
      this.timeTo = timeTo;
   }
   
	/**
	 * @return
	 */
	public UUID getReportId()
	{
		return reportId;
	}

	/**
	 * @param reportId
	 */
	public void setReportId(UUID reportId)
	{
		this.reportId = reportId;
	}

	/**
	 * @return
	 */
	public UUID getJobId()
	{
		return jobId;
	}

	/**
	 * @param jobId
	 */
	public void setJobId(UUID jobId)
	{
		this.jobId = jobId;
	}

	public Date getStartTime()
	{
		return startTime;
	}

	public void setStartTime(Date startTime)
	{
		this.startTime = startTime;
	}

	public int getDaysOfWeek()
	{
		return daysOfWeek;
	}

	public void setDaysOfWeek(int daysOfWeek)
	{
		this.daysOfWeek = daysOfWeek;
	}

	public int getDaysOfMonth()
	{
		return daysOfMonth;
	}

	public void setDaysOfMonth(int daysOfMonth)
	{
		this.daysOfMonth = daysOfMonth;
	}

	@Override
	public String toString()
	{
		return "ReportingJob [reportId=" + reportId + ", jobId=" + jobId + ", startTime=" + startTime + ", daysOfWeek=" + daysOfWeek
				+ ", daysOfMonth=" + daysOfMonth + "]";
	}

   /**
    * @return the userId
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @param userId the userId to set
    */
   public void setUserId(int userId)
   {
      this.userId = userId;
   }

   /**
    * @return the type
    */
   public int getType()
   {
      return type;
   }

   /**
    * @param type the type to set
    */
   public void setType(int type)
   {
      this.type = type;
   }

   /**
    * @return the startTimeOffset
    */
   public long getStartTimeOffset()
   {
      return startTimeOffset;
   }

   /**
    * @param startTimeOffset the startTimeOffset to set
    */
   public void setStartTimeOffset(long startTimeOffset)
   {
      this.startTimeOffset = startTimeOffset;
   }

   /**
    * @return the endTimeOffset
    */
   public long getEndTimeOffset()
   {
      return endTimeOffset;
   }

   /**
    * @param endTimeOffset the endTimeOffset to set
    */
   public void setEndTimeOffset(long endTimeOffset)
   {
      this.endTimeOffset = endTimeOffset;
   }

   /**
    * @return the timeFrom
    */
   public Date getTimeFrom()
   {
      return timeFrom;
   }

   /**
    * @param timeFrom the timeFrom to set
    */
   public void setTimeFrom(Date timeFrom)
   {
      this.timeFrom = timeFrom;
   }

   /**
    * @return the timeTo
    */
   public Date getTimeTo()
   {
      return timeTo;
   }

   /**
    * @param timeTo the timeTo to set
    */
   public void setTimeTo(Date timeTo)
   {
      this.timeTo = timeTo;
   }
}
