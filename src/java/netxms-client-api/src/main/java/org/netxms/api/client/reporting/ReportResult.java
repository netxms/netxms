package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

public class ReportResult
{
	private Date executionTime;
	private UUID jobId;
	private int userId;

	public ReportResult(Date executionTime, UUID reportId, int userId)
	{
		this.executionTime = executionTime;
		this.jobId = reportId;
		this.userId = userId;
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

	public int getUserId()
	{
		return userId;
	}

	public void setUserId(int userId)
	{
		this.userId = userId;
	}

	@Override
	public String toString()
	{
		return "ReportResult [executionTime=" + executionTime + ", jobId=" + jobId + ", userId=" + userId + "]";
	}

	public static ReportResult createFromMessage(NXCPMessage response, long base)
	{
		long id = base;
		final UUID reportId = response.getVariableAsUUID(id++);
		final Date date = response.getVariableAsDate(id++);
		final int userId = response.getVariableAsInteger(id++);
		return new ReportResult(date, reportId, userId);
	}
}
