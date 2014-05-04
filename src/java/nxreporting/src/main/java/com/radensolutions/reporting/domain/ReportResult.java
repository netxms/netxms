package com.radensolutions.reporting.domain;

import java.io.Serializable;
import java.util.Date;
import java.util.UUID;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.Table;

@Entity
@Table(name = "reporting_results")
public class ReportResult implements Serializable
{

	private static final long serialVersionUID = -3946455080023055986L;

	@Id
	@Column(name = "id")
	@GeneratedValue
	private Integer id;

	@Column(name = "executionTime")
	private Date executionTime;

	@Column(name = "reportId")
	private UUID reportId;

	@Column(name = "jobId")
	private UUID jobId;

	@Column(name = "userId")
	private int userId;
	
	public ReportResult()
	{
	}

	public ReportResult(Date executionTime, UUID reportId, UUID jobId, int userId)
	{
		this.executionTime = executionTime;
		this.reportId = reportId;
		this.jobId = jobId;
		this.userId = userId;
	}

	public Integer getId()
	{
		return id;
	}

	public void setId(Integer id)
	{
		this.id = id;
	}

	public Date getExecutionTime()
	{
		return executionTime;
	}

	public void setExecutionTime(Date executionTime)
	{
		this.executionTime = executionTime;
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
		return "ReportResult{" + "id=" + id + ", executionTime=" + executionTime + ", reportId=" + reportId + ", jobId=" + jobId + ", userId='" + userId + '\'' + '}';
	}
}
