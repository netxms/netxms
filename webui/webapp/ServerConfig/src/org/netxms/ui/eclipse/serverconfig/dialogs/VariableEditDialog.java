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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Create/edit variable
 */
public class VariableEditDialog extends Dialog
{
	private LabeledText textName;
	private LabeledText textValue;
	private String varName;
	private String varValue;
	
	/**
	 * @param parentShell
	 */
	public VariableEditDialog(Shell parentShell, String varName, String varValue)
	{
		super(parentShell);
		this.varName = varName;
		this.varValue = varValue;
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
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel("Name");
      textName.getTextControl().setTextLimit(63);
      if (varName != null)
      {
      	textName.setText(varName);
      	textName.getTextControl().setEditable(false);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textValue = new LabeledText(dialogArea, SWT.NONE);
      textValue.setLabel("Value");
      textValue.getTextControl().setTextLimit(255);
      if (varValue != null)
      	textValue.setText(varValue);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textValue.setLayoutData(gd);
      
      if (varName != null)
      	textValue.setFocus();
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((varName == null) ? "Create Variable" : "Edit Variable");
	}
	
	/**
	 * Get variable name
	 * 
	 */
	public String getVarName()
	{
		return varName;
	}
	
	/**
	 * Get variable value
	 * 
	 */
	public String getVarValue()
	{
		return varValue;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		varName = textName.getText();
		varValue = textValue.getText();
		super.okPressed();
	}
}
