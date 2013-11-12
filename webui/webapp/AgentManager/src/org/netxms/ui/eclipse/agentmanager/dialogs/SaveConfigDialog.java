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
package org.netxms.ui.eclipse.agentmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Shows prompt for saving agent's config
 *
 */
public class SaveConfigDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	public static final int SAVE_ID = 100;
	public static final int SAVE_AND_APPLY_ID = 101;
	public static final int DISCARD_ID = 102;
	
	/**
	 * @param parentShell
	 */
	public SaveConfigDialog(Shell parentShell)
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
		newShell.setText(Messages.get().SaveConfigDialog_UnsavedChanges);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		createButton(parent, SAVE_ID, Messages.get().SaveConfigDialog_Save, false);
		createButton(parent, SAVE_AND_APPLY_ID, Messages.get().SaveConfigDialog_SaveApply, false);
		createButton(parent, DISCARD_ID, Messages.get().SaveConfigDialog_Discard, false);
		createButton(parent, IDialogConstants.CANCEL_ID, Messages.get().SaveConfigDialog_Cancel, false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		final Label image = new Label(dialogArea, SWT.NONE);
		image.setImage(Activator.getImageDescriptor("icons/unsaved_config.png").createImage()); //$NON-NLS-1$
		
		final CLabel text = new CLabel(dialogArea, SWT.LEFT);
		text.setText(Messages.get().SaveConfigDialog_ModifiedMessage);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		text.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
	 */
	@Override
	protected void buttonPressed(int buttonId)
	{
		if (buttonId >= 100)
		{
			setReturnCode(buttonId);
			close();
		}
		super.buttonPressed(buttonId);
	}

}
