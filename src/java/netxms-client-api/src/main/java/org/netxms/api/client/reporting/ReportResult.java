package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

public class ReportResult
{
	private Date executionTime;
	private UUID jobId;
	private String userName;

	public ReportResult(Date executionTime, UUID reportId, String userName)
	{
		this.executionTime = executionTime;
		this.jobId = reportId;
		this.userName = userName;
	}

	public Date getExecutionTime()
	{
		return executionTime;
	}

	public void setExecutionTime(Date executionTime)
	{
		this.executionTime = executionTime;
	}

	public UUID getJobId()
	{
		return jobId;
	}

	public void setJobId(UUID reportId)
	{
		this.jobId = reportId;
	}

	public String getUserName()
	{
		return userName;
	}

	public void setUserName(String userName)
	{
		this.userName = userName;
	}

	@Override
	public String toString()
	{
		return "ReportResult [executionTime=" + executionTime + ", jobId=" + jobId + ", userName=" + userName + "]";
	}

	public static ReportResult createFromMessage(NXCPMessage response, long base)
	{
		long id = base;
		final UUID reportId = response.getVariableAsUUID(id++);
		final Date date = response.getVariableAsDate(id++);
		final String userName = response.getVariableAsString(id++);
		return new ReportResult(date, reportId, userName);
	}
}
