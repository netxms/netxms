package org.netxms.ui.web.infrastructure;

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ConnectionManagerListener implements ServletContextListener
{
	private static final Logger logger = LoggerFactory.getLogger(ConnectionManagerListener.class);

	static final String CONNECTION_MANAGER = "org.netxms.ui.web.infrastructure.ConnectionManager";

	@Override
	public void contextDestroyed(final ServletContextEvent event)
	{
		logger.info("NetXMS Web shutdown");

		final ServletContext servletContext = event.getServletContext();
		final ConnectionManager connectionManager = (ConnectionManager)servletContext.getAttribute(CONNECTION_MANAGER);
		if (connectionManager != null)
		{
			connectionManager.shutdown();
		}
		servletContext.removeAttribute(CONNECTION_MANAGER);
	}

	@Override
	public void contextInitialized(final ServletContextEvent event)
	{
		logger.info("NetXMS Web startup");

		final ServletContext servletContext = event.getServletContext();
		final ConnectionManager connectionManager = new ConnectionManager();
		servletContext.setAttribute(CONNECTION_MANAGER, connectionManager);
	}
}
