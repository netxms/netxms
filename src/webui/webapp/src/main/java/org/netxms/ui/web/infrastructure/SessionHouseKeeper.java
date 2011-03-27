package org.netxms.ui.web.infrastructure;

import java.util.Date;
import java.util.Iterator;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

class SessionHouseKeeper implements Runnable
{
	private static final Logger logger = LoggerFactory.getLogger(SessionHouseKeeper.class);

	private static final long SESSION_TIMEOUT = 600000L;

	private final Map<String, SessionState> sessions;
	private volatile boolean terminate;

	SessionHouseKeeper(final Map<String, SessionState> sessions)
	{
		this.sessions = sessions;
	}

	private void cleanupSessions()
	{
		final long currentTime = new Date().getTime();
		synchronized(sessions)
		{
			final Iterator<Map.Entry<String, SessionState>> iterator = sessions.entrySet().iterator();
			while(iterator.hasNext())
			{
				final Map.Entry<String, SessionState> entry = iterator.next();
				final SessionState state = entry.getValue();
				if (state.getLastActivity() + SESSION_TIMEOUT < currentTime)
				{
					state.getSession().disconnect();
					iterator.remove();
				}
			}
		}
	}

	@Override
	public void run()
	{
		while(!terminate)
		{
			cleanupSessions();

			try
			{
				Thread.sleep(5000L);
			}
			catch(final InterruptedException e)
			{
				logger.error("Can't sleep", e);
			}
		}
	}

	public void shutdown()
	{
		terminate = true;
	}
}
