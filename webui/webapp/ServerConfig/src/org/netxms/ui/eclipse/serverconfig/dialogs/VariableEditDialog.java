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

import java.util.HashMap;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.constants.ServerVariableDataType;
import org.netxms.client.server.ServerVariable;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Create/edit variable
 */
public class VariableEditDialog extends Dialog
{
	private LabeledText textName;
	private Text textValue;
	private Spinner spinnerValue;
	private Button buttonEnable;
	private ColorSelector colorSelector;
	private Combo comboValue;
	private String varName;
	private String varValue;
	private ServerVariableDataType varDataType;
	private HashMap<String, String> possibleValues;
	
	/**
	 * @param parentShell
	 */
	public VariableEditDialog(Shell parentShell, ServerVariable var)
	{
		super(parentShell);
		this.varName = var.getName();
		this.varValue = var.getValue();
		this.varDataType = var.getDataType();
		possibleValues = var.getPossibleValues();
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
      textName.setLabel(Messages.get().VariableEditDialog_Name);
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
      
      switch(varDataType)
      {
         case BOOLEAN:
            buttonEnable = new Button(dialogArea, SWT.CHECK);
            buttonEnable.setText("Enable");
            buttonEnable.setSelection(varValue.equals("1"));
            
            if (varName != null)
               buttonEnable.setFocus();
            break;
         case CHOICE:
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            comboValue = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER, Messages.get().VariableEditDialog_Value, gd);
            comboValue.setItems(possibleValues.values().toArray(new String[possibleValues.size()]));
            comboValue.setText(possibleValues.get(varValue));
            
            if (varName != null)
               comboValue.setFocus();
            break;
         case COLOR:
            colorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, Messages.get().VariableEditDialog_Value, WidgetHelper.DEFAULT_LAYOUT_DATA);
            if (!varValue.isEmpty())
               colorSelector.setColorValue(ColorConverter.parseColorDefinition(varValue));
            break;
         case INTEGER:
            spinnerValue = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().VariableEditDialog_Value, 0,
                                                               (possibleValues.isEmpty()) ? 0xffffff : Integer.parseInt((String)possibleValues.keySet().toArray()[0]),
                                                                     WidgetHelper.DEFAULT_LAYOUT_DATA);
            spinnerValue.setSelection(Integer.parseInt(varValue));
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            spinnerValue.setLayoutData(gd);
            
            if (varName != null)
               spinnerValue.setFocus();
            break;
         case STRING:
            textValue = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.get().VariableEditDialog_Value, 
                                                      (varValue != null ? varValue : ""), WidgetHelper.DEFAULT_LAYOUT_DATA);
            textValue.setTextLimit(2000);
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            textValue.setLayoutData(gd);
            
            if (varName != null)
               textValue.setFocus();
            break;
      }
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((varName == null) ? Messages.get().VariableEditDialog_TitleCreate : Messages.get().VariableEditDialog_TitleEdit);
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
	   switch (varDataType)
	   {
	      case BOOLEAN:
	         varValue = (buttonEnable.getSelection()) ? "1" : "0";
	         break;
	      case CHOICE:
	         varValue = Integer.toString(comboValue.getSelectionIndex());
	         if (varValue.equals("-1"))
	            varValue = "0";
	         break;
	      case COLOR:
	         if (colorSelector.getColorValue() != null)
	            varValue = ColorConverter.rgbToCss(colorSelector.getColorValue());
	         break;
	      case INTEGER:
            varValue = spinnerValue.getText();
            break;
	      case STRING:
	         varValue = textValue.getText();
	         break;
	   }
		super.okPressed();
	}
}
