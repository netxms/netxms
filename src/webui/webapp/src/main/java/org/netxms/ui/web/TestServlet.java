package org.netxms.ui.web;

import java.io.IOException;
import java.io.PrintWriter;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.netxms.client.NXCSession;
import org.netxms.ui.web.infrastructure.SessionContext;

public class TestServlet extends HttpServlet
{
	private static final long serialVersionUID = -8399305760880154960L;

	@Override
	protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException
	{
		final PrintWriter writer = resp.getWriter();
		writer.println("ok ");
		writer.flush();
		final NXCSession session = new SessionContext().getSession();
		writer.println("Session: " + session);
	}

	@Override
	protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException
	{
	}
}
