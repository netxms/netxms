package org.netxms.ui.web.infrastructure;

import java.io.IOException;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import org.netxms.api.client.NetXMSClientException;

public class LoginServlet extends HttpServlet
{
	private static final String MAIN_PAGE = "/secure/Launcher.htm";
	private static final String LOGIN_PAGE = "/login.html";
	private static final long serialVersionUID = 3897816142195225812L;

	@Override
	protected void doPost(final HttpServletRequest req, final HttpServletResponse resp) throws ServletException, IOException
	{
		final String server = req.getParameter("server");
		final String login = req.getParameter("login");
		final String password = req.getParameter("password");

		if (server != null && login != null && password != null)
		{
			final ConnectionManager manager = new ConnectionManagerContext().getManager();
			try
			{
				final String sessionId = manager.login(server, login, password);
				final HttpSession session = req.getSession(true);
				session.setAttribute(SessionFilter.NXWEB_SESSION_ATTRIBUTE, sessionId);

				((HttpServletResponse)resp).sendRedirect(req.getContextPath() + MAIN_PAGE);
			}
			catch(NetXMSClientException e)
			{
				e.printStackTrace();
			}
		}
		else
		{
			((HttpServletResponse)resp).sendRedirect(req.getContextPath() + LOGIN_PAGE);
		}
	}
}
