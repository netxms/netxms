/**
 * 
 */
package org.netxms.ui.eclipse.console;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.netxms.client.*;
import org.netxms.ui.eclipse.shared.NXMCSharedData;


/**
 * @author victor
 *
 */
public class LoginJob implements IRunnableWithProgress
{
	private String server;
	private String loginName;
	private String password;
	
	public LoginJob(final String server, final String loginName, final String password)
	{
		this.server = server;
		this.loginName = loginName;
		this.password = password;
	}

	@Override
	public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
	{
		monitor.beginTask(Messages.getString("LoginJob.connecting"), 100); //$NON-NLS-1$
		try
		{
			NXCSession session = new NXCSession(server, loginName, password);
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
			session.subscribe(NXCSession.CHANNEL_ALARMS | NXCSession.CHANNEL_OBJECTS);
			monitor.worked(5);
			
			NXMCSharedData.getInstance().setSession(session);
		}
		catch(IOException e)
		{
			throw new InvocationTargetException(e);
		}
		catch(NXCException e)
		{
			throw new InvocationTargetException(e);
		}
		finally
		{
			monitor.done();
		}
	}
}
