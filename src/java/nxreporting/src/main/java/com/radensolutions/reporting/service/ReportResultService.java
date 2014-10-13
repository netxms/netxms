package com.radensolutions.reporting.service;

import com.radensolutions.reporting.domain.ReportResult;

import java.util.List;
import java.util.UUID;

public interface ReportResultService {

    void save(ReportResult result);

    List<ReportResult> list(UUID reportId, int userId);

    void delete(UUID jobId);

    UUID findReportId(UUID jobId);
}
