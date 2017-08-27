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

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Reporting job
 */
public class ReportingJob
{
	static public final int TYPE_ONCE = 0;
	static public final int TYPE_DAILY = 1;
	static public final int TYPE_WEEKLY = 2;
	static public final int TYPE_MONTHLY = 3;

	private UUID reportId;
	private UUID jobId;
	private int daysOfWeek;
	private int daysOfMonth;
	private int userId;
	private int type = TYPE_ONCE;
	private Date startTime;
	private String comments;
	private boolean notifyOnCompletion = false;
	private ReportRenderFormat renderFormat = ReportRenderFormat.NONE;
	private List<String> emailRecipients= new ArrayList<String>(0);

	/**
	 * Create reportingJob
	 * 
	 * @param reportId report id
	 */
	public ReportingJob(UUID reportId)
	{
		jobId = UUID.randomUUID();
		this.reportId = reportId;
		startTime = new Date();
		daysOfWeek = 0;
		daysOfMonth = 0;
		type = TYPE_ONCE;
	}

	/**
	 * Create reportingJob object from NXCP message
	 * 
	 * @param msg
	 * @param varId base field id
	 */
	public ReportingJob(NXCPMessage msg, long varId)
	{
		jobId = msg.getFieldAsUUID(varId);
		reportId = msg.getFieldAsUUID(varId + 1);
		userId = msg.getFieldAsInt32(varId + 2);
		startTime = msg.getFieldAsDate(varId + 3);
		daysOfWeek = msg.getFieldAsInt32(varId + 4);
		daysOfMonth = msg.getFieldAsInt32(varId + 5);
		type = msg.getFieldAsInt32(varId + 6);
		comments = msg.getFieldAsString(varId + 7);
	}

	/**
    * @return the renderFormat
    */
   public ReportRenderFormat getRenderFormat()
   {
      return renderFormat;
   }

   /**
    * @param renderFormat the renderFormat to set
    */
   public void setRenderFormat(ReportRenderFormat renderFormat)
   {
      this.renderFormat = renderFormat;
   }

   /**
    * @return the emailRecipients
    */
   public List<String> getEmailRecipients()
   {
      return emailRecipients;
   }

   /**
    * @param emailRecipients the emailRecipients to set
    */
   public void setEmailRecipients(List<String> emailRecipients)
   {
      this.emailRecipients = emailRecipients;
   }

   /**
    * Get unique report ID
    * 
	 * @return unique report ID
	 */
	public UUID getReportId()
	{
		return reportId;
	}

	/**
	 * Set unique report ID
	 * 
	 * @param reportId report unique ID
	 */
	public void setReportId(UUID reportId)
	{
		this.reportId = reportId;
	}

	/**
	 * Get job ID
	 * 
	 * @return job ID
	 */
	public UUID getJobId()
	{
		return jobId;
	}

	/**
	 * Set job ID
	 * 
	 * @param jobId job ID
	 */
	public void setJobId(UUID jobId)
	{
		this.jobId = jobId;
	}

	/**
	 * Get days of week bit mask
	 * 
	 * @return days of week bit mask
	 */
	public int getDaysOfWeek()
	{
		return daysOfWeek;
	}

	/**
	 * Set days of week bit mask
	 * 
	 * @param daysOfWeek new days of week bit mask
	 */
	public void setDaysOfWeek(int daysOfWeek)
	{
		this.daysOfWeek = daysOfWeek;
	}

	/**
	 * Get days of month bit mask
	 * 
	 * @return days of month bit mask
	 */
	public int getDaysOfMonth()
	{
		return daysOfMonth;
	}

	/**
	 * Set days of month bit mask
	 * 
	 * @param daysOfMonth new days of month bit mask
	 */
	public void setDaysOfMonth(int daysOfMonth)
	{
		this.daysOfMonth = daysOfMonth;
	}

	/**
	 * Get user ID
	 * 
	 * @return user ID
	 */
	public int getUserId()
	{
		return userId;
	}

	/**
	 * Get job type
	 * 
	 * @return job type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * Set job type
	 * 
	 * @param type new job type
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * Get job start time
	 * 
	 * @return job start time
	 */
	public Date getStartTime()
	{
		return startTime;
	}

	/**
	 * Set job start time
	 * 
	 * @param startTime job start time
	 */
	public void setStartTime(Date startTime)
	{
		this.startTime = startTime;
	}

	/**
	 * Get comments
	 * 
	 * @return comments
	 */
	public String getComments()
	{
		return comments == null ? "" : comments;
	}

	/**
	 * Set comments
	 * 
	 * @param comments comments
	 */
	public void setComments(String comments)
	{
		this.comments = comments;
	}

	/**
	 * Check if notification on completion is on
	 * 
    * @return true if notification on completion is on
    */
   public boolean isNotifyOnCompletion()
   {
      return notifyOnCompletion;
   }

   /**
    * Set notification on completion flag
    * 
    * @param notifyOnCompletion new notification on completion flag
    */
   public void setNotifyOnCompletion(boolean notifyOnCompletion)
   {
      this.notifyOnCompletion = notifyOnCompletion;
   }

   /* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return "ReportingJob [reportId=" + reportId + ", jobId=" + jobId + ", daysOfWeek=" + daysOfWeek + ", daysOfMonth="
				+ daysOfMonth + ", userId=" + userId + ", type=" + type + ", startTime=" + startTime + ", comments=" + comments + "]";
	}
}
