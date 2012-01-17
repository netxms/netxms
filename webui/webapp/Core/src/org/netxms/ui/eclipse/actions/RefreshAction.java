/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.actions;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.netxms.webui.core.Activator;
import org.netxms.webui.core.Messages;

/**
 * @author victor
 *
 */
public class RefreshAction extends Action
{
	private static final long serialVersionUID = 1L;

	/**
	 * Constructor
	 */
	public RefreshAction()
	{
		super(Messages.RefreshAction_Name, Activator.getImageDescriptor("icons/refresh.gif")); //$NON-NLS-1$
		setAccelerator(SWT.F5);
	}
}
