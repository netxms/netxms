/**
 * 
 */
package org.netxms.ui.eclipse.console;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.netxms.api.client.Session;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * @author victor
 * 
 */
public class LoginJob implements IRunnableWithProgress
{
	private final class KeepAliveHelper implements Runnable
	{
		@Override
		public void run()
		{
			final Session session = ConsoleSharedData.getSession();
			try
			{
				session.checkConnection();
				Thread.sleep(1000 * 30); // send keepalive every 30 seconds
			}
			catch(Exception e)
			{
				// ignore everything
			}
		}
	}

	private String server;
	private String loginName;
	private String password;

	public LoginJob(final String server, final String loginName, final String password)
	{
		this.server = server;
		this.loginName = loginName;
		this.password = password;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.operation.IRunnableWithProgress#run(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
	{
		monitor.beginTask(Messages.getString("LoginJob.connecting"), 100); //$NON-NLS-1$
		try
		{
			final String hostName;
			int port = NXCSession.DEFAULT_CONN_PORT;
			final String[] split = server.split(":"); //$NON-NLS-1$
			if (split.length == 2)
			{
				hostName = split[0];
				try
				{
					port = Integer.valueOf(split[1]);
				}
				catch(NumberFormatException e)
				{
					// ignore
				}
			}
			else
			{
				hostName = server;
			}

			NXCSession session = new NXCSession(hostName, port, loginName, password);
			monitor.worked(10);

			session.connect();
			monitor.worked(40);

			monitor.setTaskName(Messages.getString("LoginJob.sync_objects")); //$NON-NLS-1$
			session.syncObjects();
			monitor.worked(25);

			monitor.setTaskName(Messages.getString("LoginJob.sync_users")); //$NON-NLS-1$
			session.syncUserDatabase();
			monitor.worked(5);

			monitor.setTaskName(Messages.getString("LoginJob.sync_event_db")); //$NON-NLS-1$
			session.syncEventTemplates();
			monitor.worked(5);

			monitor.setTaskName(Messages.getString("LoginJob.subscribe")); //$NON-NLS-1$
			session.subscribe(NXCSession.CHANNEL_ALARMS | NXCSession.CHANNEL_OBJECTS | NXCSession.CHANNEL_EVENTS);
			monitor.worked(5);

			ConsoleSharedData.setSession(session);
			
			monitor.setTaskName(Messages.getString("LoginJob.init_extensions")); //$NON-NLS-1$
			TweakletManager.postLogin(session);
			monitor.worked(5);

			Runnable keepAliveTimer = new KeepAliveHelper();
			final Thread thread = new Thread(keepAliveTimer);
			thread.setDaemon(true);
			thread.start();
		}
		catch(Exception e)
		{
			throw new InvocationTargetException(e);
		}
		finally
		{
			monitor.done();
		}
	}
}
