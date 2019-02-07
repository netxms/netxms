/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import java.util.Map;
import java.util.Map.Entry;
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
import org.netxms.client.server.ServerVariable;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
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
	private ServerVariable variable;
	private Map<Integer, String> valueMap;
	private String name;
	private String value;
	
	/**
	 * @param parentShell
	 */
	public VariableEditDialog(Shell parentShell, ServerVariable variable)
	{
		super(parentShell);
		this.variable  = variable;
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
      if (variable.getName() != null)
      {
      	textName.setText(variable.getName());
      	textName.getTextControl().setEditable(false);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      switch(variable.getDataType())
      {
         case BOOLEAN:
            buttonEnable = new Button(dialogArea, SWT.CHECK);
            buttonEnable.setText("Enable");
            buttonEnable.setSelection(variable.getValueAsBoolean());
            
            if (variable.getName() != null)
               buttonEnable.setFocus();
            break;
         case CHOICE:
            valueMap = new HashMap<Integer, String>();
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            comboValue = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().VariableEditDialog_Value, gd);
            for(Entry<String, String> e : variable.getPossibleValues().entrySet())
            {
               comboValue.add(e.getValue());
               int index = comboValue.getItemCount() - 1;
               valueMap.put(index, e.getKey());
               if (e.getKey().equals(variable.getValue()))
                  comboValue.select(index);
            }
            
            if (variable.getName() != null)
               comboValue.setFocus();
            break;
         case COLOR:
            colorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, Messages.get().VariableEditDialog_Value, WidgetHelper.DEFAULT_LAYOUT_DATA);
            if (!variable.getValue().isEmpty())
               colorSelector.setColorValue(ColorConverter.parseColorDefinition(variable.getValue()));
            break;
         case INTEGER:
            spinnerValue = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().VariableEditDialog_Value, 0,
                                                               (variable.getPossibleValues().isEmpty()) ? 0xffffff : Integer.parseInt((String)variable.getPossibleValues().keySet().toArray()[0]),
                                                                     WidgetHelper.DEFAULT_LAYOUT_DATA);
            spinnerValue.setSelection(variable.getValueAsInt());
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            spinnerValue.setLayoutData(gd);
            
            if (variable.getName() != null)
               spinnerValue.setFocus();
            break;
         case STRING:
            textValue = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.get().VariableEditDialog_Value, 
                  variable.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
            textValue.setTextLimit(2000);
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            textValue.setLayoutData(gd);
            
            if (variable.getName() != null)
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
		newShell.setText((variable.getName() == null) ? Messages.get().VariableEditDialog_TitleCreate : Messages.get().VariableEditDialog_TitleEdit);
	}
	
	/**
	 * Get variable name
	 * 
	 */
	public String getVarName()
	{
		return name;
	}
	
	/**
	 * Get variable value
	 * 
	 */
	public String getVarValue()
	{
		return value;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   name = textName.getText();
	   switch(variable.getDataType())
	   {
	      case BOOLEAN:
	         value = (buttonEnable.getSelection()) ? "1" : "0";
	         break;
	      case CHOICE:
	         if (comboValue.getSelectionIndex() == -1)
	         {
	            MessageDialogHelper.openWarning(getShell(), "Warning", "Please select valid value and then press OK");
	            return;
	         }
	         value = valueMap.get(comboValue.getSelectionIndex());
	         break;
	      case COLOR:
	         if (colorSelector.getColorValue() != null)
	            value = ColorConverter.rgbToCss(colorSelector.getColorValue());
	         break;
	      case INTEGER:
	         value = spinnerValue.getText();
            break;
	      case STRING:
	         value = textValue.getText();
	         break;
	   }
		super.okPressed();
	}
}
