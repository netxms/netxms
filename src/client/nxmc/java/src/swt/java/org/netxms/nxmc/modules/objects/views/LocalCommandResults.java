/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Results of local command execution
 */
public class LocalCommandResults extends AbstractCommandResults
{
   private static I18n i18n = LocalizationHelper.getI18n(LocalCommandResults.class);
   private static final Logger logger = LoggerFactory.getLogger(LocalCommandResults.class);
	
	private Process process;
	private boolean running = false;
	private Object mutex = new Object();
	private Action actionTerminate;
	private Action actionRestart;
   
   /**
    * Constructor
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    */
   public LocalCommandResults(ObjectContext node, ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields)
   {
      super(node, tool, inputValues, maskedFields);
   }

	/**
	 * Create actions
	 */
	protected void createActions()
	{
	   super.createActions();
	   
		actionTerminate = new Action(i18n.tr("&Terminate"), SharedIcons.TERMINATE) {
			@Override
			public void run()
			{
				synchronized(mutex)
				{
					if (running)
					{
						process.destroy();
					}
				}
			}
		};
		actionTerminate.setEnabled(false);
      actionTerminate.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.terminate_process"); //$NON-NLS-1$
		
		actionRestart = new Action(i18n.tr("&Restart"), SharedIcons.RESTART) {
			@Override
			public void run()
			{
				execute();
			}
		};
		actionRestart.setEnabled(false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		super.fillLocalMenu(manager);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
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
	
	public void execute()
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
			}
			running = true;
			actionTerminate.setEnabled(true);
			actionRestart.setEnabled(false);
		}
		
		final IOConsoleOutputStream out = console.newOutputStream();
		Job job = new Job(i18n.tr("Execute external command"), this) {
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot execute external command");
			}

			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
            if (executionString.startsWith("#{"))
            {
               int index = executionString.indexOf('}');
               if (index == -1)
               {
                  out.write("Malformed command line (missing closing bracket for #{)");
                  out.close();
                  return;
               }

               // Expect proxy configuration in form
               // proxy:remoteAddress:remotePort:localPort
               // Can use "auto" as proxy name to find suitable proxy node automatically
               String[] proxyConfig = executionString.substring(2, index).split(":");
               if (proxyConfig.length != 4)
               {
                  out.write("Malformed command line (proxy configuration should be in form proxy:remoteAddress:remotePort:localPort)");
                  out.close();
                  return;
               }

               long proxyId;
               if (proxyConfig[0].equals("auto"))
               {
                  proxyId = session.findProxyForNode(object.object.getObjectId());
                  if (proxyId == 0)
                  {
                     out.write("Cannot find suitable proxy");
                     out.close();
                     return;
                  }
               }
               else
               {
                  AbstractObject object = session.findObjectByName(proxyConfig[0]);
                  if (object == null)
                  {
                     out.write("Cannot find proxy " + proxyConfig[0]);
                     out.close();
                     return;
                  }
                  proxyId = object.getObjectId();
               }

               process = Runtime.getRuntime().exec(executionString.substring(index + 1).trim());
            }
            else
            {
               process = Runtime.getRuntime().exec(executionString);
            }
				InputStream in = process.getInputStream();
				try
				{
					byte[] data = new byte[16384];
		         String os = System.getProperty("os.name").toLowerCase();
					boolean isWindows =  os.contains("windows");
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
					
					out.write(i18n.tr("\n\n*** TERMINATED ***\n\n\n"));
				}
				catch(IOException e)
				{
				   logger.error("Exception while running local command", e); //$NON-NLS-1$
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		synchronized(mutex)
		{
			if (running)
			{
				process.destroy();
			}
		}
		super.dispose();
	}
}
