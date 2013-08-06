/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;

/**
 * Table threshold editing dialog
 */
public class EditTableThresholdDialog extends Dialog
{
	private TableThreshold threshold;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	
	/**
	 * @param parentShell
	 * @param threshold
	 */
	public EditTableThresholdDialog(Shell parentShell, TableThreshold threshold)
	{
		super(parentShell);
		this.threshold = threshold;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		activationEvent = new EventSelector(dialogArea, SWT.NONE);
		activationEvent.setEventCode(threshold.getActivationEvent());
		
		deactivationEvent = new EventSelector(dialogArea, SWT.NONE);
		deactivationEvent.setEventCode(threshold.getDeactivationEvent());
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		threshold.setActivationEvent((int)activationEvent.getEventCode());
		threshold.setDeactivationEvent((int)deactivationEvent.getEventCode());
		super.okPressed();
	}	
}
