/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.server.ServerVariable;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Create/edit variable
 */
public class VariableEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(VariableEditDialog.class);
   
	private LabeledText textName;
	private Text textValue;
	private Spinner spinnerValue;
   private Button trueValue;
   private Button falseValue;
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((variable.getName() == null) ? i18n.tr("Create Variable") : i18n.tr("Edit Variable"));
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

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
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
      
      if (!variable.getUnit().isEmpty())
      {
        layout.numColumns = 2;
        gd.horizontalSpan = 2;
      }

      switch(variable.getDataType())
      {
         case BOOLEAN:
            trueValue = new Button(dialogArea, SWT.RADIO);
            trueValue.setText("&True");
            trueValue.setSelection(variable.getValueAsBoolean());

            falseValue = new Button(dialogArea, SWT.RADIO);
            falseValue.setText("&False");
            falseValue.setSelection(!trueValue.getSelection());

            if (variable.getName() != null)
               trueValue.setFocus();
            break;
         case CHOICE:
            valueMap = new HashMap<Integer, String>();
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            comboValue = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Value"), gd);
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
            colorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, i18n.tr("Value"), WidgetHelper.DEFAULT_LAYOUT_DATA);
            if (!variable.getValue().isEmpty())
               colorSelector.setColorValue(ColorConverter.parseColorDefinition(variable.getValue()));
            break;
         case INTEGER:
            spinnerValue = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Value"), 0,
                                                               (variable.getPossibleValues().isEmpty()) ? 0x7FFFFFFF : Integer.parseInt((String)variable.getPossibleValues().keySet().toArray()[0]),
                                                                     WidgetHelper.DEFAULT_LAYOUT_DATA);
            spinnerValue.setSelection(variable.getValueAsInt());
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            spinnerValue.setLayoutData(gd);

            if (!variable.getUnit().isEmpty())
            {
               Label unit = new Label(dialogArea, SWT.NONE);
               unit.setText(variable.getUnit());
               gd = new GridData();
                gd.grabExcessHorizontalSpace = true;
               gd.verticalIndent = 15;
               gd.horizontalAlignment = SWT.FILL;
               unit.setLayoutData(gd);
            }

            if (variable.getName() != null)
               spinnerValue.setFocus();
            break;
         case PASSWORD:
         case STRING:
            textValue = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, i18n.tr("Value"), 
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

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
	   name = textName.getText();
	   switch(variable.getDataType())
	   {
	      case BOOLEAN:
            value = trueValue.getSelection() ? "1" : "0";
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
         case PASSWORD:
	      case STRING:
	         value = textValue.getText();
	         break;
	   }
		super.okPressed();
	}
}
