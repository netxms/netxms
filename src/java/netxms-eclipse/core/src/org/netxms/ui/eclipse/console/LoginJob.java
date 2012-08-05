/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.console;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;
import org.netxms.api.client.Session;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Login job
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

	private Display display;
	private String server;
	private String loginName;
	private String password;
	private boolean encryptSession;

	/**
	 * @param display
	 * @param server
	 * @param loginName
	 * @param password
	 */
	public LoginJob(Display display, String server, String loginName, String password, boolean encryptSession)
	{
		this.display = display;
		this.server = server;
		this.loginName = loginName;
		this.password = password;
		this.encryptSession = encryptSession;
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

			NXCSession session = new NXCSession(hostName, port, loginName, password, encryptSession);
			session.setConnClientInfo("nxmc/" + NXCommon.VERSION); //$NON-NLS-1$
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
			callLoginListeners(session);
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
	
	/**
	 * Inform all registered login listeners about successful login
	 * 
	 * @param session new client session
	 */
	private void callLoginListeners(NXCSession session)
	{
		// Read all registered extensions and create listeners
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.loginlisteners"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ConsoleLoginListener listener = (ConsoleLoginListener)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				listener.afterLogin(session, display);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
}
