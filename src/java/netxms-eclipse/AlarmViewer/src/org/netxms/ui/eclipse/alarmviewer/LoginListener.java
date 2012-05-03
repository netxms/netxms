/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;

/**
 * Console login listener
 */
public class LoginListener implements ConsoleLoginListener
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.console.api.ConsoleLoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
	 */
	@Override
	public void afterLogin(NXCSession session, Display display)
	{
		AlarmNotifier.init(session);
	}
}
