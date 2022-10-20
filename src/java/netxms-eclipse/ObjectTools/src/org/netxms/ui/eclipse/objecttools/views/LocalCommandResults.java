/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.handlers.IHandlerService;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.TcpPortForwarder;

/**
 * Results of local command execution
 */
public class LocalCommandResults extends AbstractCommandResults
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.LocalCommandResults"; //$NON-NLS-1$

	private Process process;
   private TcpPortForwarder tcpPortForwarder = null;
	private boolean running = false;
	private String lastCommand = null;
	private Object mutex = new Object();
	private Action actionTerminate;
	private Action actionRestart;

	/**
	 * Create actions
	 */
	protected void createActions()
	{
	   super.createActions();

		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

		actionTerminate = new Action(Messages.get().LocalCommandResults_Terminate, SharedIcons.TERMINATE) {
			@Override
			public void run()
			{
				synchronized(mutex)
				{
					if (running)
					{
						process.destroy();
                  if (tcpPortForwarder != null)
                  {
                     tcpPortForwarder.close();
                     tcpPortForwarder = null;
                  }
					}
				}
			}
		};
		actionTerminate.setEnabled(false);
      actionTerminate.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.terminate_process"); //$NON-NLS-1$
		handlerService.activateHandler(actionTerminate.getActionDefinitionId(), new ActionHandler(actionTerminate));

		actionRestart = new Action(Messages.get().LocalCommandResults_Restart, SharedIcons.RESTART) {
			@Override
			public void run()
			{
				runCommand(lastCommand);
			}
		};
		actionRestart.setEnabled(false);
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager Menu manager for pull-down menu
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		super.fillLocalPullDown(manager);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager Menu manager for local toolbar
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		super.fillLocalToolBar(manager);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		super.fillContextMenu(manager);
	}

	/**
	 * Run command (called by tool execution action)
	 * 
	 * @param command
	 */
	public void runCommand(final String command)
	{
		synchronized(mutex)
		{
			if (running)
			{
				process.destroy();
				try
				{
					mutex.wait();
				}
				catch(InterruptedException e)
				{
				}
            if (tcpPortForwarder != null)
            {
               tcpPortForwarder.close();
               tcpPortForwarder = null;
            }
			}
			running = true;
			lastCommand = command;
			actionTerminate.setEnabled(true);
			actionRestart.setEnabled(false);
		}

		final IOConsoleOutputStream out = console.newOutputStream();
		ConsoleJob job = new ConsoleJob(Messages.get().LocalCommandResults_JobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().LocalCommandResults_JobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            String commandLine;
            if (remotePort > 0)
            {
               tcpPortForwarder = new TcpPortForwarder(session, nodeId, remotePort, 0);
               tcpPortForwarder.setConsoleOutputStream(out);
               tcpPortForwarder.run();
               commandLine = command.replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
            }
            else
            {
               commandLine = command;
            }

            Process process;
            if (Platform.getOS().equals(Platform.OS_WIN32))
            {
               commandLine = "CMD.EXE /C " + commandLine; //$NON-NLS-1$
               process = Runtime.getRuntime().exec(commandLine);
            }
            else
            {
               process = Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", commandLine }); //$NON-NLS-1$ //$NON-NLS-2$
            }

				InputStream in = process.getInputStream();
				try
				{
					byte[] data = new byte[16384];
					boolean isWindows = Platform.getOS().equals(Platform.OS_WIN32);
					while(true)
					{
						int bytes = in.read(data);
						if (bytes == -1)
							break;
						String s = new String(Arrays.copyOf(data, bytes));

						// The following is a workaround for issue NX-65
						// Problem is that on Windows XP many system commands
						// (like ping, tracert, etc.) generates output with lines
						// ending in 0x0D 0x0D 0x0A
						if (isWindows)
							out.write(s.replace("\r\r\n", " \r\n")); //$NON-NLS-1$ //$NON-NLS-2$
						else
							out.write(s);
					}

					out.write(Messages.get().LocalCommandResults_Terminated);
				}
				catch(IOException e)
				{
				   Activator.logError("Exception while running local command", e); //$NON-NLS-1$
				}
				finally
				{
					in.close();
					out.close();
				}
			}

			@Override
			protected void jobFinalize()
			{
				synchronized(mutex)
				{
               if (tcpPortForwarder != null)
               {
                  tcpPortForwarder.close();
                  tcpPortForwarder = null;
               }
					running = false;
					process = null;
					mutex.notifyAll();
				}

				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						synchronized(mutex)
						{
							actionTerminate.setEnabled(running);
							actionRestart.setEnabled(!running);
						}
					}
				});
			}
		};
		job.setUser(false);
		job.setSystem(true);
		job.start();
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
		synchronized(mutex)
		{
			if (running)
			{
			   if (process != null)
			      process.destroy();
            if (tcpPortForwarder != null)
            {
               tcpPortForwarder.close();
               tcpPortForwarder = null;
            }
			}
		}
		super.dispose();
	}
}
