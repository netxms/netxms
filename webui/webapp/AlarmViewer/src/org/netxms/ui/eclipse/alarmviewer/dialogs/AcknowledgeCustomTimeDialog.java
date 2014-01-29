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
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Dialog box for custom time dialog
 */
public class AcknowledgeCustomTimeDialog extends Dialog
{
   private int time;
	private Label info;
	private Label daysLabel;
	private Label hoursLabel;
	private Label minutesLabel;
	private Text daysText;
	private Text hoursText;
	private Text minutesText;
	
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
	   
	   dialogArea.setLayout(new GridLayout(2, false));
      
	   daysLabel = new Label(dialogArea, SWT.NONE);
	   daysLabel.setText(Messages.get().AcknowledgeCustomTimeDialog_Days);
      
	   daysText = new Text(dialogArea, SWT.BORDER);
      GridData gridData = new GridData();
      gridData.widthHint = 50;
      gridData.grabExcessHorizontalSpace = true;
      daysText.setLayoutData(gridData);
      daysText.setText("0"); //$NON-NLS-1$
      
      hoursLabel = new Label(dialogArea, SWT.NONE);
      hoursLabel.setText(Messages.get().AcknowledgeCustomTimeDialog_Hours);
      
      hoursText = new Text(dialogArea, SWT.BORDER);
      gridData = new GridData();
      gridData.widthHint = 50;
      gridData.grabExcessHorizontalSpace = true;
      hoursText.setLayoutData(gridData);
      hoursText.setText("0"); //$NON-NLS-1$
      
      minutesLabel = new Label(dialogArea, SWT.NONE);
      minutesLabel.setText(Messages.get().AcknowledgeCustomTimeDialog_Minutes);
      
      minutesText = new Text(dialogArea, SWT.BORDER);
      gridData = new GridData();
      gridData.widthHint = 50;
      gridData.grabExcessHorizontalSpace = true;
      minutesText.setLayoutData(gridData);
      minutesText.setText("0"); //$NON-NLS-1$
      
      info = new Label(dialogArea, SWT.WRAP);
      info.setText(Messages.get().AcknowledgeCustomTimeDialog_ConfigurationInfoLabel);
      gridData = new GridData();
      gridData.horizontalSpan = 2;
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
		time = 0;
		try
      {
		   if (minutesText.getText() != "") //$NON-NLS-1$
		      time += Integer.parseInt(minutesText.getText())*60;
		   if (hoursText.getText() != "") //$NON-NLS-1$
		      time += Integer.parseInt(hoursText.getText())*60*60;
		   if (daysText.getText() != "") //$NON-NLS-1$
		      time += Integer.parseInt(daysText.getText())*24*60*60;
      }
      catch(NumberFormatException e)
      {
         MessageDialogHelper.openWarning(getShell(), Messages.get().AcknowledgeCustomTimeDialog_Waring, Messages.get().AcknowledgeCustomTimeDialog_WarningOnlyDigits);
         return;
      }
		
		if(time <= 0){
		   MessageDialogHelper.openWarning(getShell(), Messages.get().AcknowledgeCustomTimeDialog_Waring, Messages.get().AcknowledgeCustomTimeDialog_WarningTimeGatherThen0);
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
