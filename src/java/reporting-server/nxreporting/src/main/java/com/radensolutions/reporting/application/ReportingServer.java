package com.radensolutions.reporting.application;

import com.radensolutions.reporting.infrastructure.Connector;
import com.radensolutions.reporting.infrastructure.ReportManager;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;

public class ReportingServer
{

	ApplicationContext context;

	public ReportingServer()
	{
		initContext();
	}

	private void initContext()
	{
		if (context == null)
			context = new ClassPathXmlApplicationContext("SpringBeans.xml");
	}

	public Connector getConnector()
	{
		return context.getBean(Connector.class);
	}

	public Session getSession(Connector connector)
	{
		return (Session) context.getBean("session", connector);
	}

	public ReportManager getReportManager()
	{
		return (ReportManager) context.getBean("reportManager");
	}
}
