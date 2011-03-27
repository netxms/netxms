package org.netxms.ui.web.infrastructure;

import java.io.IOException;
import java.security.SecureRandom;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ConnectionManager
{
	private static final Logger logger = LoggerFactory.getLogger(ConnectionManager.class);

	private final Map<String, SessionState> sessions;

	private final Thread houseKeeperThread;

	private final SessionHouseKeeper sessionHouseKeeper;

	public ConnectionManager()
	{
		sessions = new HashMap<String, SessionState>(0);
		sessionHouseKeeper = new SessionHouseKeeper(sessions);
		houseKeeperThread = new Thread(sessionHouseKeeper, "Sessions house keeper");
	}

	private void closeAllSessions()
	{
		synchronized(sessions)
		{
			for(final Map.Entry<String, SessionState> entry : sessions.entrySet())
			{
				final SessionState sessionState = entry.getValue();
				if (sessionState.isConnected())
				{
					logger.debug("Terminating session {}", entry.getKey());
					sessionState.getSession().disconnect();
					sessionState.setConnected(false);
				}
			}
			sessions.clear();
		}
	}

	private String generateSessionId()
	{
		final SecureRandom random = new SecureRandom();
		return Long.toHexString(random.nextLong()) + Long.toHexString(new Date().getTime()) + Long.toHexString(random.nextLong());
	}

	/**
	 * Retrive existing NXCSession by session id and reset timeout timer
	 * 
	 * @param sessionId
	 *           existing session id
	 * @return NXCSession or null if session not found
	 */
	public NXCSession getSession(final String sessionId)
	{
		NXCSession session = null;
		synchronized(sessions)
		{
			final SessionState sessionState = sessions.get(sessionId);
			if (sessionState != null)
			{
				sessionState.ping();
				session = sessionState.getSession();
			}
		}
		return session;
	}

	/**
	 * Login to NetXMS Server and return session id, which should be used to
	 * retrive NXCSession later on
	 * 
	 * @param hostname
	 *           NetXMS server hostname or IP
	 * @param username
	 *           User login
	 * @param password
	 *           User password
	 * @return Session id
	 * @throws IOException
	 *            Communication error
	 * @throws NetXMSClientException
	 *            NetXMS error (access denied, etc.)
	 */
	public String login(final String hostname, final String username, final String password) throws IOException,
			NetXMSClientException
	{
		final String sessionId = generateSessionId();
		logger.debug("Generated session id {} for user {}", sessionId, username);

		final NXCSession session = new NXCSession(hostname, username, password);
		session.connect();
		session.syncObjects();
		session.syncUserDatabase();
		session.syncEventTemplates();

		session.subscribe(NXCSession.CHANNEL_ALARMS | NXCSession.CHANNEL_OBJECTS | NXCSession.CHANNEL_EVENTS);

		synchronized(sessions)
		{
			sessions.put(sessionId, new SessionState(session));
		}

		return sessionId;
	}

	public void logout(final String sessionId)
	{
		final SessionState sessionState;
		synchronized(sessions)
		{
			sessionState = sessions.remove(sessionId);
		}
		if (sessionState != null)
		{
			final NXCSession session = sessionState.getSession();
			session.disconnect();
		}
	}

	void shutdown()
	{
		logger.debug("Shutdown");

		shutdownHousekeeper();

		logger.debug("Housekeeper terminated");

		closeAllSessions();

		if (logger.isDebugEnabled())
		{
			logger.debug("All connections closed");
		}
	}

	private void shutdownHousekeeper()
	{
		sessionHouseKeeper.shutdown();

		if (houseKeeperThread.isAlive())
		{
			try
			{
				houseKeeperThread.join();
			}
			catch(final InterruptedException e)
			{
				logger.error("Can't join housekeeper thread", e);
			}
		}
	}

	final void startup()
	{
		logger.debug("Startup");

		houseKeeperThread.start();
	}
}
