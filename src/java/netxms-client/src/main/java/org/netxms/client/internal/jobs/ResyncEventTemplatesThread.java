/**
 * 
 */
package org.netxms.client.internal.jobs;

import org.netxms.client.NXCSession;

/**
 * @author victor
 *
 */
public class ResyncEventTemplatesThread extends Thread
{
	private NXCSession session;
	
	public ResyncEventTemplatesThread(final NXCSession session)
	{
		this.session = session;
		setDaemon(true);
		start();
	}

	/* (non-Javadoc)
	 * @see java.lang.Thread#run()
	 */
	@Override
	public void run()
	{
		try
		{
			session.syncEventTemplates();
		}
		catch(Exception e)
		{
			/* TODO: add logger output */
		}
	}
}
