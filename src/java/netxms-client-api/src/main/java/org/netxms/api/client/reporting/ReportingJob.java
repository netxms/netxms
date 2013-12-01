package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;

public class ReportingJob
{
	private UUID reportId;
	private UUID jobId;
	private Date startTime;
	private int daysOfWeek;
	private int daysOfMonth;

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
}
