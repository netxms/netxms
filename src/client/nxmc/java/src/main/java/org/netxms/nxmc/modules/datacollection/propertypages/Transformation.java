/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.Arrays;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.TestTransformationDlg;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Transformation" property page for DCI
 */
public class Transformation extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Transformation.class);

   private static final String[] DCI_FUNCTIONS = { "FindDCIByName", "FindDCIByDescription", "GetDCIObject", "GetDCIValue", "GetDCIValueByDescription", "GetDCIValueByName" };
   private static final String[] DCI_VARIABLES = { "$dci", "$node" };

   private LabeledCombo transformedDataType;
   private LabeledCombo deltaCalculation;
	private ScriptEditor transformationScript;
	private Button testScriptButton;

   /**
    * Create property page.
    *
    * @param editor data collection editor
    */
   public Transformation(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(Transformation.class).tr("Transformation"), editor);
   }
	
   /**
    * @see org.netxms.nxmc.modules.datacollection.propertypages.AbstractDCIPropertyPage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = (Composite)super.createContents(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      if (editor.getObject() instanceof DataCollectionItem)
      {
         transformedDataType = new LabeledCombo(dialogArea, SWT.NONE);
         transformedDataType.setLabel(i18n.tr("Data type after transformation"));
         transformedDataType.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         transformedDataType.add(i18n.tr("Same as original"));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT32));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT32));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER32));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT64));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT64));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER64));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.FLOAT));
         transformedDataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.STRING));
         transformedDataType.select(getDataTypePosition(editor.getObjectAsItem().getTransformedDataType()));

         deltaCalculation = new LabeledCombo(dialogArea, SWT.NONE);
         deltaCalculation.setLabel(i18n.tr("Step 1 - delta calculation"));
         deltaCalculation.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         deltaCalculation.add(i18n.tr("None (keep original value)"));
         deltaCalculation.add(i18n.tr("Simple delta"));
         deltaCalculation.add(i18n.tr("Average delta per second"));
         deltaCalculation.add(i18n.tr("Average delta per minute"));
         deltaCalculation.select(editor.getObjectAsItem().getDeltaCalculation());
      }

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new ScriptEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL, 
                  i18n.tr("Variables:\n\t$1\t\t\tvalue to transform (after delta calculation);\n\t$dci\t\t\tthis DCI object;\n\t$isCluster\ttrue if DCI is on cluster;\n\t$node\t\tcurrent node object (null if DCI is not on the node);\n\t$object\t\tcurrent object.\n\nReturn value: transformed DCI value or null to keep original value."));
         }
      };
      transformationScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory,
            (editor.getObject() instanceof DataCollectionItem) ? i18n.tr("Step 2 - transformation script") : i18n.tr("Transformation script"), gd);
      transformationScript.addFunctions(Arrays.asList(DCI_FUNCTIONS));
      transformationScript.addVariables(Arrays.asList(DCI_VARIABLES));
      transformationScript.setText(editor.getObject().getTransformationScript());

      if (editor.getObject() instanceof DataCollectionItem)
      {
         testScriptButton = new Button(transformationScript.getParent(), SWT.PUSH);
         testScriptButton.setText(i18n.tr("&Test..."));
         gd = new GridData();
         gd.horizontalAlignment = SWT.RIGHT;
         gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
         testScriptButton.setLayoutData(gd);
         testScriptButton.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               TestTransformationDlg dlg = new TestTransformationDlg(getShell(), editor.getObject(), transformationScript.getText());
               dlg.open();
            }
         });
      }

      return dialogArea;
   }

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		if (editor.getObject() instanceof DataCollectionItem)
      {
			editor.getObjectAsItem().setDeltaCalculation(deltaCalculation.getSelectionIndex());
         editor.getObjectAsItem().setTransformedDataType(getDataTypeByPosition(transformedDataType.getSelectionIndex()));
      }
      editor.getObject().setTransformationScript(transformationScript.getText());
      editor.modify();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		if (deltaCalculation != null)
			deltaCalculation.select(DataCollectionItem.DELTA_NONE);
      transformationScript.setText("");
	}

   /**
    * Get selector position for given data type
    * 
    * @param type data type
    * @return corresponding selector's position
    */
   private static int getDataTypePosition(DataType type)
   {
      switch(type)
      {
         case COUNTER32:
            return 3;
         case COUNTER64:
            return 4;
         case FLOAT:
            return 7;
         case INT32:
            return 1;
         case INT64:
            return 4;
         case STRING:
            return 8;
         case UINT32:
            return 2;
         case UINT64:
            return 5;
         default:
            return 0; // fallback to "no change"
      }
   }

   /**
    * Data type positions in selector
    */
   private static final DataType[] TYPES = { 
      DataType.NULL, DataType.INT32, DataType.UINT32, DataType.COUNTER32, DataType.INT64,
      DataType.UINT64, DataType.COUNTER64, DataType.FLOAT, DataType.STRING
   };

   /**
    * Get data type by selector position
    * 
    * @param position selector position
    * @return corresponding data type
    */
   private static DataType getDataTypeByPosition(int position)
   {
      return TYPES[position];
   }
}
