package com.radensolutions.reporting.infrastructure;

import com.radensolutions.reporting.domain.ReportDefinition;
import com.radensolutions.reporting.domain.ReportResult;
import org.netxms.api.client.reporting.ReportRenderFormat;

import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;

public interface ReportManager {
    List<UUID> listReports();

    ReportDefinition getReportDefinition(UUID reportId, Locale locale);

    void compileDeployedReports();

    boolean execute(int userId, UUID reportId, UUID jobId, Map<String, String> parameters, Locale locale);

    List<ReportResult> listResults(UUID reportId, int userId);

    boolean deleteResult(UUID reportId, UUID jobId);

    byte[] renderResult(UUID reportId, UUID jobId, ReportRenderFormat format);
}
