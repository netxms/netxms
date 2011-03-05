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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.tm.internal.terminal.control.ITerminalListener;
import org.eclipse.tm.internal.terminal.control.ITerminalViewControl;
import org.eclipse.tm.internal.terminal.control.TerminalViewControlFactory;
import org.eclipse.tm.internal.terminal.provisional.api.TerminalState;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerConsoleListener;
import org.netxms.ui.eclipse.console.views.helpers.ServerConsoleTerminalConnector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Server console view
 */
@SuppressWarnings("restriction")
public class ServerConsole extends ViewPart implements ITerminalListener
{
	public static final String ID = "org.netxms.ui.eclipse.console.views.ServerConsole";
	
	private ITerminalViewControl terminal;
	private ServerConsoleTerminalConnector connector;
	private NXCSession session;

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
		
		writeToTerminal("\u001b[1mNetXMS Server Remote Console V" + session.getServerVersion() + " Ready\r\n\r\n\u001b[0m");
		
		session.addConsoleListener(new ServerConsoleListener() {
			@Override
			public void onConsoleOutput(String text)
			{
				writeToTerminal(text.replaceAll("\n", "\r\n"));
			}
		});

		// Read console input and send to server
		Thread inputReader = new Thread() {
			@Override
			public void run()
			{
				try
				{
					final BufferedReader in = new BufferedReader(new InputStreamReader(connector.getInputStream()));
					while(true)
					{
						writeToTerminal("\u001b[33mnetxms:\u001b[0m ");
						String command = in.readLine().trim();
						if (!command.isEmpty())
							session.processConsoleCommand(command);
					}
				}
				catch(Exception e)
				{
					
				}
			}
		};
		inputReader.start();
	}

	private void writeToTerminal(String text) {
		try {
			connector.getRemoteToTerminalOutputStream().write(text.getBytes());
		} catch (UnsupportedEncodingException e) {
			// should never happen!
			e.printStackTrace();
		} catch (IOException e) {
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
}
