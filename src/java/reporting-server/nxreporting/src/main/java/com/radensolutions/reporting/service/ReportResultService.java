package com.radensolutions.reporting.service;

import java.util.List;
import java.util.UUID;

import com.radensolutions.reporting.domain.ReportResult;

public interface ReportResultService
{

	void save(ReportResult result);

	List<ReportResult> list(UUID reportId, int userId);

	void delete(UUID jobId);
	
	UUID findReportId(UUID jobId);
}
