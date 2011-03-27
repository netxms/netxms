package org.netxms.ui.web.infrastructure;

import org.netxms.client.NXCSession;

public class SessionContext
{
	private static final ThreadLocal<NXCSession> threadLocal = new ThreadLocal<NXCSession>();

	public NXCSession getSession()
	{
		return threadLocal.get();
	}

	void setSession(final NXCSession session)
	{
		threadLocal.set(session);
	}
}
