package org.netxms.ui.web.infrastructure;

import java.io.IOException;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;

public class ConnectionManagerFilter implements Filter
{
	private FilterConfig filterConfig;

	@Override
	public void destroy()
	{
		filterConfig = null;
	}

	@Override
	public void doFilter(final ServletRequest servletRequest, final ServletResponse servletResponse, final FilterChain filterChain)
			throws IOException, ServletException
	{
		final ServletContext context = filterConfig.getServletContext();
		final ConnectionManager connectionManager = (ConnectionManager)context
				.getAttribute(ConnectionManagerListener.CONNECTION_MANAGER);

		final ConnectionManagerContext connectionManagerContext = new ConnectionManagerContext();
		connectionManagerContext.setConnectionManager(connectionManager);

		filterChain.doFilter(servletRequest, servletResponse);
	}

	@Override
	public void init(final FilterConfig filterConfig) throws ServletException
	{
		this.filterConfig = filterConfig;
	}
}
