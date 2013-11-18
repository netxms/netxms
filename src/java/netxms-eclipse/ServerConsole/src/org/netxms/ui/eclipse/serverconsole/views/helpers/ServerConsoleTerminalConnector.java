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
package org.netxms.ui.eclipse.serverconsole.views.helpers;

import java.io.IOException;
import java.io.OutputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;

import org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector;
import org.eclipse.tm.internal.terminal.provisional.api.ITerminalControl;
import org.eclipse.tm.internal.terminal.provisional.api.TerminalState;
import org.eclipse.tm.internal.terminal.provisional.api.provider.TerminalConnectorImpl;

/**
 * Terminal connector for server console
 */
@SuppressWarnings("restriction")
public class ServerConsoleTerminalConnector extends TerminalConnectorImpl implements ITerminalConnector
{
	private PipedInputStream inputStream;
	private PipedOutputStream outputStream;
	
	/**
	 * 
	 */
	public ServerConsoleTerminalConnector()
	{
		outputStream = new PipedOutputStream();
		inputStream = new PipedInputStream();
		try
		{
			outputStream.connect(inputStream);
		}
		catch(IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.provider.TerminalConnectorImpl#getTerminalToRemoteStream()
	 */
	@Override
	public OutputStream getTerminalToRemoteStream()
	{
		return outputStream;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.provider.TerminalConnectorImpl#getSettingsSummary()
	 */
	@Override
	public String getSettingsSummary()
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdaptable#getAdapter(java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Class adapter)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector#getId()
	 */
	@Override
	public String getId()
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector#getName()
	 */
	@Override
	public String getName()
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector#isHidden()
	 */
	@Override
	public boolean isHidden()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector#isInitialized()
	 */
	@Override
	public boolean isInitialized()
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.ITerminalConnector#getInitializationErrorMessage()
	 */
	@Override
	public String getInitializationErrorMessage()
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.provider.TerminalConnectorImpl#isLocalEcho()
	 */
	@Override
	public boolean isLocalEcho()
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.tm.internal.terminal.provisional.api.provider.TerminalConnectorImpl#connect(org.eclipse.tm.internal.terminal.provisional.api.ITerminalControl)
	 */
	@Override
	public void connect(ITerminalControl control)
	{
		super.connect(control);
		control.setState(TerminalState.CONNECTED);
	}

	/**
	 * @return the inputStream
	 */
	public PipedInputStream getInputStream()
	{
		return inputStream;
	}

	/**
	 * @return
	 */
	public OutputStream getRemoteToTerminalOutputStream()
	{
		return fControl.getRemoteToTerminalOutputStream();
	}
}
