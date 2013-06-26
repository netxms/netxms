package org.netxms.api.client.reporting;

import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPMessage;

public class ReportResult
{
	private Date executionTime;
	private UUID jobId;

	public ReportResult(Date executionTime, UUID reportId)
	{
		this.executionTime = executionTime;
		this.jobId = reportId;
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

	@Override
	public String toString()
	{
		return "ReportResult [executionTime=" + executionTime + ", jobId=" + jobId + "]";
	}

	public static ReportResult createFromMessage(NXCPMessage response, long base)
	{
		long id = base;
		final UUID reportId = response.getVariableAsUUID(id++);
		final Date date = response.getVariableAsDate(id++);
		return new ReportResult(date, reportId);
	}
}
