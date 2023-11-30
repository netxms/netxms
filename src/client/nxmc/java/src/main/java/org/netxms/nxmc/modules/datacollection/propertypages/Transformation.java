/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.TestTransformationDlg;
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

	private Combo deltaCalculation;
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
         deltaCalculation = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Step 1 - delta calculation"), WidgetHelper.DEFAULT_LAYOUT_DATA);
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
            return new ScriptEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL, true,
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
         testScriptButton.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               TestTransformationDlg dlg = new TestTransformationDlg(getShell(), editor.getObject(), transformationScript.getText());
               dlg.open();
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
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
			editor.getObjectAsItem().setDeltaCalculation(deltaCalculation.getSelectionIndex());
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
}
