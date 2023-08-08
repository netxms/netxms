/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog box for custom time dialog
 */
public class CustomTimePeriodDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CustomTimePeriodDialog.class);
   private int time;
	private Label info;
	private LabeledSpinner days;
	private LabeledSpinner hours;
	private LabeledSpinner minutes;

	/**
    * Create dialog.
    *
    * @param parentShell parent shell
    */
	public CustomTimePeriodDialog(Shell parentShell)
	{
		super(parentShell);
      setShellStyle(SWT.DIALOG_TRIM | SWT.RESIZE | SWT.SYSTEM_MODAL);
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Set custom timeout"));
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
      days.setLabel(i18n.tr("Days"));
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      days.setLayoutData(gridData);
      days.setRange(0, 999);
      days.setSelection(0);
      
      hours = new LabeledSpinner(dialogArea, SWT.NONE);
      hours.setLabel(i18n.tr("Hours"));
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      hours.setLayoutData(gridData);
      hours.setRange(0, 23);
      hours.setSelection(0);
      
      minutes = new LabeledSpinner(dialogArea, SWT.NONE);
      minutes.setLabel(i18n.tr("Minutes"));
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      minutes.setLayoutData(gridData);
      minutes.setRange(0, 59);
      minutes.setSelection(0);

      info = new Label(dialogArea, SWT.WRAP);
      info.setText(i18n.tr("Custom periods can be configured in client preferences (in \"Alarms\" for sticky alarm acknowledgement and in \"Object Maintenance\" for maintenance)"));
      gridData = new GridData();
      gridData.horizontalSpan = 3;
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      info.setLayoutData(gridData);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		time = days.getSelection() * 86400 + hours.getSelection() * 3600 + minutes.getSelection() * 60;
		if (time <= 0)
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Time should be greater than zero."));
		   return;
		}
	   super.okPressed();
	}

	/**
    * Get time entered by user.
    *
    * @return time entered by user
    */
	public int getTime()
	{
		return time;
	}
}
