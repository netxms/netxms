package org.netxms.api.client.reporting;

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
}
