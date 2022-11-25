/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * Status indicator element properties
 */
public class StatusIndicator extends PropertyPage
{
	private StatusIndicatorConfig config;
   private TitleConfigurator title;
   private LabeledSpinner numColumns;
   private Combo shape;
   private Combo labelType;
	private Button checkFullColors;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = getElement().getAdapter(StatusIndicatorConfig.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      title.setLayoutData(gd);

      shape = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Shape", WidgetHelper.DEFAULT_LAYOUT_DATA);
      shape.add("Circle");
      shape.add("Rectangle");
      shape.add("Rounded rectangle");
      shape.select(config.getShape());

      labelType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Label", WidgetHelper.DEFAULT_LAYOUT_DATA);
      labelType.add("None");
      labelType.add("Inside");
      labelType.add("Outside");
      labelType.select(config.getLabelType());

      numColumns = new LabeledSpinner(dialogArea, SWT.NONE);
      numColumns.setLabel("Columns");
      numColumns.setRange(1, 64);
      numColumns.setSelection(config.getNumColumns());

		checkFullColors = new Button(dialogArea, SWT.CHECK);
		checkFullColors.setText(Messages.get().StatusIndicator_UseFullColorRange);
		checkFullColors.setSelection(config.isFullColorRange());

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
      title.updateConfiguration(config);
      config.setLabelType(labelType.getSelectionIndex());
      config.setShape(shape.getSelectionIndex());
      config.setNumColumns(numColumns.getSelection());
		config.setFullColorRange(checkFullColors.getSelection());
		return true;
	}
}
