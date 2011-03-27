package org.netxms.ui.web.infrastructure;

import java.io.IOException;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import org.netxms.client.NXCSession;

public class SessionFilter implements Filter
{

	static final String NXWEB_SESSION_ATTRIBUTE = "nxweb-session-id";
	private String contextPath;

	@Override
	public void destroy()
	{
	}

	@Override
	public void doFilter(ServletRequest request, ServletResponse response, FilterChain filterChain) throws IOException,
			ServletException
	{
		final HttpSession session = ((HttpServletRequest)request).getSession(false);
		boolean allowedRequest = false;
		if (session != null)
		{
			final String nxwebSessionId = (String)session.getAttribute(NXWEB_SESSION_ATTRIBUTE);
			if (nxwebSessionId != null)
			{
				final ConnectionManagerContext context = new ConnectionManagerContext();
				if (context != null)
				{
					final NXCSession nxSession = context.getSession(nxwebSessionId);
					if (nxSession != null && nxSession.isConnected())
					{
						allowedRequest = true;
						new SessionContext().setSession(nxSession);
					}
					else
					{
						session.invalidate();
					}
				}
			}
		}

		if (allowedRequest)
		{
			filterChain.doFilter(request, response);
		}
		else
		{
			((HttpServletResponse)response).sendRedirect(contextPath + "/login.html");
		}
	}

	@Override
	public void init(FilterConfig filterConfig) throws ServletException
	{
		contextPath = filterConfig.getServletContext().getContextPath();
	}
}
