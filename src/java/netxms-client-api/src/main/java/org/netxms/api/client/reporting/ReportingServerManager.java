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
package org.netxms.api.client.reporting;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.netxms.api.client.NetXMSClientException;

/**
 * Reporting server management interface
 */
public interface ReportingServerManager
{
	List<UUID> listReports() throws NetXMSClientException, IOException;

	ReportDefinition getReportDefinition(UUID reportId) throws NetXMSClientException, IOException;

	UUID executeReport(UUID reportId, Map<String, String> parameters) throws NetXMSClientException, IOException;

	public void scheduleReport(ReportingJob reportingJob, Map<String, String> parameters) throws NetXMSClientException, IOException;
	
	List<ReportingJob> listScheduledJobs(UUID reportId) throws NetXMSClientException, IOException;

	List<ReportResult> listReportResults(UUID reportId) throws NetXMSClientException, IOException;

	void deleteReportResult(UUID reportId, UUID jobId) throws NetXMSClientException, IOException;
	
	void deleteReportSchedule(UUID reportId, UUID jobId) throws NetXMSClientException, IOException;

	File renderReport(UUID reportId, UUID jobId, ReportRenderFormat format) throws NetXMSClientException, IOException;
}
