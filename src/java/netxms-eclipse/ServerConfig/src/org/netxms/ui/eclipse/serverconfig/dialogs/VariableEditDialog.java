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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;


/**
 * @author victor
 *
 */
public class VariableEditDialog extends Dialog
{
	private Text textName;
	private Text textValue;
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
		
		FillLayout layout = new FillLayout();
      layout.type = SWT.VERTICAL;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Name");
      
      textName = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textName.setTextLimit(63);
      if (varName != null)
      {
      	textName.setText(varName);
      	textName.setEditable(false);
      }
      
      label = new Label(dialogArea, SWT.NONE);
      label.setText("");

      label = new Label(dialogArea, SWT.NONE);
      label.setText("Value");

      textValue = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
      textValue.setTextLimit(255);
      textValue.getShell().setMinimumSize(300, 0);
      if (varValue != null)
      	textValue.setText(varValue);
      
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
