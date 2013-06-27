package org.netxms.api.client.reporting;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.api.client.NetXMSClientException;

public interface ReportingServerManager
{
	List<UUID> listReports() throws NetXMSClientException, IOException;

	ReportDefinition getReportDefinition(UUID reportId) throws NetXMSClientException, IOException;

	UUID executeReport(UUID reportId, Map<String, String> parameters) throws NetXMSClientException, IOException;

	List<ReportResult> listReportResults(UUID reportId) throws NetXMSClientException, IOException;

	void deleteReportResult(UUID reportId, UUID jobId) throws NetXMSClientException, IOException;

	File renderReport(UUID reportId, UUID jobId, ReportRenderFormat format) throws NetXMSClientException, IOException;
}
