package org.netxms.ui.web.infrastructure;

import java.util.Date;
import org.netxms.client.NXCSession;

class SessionState
{
	private long lastActivity;

	private Boolean connected;

	private final NXCSession session;

	SessionState(final NXCSession session)
	{
		lastActivity = new Date().getTime();
		connected = true;
		this.session = session;
	}

	public long getLastActivity()
	{
		return lastActivity;
	}

	public NXCSession getSession()
	{
		return session;
	}

	public Boolean isConnected()
	{
		return connected;
	}

	public void ping()
	{
		lastActivity = new Date().getTime();
	}

	public void setConnected(final Boolean connected)
	{
		this.connected = connected;
	}
}
