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
package org.netxms.ui.eclipse.alarmviewer.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * Dialog box for custom time dialog
 */
public class AcknowledgeCustomTimeDialog extends Dialog
{
   private int time;
	private Label info;
	private LabeledSpinner days;
	private LabeledSpinner hours;
	private LabeledSpinner minutes;
	
	/**
	 * @param parentShell
	 */
	public AcknowledgeCustomTimeDialog(Shell parentShell)
	{
		super(parentShell);
      setShellStyle(SWT.DIALOG_TRIM | SWT.RESIZE | SWT.SYSTEM_MODAL);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().AcknowledgeCustomTimeDialog_CustomTimeDialogTitle);
		newShell.setSize(350, 220);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createDialogArea(parent);
	   
	   GridLayout layout = new GridLayout();
	   layout.numColumns = 3;
	   layout.makeColumnsEqualWidth = true;
	   layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
	   layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
	   layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
	   layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
	   dialogArea.setLayout(layout);
      
	   days = new LabeledSpinner(dialogArea, SWT.NONE);
	   days.setLabel(Messages.get().AcknowledgeCustomTimeDialog_Days);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      days.setLayoutData(gridData);
      days.setRange(0, 999);
      days.setSelection(0);
      
      hours = new LabeledSpinner(dialogArea, SWT.NONE);
      hours.setLabel(Messages.get().AcknowledgeCustomTimeDialog_Hours);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      hours.setLayoutData(gridData);
      hours.setRange(0, 23);
      hours.setSelection(0);
      
      minutes = new LabeledSpinner(dialogArea, SWT.NONE);
      minutes.setLabel(Messages.get().AcknowledgeCustomTimeDialog_Minutes);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      minutes.setLayoutData(gridData);
      minutes.setRange(0, 59);
      minutes.setSelection(0);
      
      info = new Label(dialogArea, SWT.WRAP);
      info.setText(Messages.get().AcknowledgeCustomTimeDialog_ConfigurationInfoLabel);
      gridData = new GridData();
      gridData.horizontalSpan = 3;
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      info.setLayoutData(gridData);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		time = days.getSelection() * 86400 + hours.getSelection() * 3600 + minutes.getSelection() * 60;
		if (time <= 0)
		{
		   MessageDialogHelper.openWarning(getShell(), Messages.get().AcknowledgeCustomTimeDialog_Warning, Messages.get().AcknowledgeCustomTimeDialog_WarningZeroTime);
		   return;
		}
	   super.okPressed();
	}

	/**
	 * @return the text
	 */
	public int getTime()
	{
		return time;
	}
}
