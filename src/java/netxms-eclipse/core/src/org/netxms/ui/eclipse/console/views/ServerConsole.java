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
package org.netxms.ui.eclipse.console.views;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.tm.internal.terminal.control.ITerminalListener;
import org.eclipse.tm.internal.terminal.control.ITerminalViewControl;
import org.eclipse.tm.internal.terminal.control.TerminalViewControlFactory;
import org.eclipse.tm.internal.terminal.provisional.api.TerminalState;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerConsoleListener;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.views.helpers.ServerConsoleTerminalConnector;
import org.netxms.ui.eclipse.console.views.helpers.TerminalReader;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Server console view
 */
@SuppressWarnings("restriction")
public class ServerConsole extends ViewPart implements ITerminalListener
{
	public static final String ID = "org.netxms.ui.eclipse.console.views.ServerConsole"; //$NON-NLS-1$
	
	private ITerminalViewControl terminal;
	private ServerConsoleTerminalConnector connector;
	private NXCSession session;
	private boolean scrollLock = false;
	private Action actionClear;
	private Action actionScrollLock;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());
		
		terminal = TerminalViewControlFactory.makeControl(this, parent, null);
		connector = new ServerConsoleTerminalConnector();
		terminal.setConnector(connector);
		terminal.connectTerminal();
		terminal.setInvertedColors(true);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		if (session.isServerConsoleConnected())
			setConnected();
		else
			connectToServer();
	}
	
	/**
	 * Connect to server
	 */
	private void connectToServer()
	{
		new ConsoleJob(Messages.getString("ServerConsole.OpenServerConsole"), null, Activator.PLUGIN_ID, null) { //$NON-NLS-1$
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				session.openConsole();
				setConnected();
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.getString("ServerConsole.CannotOpen"); //$NON-NLS-1$
			}

			@Override
			protected void jobFailureHandler()
			{
				writeToTerminal("\r\n\u001b[31;1m*** DISCONNECTED ***\u001b[0m"); //$NON-NLS-1$
				terminal.disconnectTerminal();
			}
		}.start();
	}
	
	/**
	 * Inform view that server console is connected
	 */
	private void setConnected()
	{
		writeToTerminal("\u001b[1mNetXMS Server Remote Console V" + session.getServerVersion() + " Ready\r\n\r\n\u001b[0m"); //$NON-NLS-1$ //$NON-NLS-2$
		
		session.addConsoleListener(new ServerConsoleListener() {
			@Override
			public void onConsoleOutput(String text)
			{
				writeToTerminal(text.replaceAll("\n", "\r\n")); //$NON-NLS-1$ //$NON-NLS-2$
			}
		});

		// Read console input and send to server
		Thread inputReader = new Thread() {
			@Override
			public void run()
			{
				try
				{
					final TerminalReader in = new TerminalReader(new InputStreamReader(connector.getInputStream()), connector.getRemoteToTerminalOutputStream());
					while(true)
					{
						String command = in.readLine().trim();
						if (!command.isEmpty())
						{
							if (session.processConsoleCommand(command))
							{
								// Console must be closed
								PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
									@Override
									public void run()
									{
										terminal.disconnectTerminal();
										PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().hideView(ServerConsole.this);
									}
								});
								break;
							}
						}
					}
					in.close();
				}
				catch(Exception e)
				{
				}
			}
		};
		inputReader.start();
	}

	/**
	 * Write text to terminal
	 * 
	 * @param text text to write
	 */
	private void writeToTerminal(String text)
	{
		try
		{
			connector.getRemoteToTerminalOutputStream().write(text.getBytes());
		}
		catch(UnsupportedEncodingException e)
		{
			// should never happen!
			e.printStackTrace();
		}
		catch(IOException e)
		{
			// should never happen!
			e.printStackTrace();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		terminal.setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionClear = new Action(Messages.getString("ServerConsole.ClearTerminal")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				terminal.clearTerminal();
				writeToTerminal("\u001b[33mnetxmsd:\u001b[0m "); //$NON-NLS-1$
			}
		};
		actionClear.setImageDescriptor(SharedIcons.CLEAR_LOG);

		actionScrollLock = new Action(Messages.getString("ServerConsole.ScrollLock"), Action.AS_CHECK_BOX) { //$NON-NLS-1$
			@Override
			public void run()
			{
				scrollLock = !scrollLock;
				terminal.setScrollLock(scrollLock);
				actionScrollLock.setChecked(scrollLock);
			}
		};
		actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
		actionScrollLock.setChecked(scrollLock);
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(terminal.getControl());
		terminal.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	private void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.control.ITerminalListener#setState(org.eclipse.tm.internal.terminal.provisional.api.TerminalState)
	 */
	@Override
	public void setState(TerminalState state)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.control.ITerminalListener#setTerminalTitle(java.lang.String)
	 */
	@Override
	public void setTerminalTitle(String title)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		terminal.disposeTerminal();
		super.dispose();
	}
}
