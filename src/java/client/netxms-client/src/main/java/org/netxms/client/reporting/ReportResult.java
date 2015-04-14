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
		final UUID reportId = response.getFieldAsUUID(id++);
		final Date date = response.getFieldAsDate(id++);
		final int userId = response.getFieldAsInt32(id++);
		return new ReportResult(date, reportId, userId);
	}
}
