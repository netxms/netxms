package com.radensolutions.reporting.dao;

import com.radensolutions.reporting.model.ReportResult;

import java.util.List;
import java.util.UUID;

public interface ReportResultDAO {
    void saveResult(ReportResult result);

    List<ReportResult> listResults(UUID reportId, int userId);

    void deleteJob(UUID jobId);

    List<ReportResult> findReportsResult(UUID jobId);
}
