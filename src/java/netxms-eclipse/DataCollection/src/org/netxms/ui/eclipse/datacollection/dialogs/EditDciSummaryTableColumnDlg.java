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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for editing DCI summary table column
 */
public class EditDciSummaryTableColumnDlg extends Dialog
{
	private DciSummaryTableColumn column;
	private LabeledText name;
	private LabeledText dciName;
	private Button checkRegexpMatch;

	/**
	 * @param parentShell
	 * @param column
	 */
	public EditDciSummaryTableColumnDlg(Shell parentShell, DciSummaryTableColumn column)
	{
		super(parentShell);
		this.column = column;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
		
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(Messages.EditDciSummaryTableColumnDlg_Name);
      name.getTextControl().setTextLimit(127);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      name.setLayoutData(gd);
      name.setText(column.getName());
      
      dciName = new LabeledText(dialogArea, SWT.NONE);
      dciName.setLabel(Messages.EditDciSummaryTableColumnDlg_DciName);
      dciName.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      dciName.setLayoutData(gd);
      dciName.setText(column.getDciName());
      
      checkRegexpMatch = new Button(dialogArea, SWT.CHECK);
      checkRegexpMatch.setText(Messages.EditDciSummaryTableColumnDlg_UseRegExp);
      checkRegexpMatch.setSelection(column.isRegexpMatch());
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.EditDciSummaryTableColumnDlg_EditColumn);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		column.setName(name.getText());
		column.setDciName(dciName.getText());
		column.setRegexpMatch(checkRegexpMatch.getSelection());
		super.okPressed();
	}
}
