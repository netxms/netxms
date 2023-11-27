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
package org.netxms.nxmc.modules.dashboards.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Status indicator element properties
 */
public class StatusIndicator extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(StatusIndicator.class);

	private StatusIndicatorConfig config;
   private TitleConfigurator title;
   private LabeledSpinner numColumns;
   private Combo shape;
   private Combo labelType;
	private Button checkFullColors;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public StatusIndicator(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(StatusIndicator.class).tr("Status Indicator"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "status-indicator";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof StatusIndicatorConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 0;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (StatusIndicatorConfig)elementConfig;

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

      shape = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Shape"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      shape.add(i18n.tr("Circle"));
      shape.add(i18n.tr("Rectangle"));
      shape.add(i18n.tr("Rounded rectangle"));
      shape.select(config.getShape());

      labelType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Label"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      labelType.add(i18n.tr("None"));
      labelType.add(i18n.tr("Inside"));
      labelType.add(i18n.tr("Outside"));
      labelType.select(config.getLabelType());

      numColumns = new LabeledSpinner(dialogArea, SWT.NONE);
      numColumns.setLabel("Columns");
      numColumns.setRange(1, 64);
      numColumns.setSelection(config.getNumColumns());

		checkFullColors = new Button(dialogArea, SWT.CHECK);
      checkFullColors.setText(i18n.tr("Use &full status color range"));
		checkFullColors.setSelection(config.isFullColorRange());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
      config.setLabelType(labelType.getSelectionIndex());
      config.setShape(shape.getSelectionIndex());
      config.setNumColumns(numColumns.getSelection());
		config.setFullColorRange(checkFullColors.getSelection());
		return true;
	}
}
