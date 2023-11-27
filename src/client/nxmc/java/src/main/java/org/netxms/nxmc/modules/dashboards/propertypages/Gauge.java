/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Gauge properties
 */
public class Gauge extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Gauge.class);

	private GaugeConfig config;
	private Combo gaugeType;
	private LabeledText fontName;
   private LabeledSpinner fontSize;
   private LabeledSpinner expectedTextWidth;
	private Button checkLegendInside;
	private Button checkVertical;
	private Button checkElementBorders;
	private LabeledText minValue;
	private LabeledText maxValue;
	private LabeledText leftYellowZone;
	private LabeledText leftRedZone;
	private LabeledText rightYellowZone;
	private LabeledText rightRedZone;
	private Combo colorMode;
	private ColorSelector customColor;
	private ObjectSelector drillDownObject;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public Gauge(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(Gauge.class).tr("Gauge"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "gauge";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof GaugeConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (GaugeConfig)elementConfig;
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
      gaugeType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Gauge type"), gd);
      gaugeType.add(i18n.tr("Dial"));
      gaugeType.add(i18n.tr("Bar"));
      gaugeType.add(i18n.tr("Text"));
      gaugeType.add(i18n.tr("Circular"));
		gaugeType.select(config.getGaugeType());

		fontName = new LabeledText(dialogArea, SWT.NONE);
      fontName.setLabel(i18n.tr("Font name (leave empty to use default)"));
		fontName.setText(config.getFontName());
		gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		fontName.setLayoutData(gd);

      fontSize = new LabeledSpinner(dialogArea, SWT.NONE);
      fontSize.setLabel(i18n.tr("Font size (0 for autoscale)"));
      fontSize.setRange(0, 72);
      fontSize.setSelection(config.getFontSize());
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      fontSize.setLayoutData(gd);

      expectedTextWidth = new LabeledSpinner(dialogArea, SWT.NONE);
      expectedTextWidth.setLabel(i18n.tr("Expected text width (0 for autoscale)"));
      expectedTextWidth.setRange(0, 1000);
      expectedTextWidth.setSelection(config.getExpectedTextWidth());
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      expectedTextWidth.setLayoutData(gd);

		minValue = new LabeledText(dialogArea, SWT.NONE);
      minValue.setLabel(i18n.tr("Minimum value"));
		minValue.setText(Double.toString(config.getMinValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		minValue.setLayoutData(gd);

		maxValue = new LabeledText(dialogArea, SWT.NONE);
      maxValue.setLabel(i18n.tr("Maximum value"));
		maxValue.setText(Double.toString(config.getMaxValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		maxValue.setLayoutData(gd);

		leftRedZone = new LabeledText(dialogArea, SWT.NONE);
      leftRedZone.setLabel(i18n.tr("Left red zone end"));
		leftRedZone.setText(Double.toString(config.getLeftRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftRedZone.setLayoutData(gd);

		leftYellowZone = new LabeledText(dialogArea, SWT.NONE);
      leftYellowZone.setLabel(i18n.tr("Left yellow zone end"));
		leftYellowZone.setText(Double.toString(config.getLeftYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftYellowZone.setLayoutData(gd);

		rightYellowZone = new LabeledText(dialogArea, SWT.NONE);
      rightYellowZone.setLabel(i18n.tr("Right yellow zone start"));
		rightYellowZone.setText(Double.toString(config.getRightYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightYellowZone.setLayoutData(gd);

		rightRedZone = new LabeledText(dialogArea, SWT.NONE);
      rightRedZone.setLabel(i18n.tr("Right red zone start"));
		rightRedZone.setText(Double.toString(config.getRightRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightRedZone.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      colorMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Color mode"), gd);
      colorMode.add(i18n.tr("Zone color"));
      colorMode.add(i18n.tr("Fixed custom color"));
      colorMode.add(i18n.tr("Active threshold color"));
      colorMode.add(i18n.tr("Data source color"));
      colorMode.select(config.getColorMode());
      colorMode.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onColorModeChange(colorMode.getSelectionIndex());
         }
      });

      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      customColor = WidgetHelper.createLabeledColorSelector(dialogArea, i18n.tr("Custom color"), gd);
      customColor.setColorValue(ColorConverter.rgbFromInt(config.getCustomColor()));

      drillDownObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      drillDownObject.setLabel(i18n.tr("Drill-down object"));
      drillDownObject.setObjectClass(AbstractObject.class);
      drillDownObject.setClassFilter(ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
      drillDownObject.setObjectId(config.getDrillDownObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      drillDownObject.setLayoutData(gd);

		checkLegendInside = new Button(dialogArea, SWT.CHECK);
      checkLegendInside.setText(i18n.tr("Place legend &inside dial"));
		checkLegendInside.setSelection(config.isLegendInside());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkLegendInside.setLayoutData(gd);

		checkVertical = new Button(dialogArea, SWT.CHECK);
      checkVertical.setText(i18n.tr("&Vertical orientation"));
		checkVertical.setSelection(config.isVertical());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkVertical.setLayoutData(gd);

      checkElementBorders = new Button(dialogArea, SWT.CHECK);
      checkElementBorders.setText(i18n.tr("Show &border around each element"));
      checkElementBorders.setSelection(config.isElementBordersVisible());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      checkElementBorders.setLayoutData(gd);

      onColorModeChange(config.getColorMode());
		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		double min, max, ly, lr, ry, rr;
		try
		{
			min = Double.parseDouble(minValue.getText().trim());
			max = Double.parseDouble(maxValue.getText().trim());
			ly = Double.parseDouble(leftYellowZone.getText().trim());
			lr = Double.parseDouble(leftRedZone.getText().trim());
			ry = Double.parseDouble(rightYellowZone.getText().trim());
			rr = Double.parseDouble(rightRedZone.getText().trim());
		}
		catch(NumberFormatException e)
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter correct number"));
			return false;
		}

		config.setGaugeType(gaugeType.getSelectionIndex());
		config.setFontName(fontName.getText().trim());
      config.setFontSize(fontSize.getSelection());
      config.setExpectedTextWidth(expectedTextWidth.getSelection());
		config.setMinValue(min);
		config.setMaxValue(max);
		config.setLeftYellowZone(ly);
		config.setLeftRedZone(lr);
		config.setRightYellowZone(ry);
		config.setRightRedZone(rr);
		config.setLegendInside(checkLegendInside.getSelection());
		config.setVertical(checkVertical.getSelection());
		config.setElementBordersVisible(checkElementBorders.getSelection());
		config.setColorMode(colorMode.getSelectionIndex());
		config.setCustomColor(ColorConverter.rgbToInt(customColor.getColorValue()));
		config.setDrillDownObjectId(drillDownObject.getObjectId());
		return true;
	}

	/**
	 * Handle color mode change
	 * 
	 * @param newMode
	 */
	private void onColorModeChange(int newMode)
	{
	   leftYellowZone.setEnabled(newMode == GaugeColorMode.ZONE.getValue());
	   leftRedZone.setEnabled(newMode == GaugeColorMode.ZONE.getValue());
      rightYellowZone.setEnabled(newMode == GaugeColorMode.ZONE.getValue());
      rightRedZone.setEnabled(newMode == GaugeColorMode.ZONE.getValue());
	   customColor.setEnabled(newMode == GaugeColorMode.CUSTOM.getValue());
	}
}
