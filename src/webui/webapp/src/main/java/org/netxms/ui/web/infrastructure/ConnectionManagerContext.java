package org.netxms.ui.web.infrastructure;

import org.netxms.client.NXCSession;

public class ConnectionManagerContext
{
	private static final ThreadLocal<ConnectionManager> threadLocal = new ThreadLocal<ConnectionManager>();

	public ConnectionManager getManager()
	{
		return threadLocal.get();
	}

	public NXCSession getSession(final String sessionId)
	{
		return getManager().getSession(sessionId);
	}

	void setConnectionManager(final ConnectionManager connectionManager)
	{
		threadLocal.set(connectionManager);
	}
}
