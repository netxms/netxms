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
package org.netxms.ui.eclipse.objectmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Set expected state for interface(s)
 */
public class SetInterfaceExpStateDlg extends Dialog
{
	private Combo comboStates;
	private int expectedState;
	
	/**
	 * @param parentShell
	 */
	public SetInterfaceExpStateDlg(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().SetInterfaceExpStateDlg_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText(Messages.get().SetInterfaceExpStateDlg_Label);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		label.setLayoutData(gd);
		
		comboStates = new Combo(dialogArea, SWT.BORDER | SWT.READ_ONLY);
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		comboStates.setLayoutData(gd);
		comboStates.add(Messages.get().SetInterfaceExpStateDlg_StateUp);
		comboStates.add(Messages.get().SetInterfaceExpStateDlg_StateDown);
		comboStates.add(Messages.get().SetInterfaceExpStateDlg_StateIgnore);
		comboStates.select(0);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		expectedState = comboStates.getSelectionIndex();
		super.okPressed();
	}

	/**
	 * @return the expectedState
	 */
	public int getExpectedState()
	{
		return expectedState;
	}
}
