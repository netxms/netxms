package com.radensolutions.reporting.domain.impl;

import static java.util.Collections.checkedList;

import java.util.List;
import java.util.UUID;

import org.hibernate.Query;
import org.hibernate.SessionFactory;
import org.hibernate.classic.Session;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import com.radensolutions.reporting.domain.ReportResult;
import com.radensolutions.reporting.domain.ReportResultDAO;

@SuppressWarnings("unchecked")
@Repository
public class ReportResultDAOImpl implements ReportResultDAO
{
	
	public static final Logger logger = LoggerFactory.getLogger(ReportResultDAOImpl.class);

	@Autowired
	private SessionFactory sessionFactory;

	@Override
	public void saveResult(ReportResult result)
	{
		sessionFactory.getCurrentSession().save(result);
	}

	@Override
	public List<ReportResult> listResults(UUID reportId, int userId)
	{
		String strQuery = "from ReportResult result where result.reportId = ? order by executionTime desc";
		final Query query = sessionFactory.getCurrentSession().createQuery(strQuery).setParameter(0, reportId);
		return checkedList(query.list(), ReportResult.class);
	}

	@Override
	public void deleteJob(UUID jobId)
	{
		final Session session = sessionFactory.getCurrentSession();
		final List<ReportResult> list = findReportsResult(jobId);
		for (ReportResult reportResult : list)
		{
			session.delete(reportResult);
		}
	}
	
	@Override
	public List<ReportResult> findReportsResult(UUID jobId)
	{
		final Query query = sessionFactory.getCurrentSession().createQuery("from ReportResult where jobId = ?").setParameter(0, jobId);
		return checkedList(query.list(), ReportResult.class);
	}
}
