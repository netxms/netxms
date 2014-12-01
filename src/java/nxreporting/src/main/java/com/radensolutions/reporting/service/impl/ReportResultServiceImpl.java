package com.radensolutions.reporting.service.impl;

import com.radensolutions.reporting.dao.ReportResultDAO;
import com.radensolutions.reporting.model.ReportResult;
import com.radensolutions.reporting.service.ReportResultService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.UUID;

@Service
public class ReportResultServiceImpl implements ReportResultService {

    @Autowired
    private ReportResultDAO reportResultDAO;

    @Override
    @Transactional
    public void save(ReportResult result) {
        reportResultDAO.saveResult(result);
    }

    @Override
    @Transactional
    public List<ReportResult> list(UUID reportId, int userId) {
        return reportResultDAO.listResults(reportId, userId);
    }

    @Override
    @Transactional
    public void delete(UUID jobId) {
        reportResultDAO.deleteJob(jobId);
    }

    @Override
    @Transactional
    public UUID findReportId(UUID jobId) {
        UUID result = null;
        List<ReportResult> reportResult = reportResultDAO.findReportsResult(jobId);
        if (reportResult.size() > 0) {
            result = reportResult.get(0).getReportId();
        }
        return result;
    }
}
