/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing DCI summary table column
 */
public class EditDciSummaryTableColumnDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditDciSummaryTableColumnDlg.class);
   
	private DciSummaryTableColumn column;
	private LabeledText name;
	private LabeledText dciName;
   private Button checkDescriptionMatch;
   private Button checkRegexpMatch;
   private Button checkMultivalued;
   private LabeledText separator;

	/**
	 * @param parentShell
	 * @param column
	 */
	public EditDciSummaryTableColumnDlg(Shell parentShell, DciSummaryTableColumn column)
	{
		super(parentShell);
		this.column = column;
	}

   /**
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
      name.setLabel(i18n.tr("Name"));
      name.getTextControl().setTextLimit(127);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      name.setLayoutData(gd);
      name.setText(column.getName());
      
      dciName = new LabeledText(dialogArea, SWT.NONE);
      dciName.setLabel(i18n.tr("DCI name"));
      dciName.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      dciName.setLayoutData(gd);
      dciName.setText(column.getDciName());
      
      checkDescriptionMatch = new Button(dialogArea, SWT.CHECK);
      checkDescriptionMatch.setText("Match by description instead of name");
      checkDescriptionMatch.setSelection(column.isDescriptionMatch());

      checkRegexpMatch = new Button(dialogArea, SWT.CHECK);
      checkRegexpMatch.setText(i18n.tr("Use regular expression for name or description matching"));
      checkRegexpMatch.setSelection(column.isRegexpMatch());

      checkMultivalued = new Button(dialogArea, SWT.CHECK);
      checkMultivalued.setText("&Multivalued column");
      checkMultivalued.setSelection(column.isMultivalued());
      checkMultivalued.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            separator.setEnabled(checkMultivalued.getSelection());
         }
      });
      
      separator = new LabeledText(dialogArea, SWT.NONE);
      separator.setLabel("&Separator for multiple values");
      separator.getTextControl().setTextLimit(15);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      separator.setLayoutData(gd);
      separator.setText(column.getSeparator());
      separator.setEnabled(column.isMultivalued());
      
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("Edit Column"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		column.setName(name.getText());
		column.setDciName(dciName.getText());
      column.setDescriptionMatch(checkDescriptionMatch.getSelection());
		column.setRegexpMatch(checkRegexpMatch.getSelection());
      column.setMultivalued(checkMultivalued.getSelection());
      column.setSeparator(separator.getText().trim());
		super.okPressed();
	}
}
