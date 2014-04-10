package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;

import org.netxms.base.NXCPMessage;

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
	private int type;
	private Date startTime;
	
	// time format - "year;month;day;hour;minute"
	private String timeFrom;
	private String timeTo;

	/**
	 * Create reportingJob
	 * 
	 * @param reportGuid
	 * @param userId
	 */
	public ReportingJob(UUID reportGuid, int userId)
	{
		jobId = UUID.randomUUID();
		reportId = reportGuid;
		this.userId = userId;
	}
	
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
		startTime = msg.getVariableAsDate(varId + 3);
		daysOfWeek = msg.getVariableAsInteger(varId + 4);
		daysOfMonth = msg.getVariableAsInteger(varId + 5);
		type = msg.getVariableAsInteger(varId + 6);
		timeFrom = msg.getVariableAsString(varId + 7);
		timeTo = msg.getVariableAsString(varId + 8);
	}
	
	public UUID getReportId()
	{
		return reportId;
	}

	public void setReportId(UUID reportId)
	{
		this.reportId = reportId;
	}

	public UUID getJobId()
	{
		return jobId;
	}

	public void setJobId(UUID jobId)
	{
		this.jobId = jobId;
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

	public int getUserId()
	{
		return userId;
	}

	public int getType()
	{
		return type;
	}

	public void setType(int type)
	{
		this.type = type;
	}

	public String getTimeFrom()
	{
		return timeFrom;
	}

	public String getTimeTo()
	{
		return timeTo;
	}

	public Date getStartTime()
	{
		return startTime;
	}

	public void setStartTime(Date startTime)
	{
		this.startTime = startTime;
	}

	@Override
	public String toString()
	{
		return "ReportingJob [reportId=" + reportId + ", jobId=" + jobId + ", startTime=" + startTime +  ", timeFrom=" + timeFrom + 
				", timeTo=" + timeTo + ", daysOfWeek=" + daysOfWeek + ", daysOfMonth=" + daysOfMonth + "]";
	}
}
