package com.radensolutions.reporting.application.jobs;

import com.radensolutions.reporting.application.ReportingServerFactory;
import com.radensolutions.reporting.infrastructure.ReportManager;
import org.quartz.Job;
import org.quartz.JobExecutionContext;
import org.quartz.JobExecutionException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;

public class GeneratorJob implements Job
{

	private static final Logger log = LoggerFactory.getLogger(GeneratorJob.class);
	private final ReportManager reportManager;
	private UUID jobId;
	private UUID reportId;
	private Map<String, String> parameters = new HashMap<String, String>();
	private String userName;
	private int userId;
	private long startTimeOffset;
	private long endTimeOffset;

	public GeneratorJob()
	{
		reportManager = ReportingServerFactory.getApp().getReportManager();
	}

	@Override
	public void execute(JobExecutionContext context) throws JobExecutionException
	{
		try
		{
			reportManager.execute(userId, reportId, jobId, parameters, Locale.US);
		} catch (Exception e)
		{
			log.error("Failed to execute report, job terminating", e);
		}
	}

	public UUID getJobId()
	{
		return jobId;
	}

	public void setJobId(UUID jobId)
	{
		this.jobId = jobId;
	}

	public UUID getReportId()
	{
		return reportId;
	}

	public void setReportId(UUID reportId)
	{
		this.reportId = reportId;
	}

	public Map<String, String> getParameters()
	{
		return parameters;
	}

	public void setParameters(Map<String, String> parameters)
	{
		this.parameters = parameters;
	}

	public String getUserName()
	{
		return userName;
	}

	public void setUserName(String userName)
	{
		this.userName = userName;
	}

	public int getUserId()
	{
		return userId;
	}

	public void setUserId(int userId)
	{
		this.userId = userId;
	}

	public long getStartTimeOffset()
	{
		return startTimeOffset;
	}

	public void setStartTimeOffset(long startTimeOffset)
	{
		this.startTimeOffset = startTimeOffset;
	}

	public long getEndTimeOffset()
	{
		return endTimeOffset;
	}

	public void setEndTimeOffset(long endTimeOffset)
	{
		this.endTimeOffset = endTimeOffset;
	}

	@Override
	public String toString()
	{
		return "GeneratorJob{" + "reportManager=" + reportManager + ", jobId=" + jobId + ", reportId=" + reportId + ", parameters=" + parameters + ", userName='" + userName + '\'' + ", userId='" + userId;
	}
}