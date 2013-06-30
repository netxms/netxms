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
package org.netxms.ui.eclipse.epp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Add/edit attribute
 *
 */
public class AttributeEditDialog extends Dialog
{
	private LabeledText textName;
	private LabeledText textValue;
	private String attrName;
	private String attrValue;
	
	/**
	 * @param parentShell
	 */
	public AttributeEditDialog(Shell parentShell, String attrName, String attrValue)
	{
		super(parentShell);
		this.attrName = attrName;
		this.attrValue = attrValue;
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
      textName.setLabel(Messages.AttributeEditDialog_Name);
      textName.getTextControl().setTextLimit(63);
      if (attrName != null)
      {
      	textName.setText(attrName);
      	textName.getTextControl().setEditable(false);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textValue = new LabeledText(dialogArea, SWT.NONE);
      textValue.setLabel(Messages.AttributeEditDialog_Value);
      if (attrValue != null)
      	textValue.setText(attrValue);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textValue.setLayoutData(gd);
      
      if (attrName != null)
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
		newShell.setText((attrName == null) ? Messages.AttributeEditDialog_TitleAdd : Messages.AttributeEditDialog_TitleEdit);
	}
	
	/**
	 * Get attribute name
	 * 
	 */
	public String getAtributeName()
	{
		return attrName;
	}
	
	/**
	 * Get attribute value
	 * 
	 */
	public String getAttributeValue()
	{
		return attrValue;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		attrName = textName.getText();
		attrValue = textValue.getText();
		super.okPressed();
	}
}
