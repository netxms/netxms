package com.radensolutions.reporting.domain;

import java.util.List;
import java.util.UUID;

public interface ReportResultDAO {
    void saveResult(ReportResult result);

    List<ReportResult> listResults(UUID reportId, int userId);

    void deleteJob(UUID jobId);

    List<ReportResult> findReportsResult(UUID jobId);
}
