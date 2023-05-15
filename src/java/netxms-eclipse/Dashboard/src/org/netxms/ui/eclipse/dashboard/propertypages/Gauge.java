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
package org.netxms.ui.eclipse.dashboard.propertypages;

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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.charts.api.GaugeColorMode;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GaugeConfig;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Gauge properties
 */
public class Gauge extends PropertyPage
{
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
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (GaugeConfig)getElement().getAdapter(GaugeConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		gaugeType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().Gauge_Type, gd);
		gaugeType.add(Messages.get().Gauge_Dial);
		gaugeType.add(Messages.get().Gauge_Bar);
		gaugeType.add(Messages.get().Gauge_Text);
      gaugeType.add("Circular");
		gaugeType.select(config.getGaugeType());

		fontName = new LabeledText(dialogArea, SWT.NONE);
		fontName.setLabel(Messages.get().Gauge_FontName);
		fontName.setText(config.getFontName());
		gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		fontName.setLayoutData(gd);

      fontSize = new LabeledSpinner(dialogArea, SWT.NONE);
      fontSize.setLabel("Font size (0 for autoscale)");
      fontSize.setRange(0, 72);
      fontSize.setSelection(config.getFontSize());
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      fontSize.setLayoutData(gd);

      expectedTextWidth = new LabeledSpinner(dialogArea, SWT.NONE);
      expectedTextWidth.setLabel("Expected text width (0 for autoscale)");
      expectedTextWidth.setRange(0, 1000);
      expectedTextWidth.setSelection(config.getExpectedTextWidth());
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      expectedTextWidth.setLayoutData(gd);

		minValue = new LabeledText(dialogArea, SWT.NONE);
		minValue.setLabel(Messages.get().DialChart_MinVal);
		minValue.setText(Double.toString(config.getMinValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		minValue.setLayoutData(gd);

		maxValue = new LabeledText(dialogArea, SWT.NONE);
		maxValue.setLabel(Messages.get().DialChart_MaxVal);
		maxValue.setText(Double.toString(config.getMaxValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		maxValue.setLayoutData(gd);

		leftRedZone = new LabeledText(dialogArea, SWT.NONE);
		leftRedZone.setLabel(Messages.get().DialChart_LeftRed);
		leftRedZone.setText(Double.toString(config.getLeftRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftRedZone.setLayoutData(gd);

		leftYellowZone = new LabeledText(dialogArea, SWT.NONE);
		leftYellowZone.setLabel(Messages.get().DialChart_LeftYellow);
		leftYellowZone.setText(Double.toString(config.getLeftYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftYellowZone.setLayoutData(gd);

		rightYellowZone = new LabeledText(dialogArea, SWT.NONE);
		rightYellowZone.setLabel(Messages.get().DialChart_RightYellow);
		rightYellowZone.setText(Double.toString(config.getRightYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightYellowZone.setLayoutData(gd);

		rightRedZone = new LabeledText(dialogArea, SWT.NONE);
		rightRedZone.setLabel(Messages.get().DialChart_RightRed);
		rightRedZone.setText(Double.toString(config.getRightRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightRedZone.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      colorMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().Gauge_ColorMode, gd);
      colorMode.add(Messages.get().Gauge_ZoneColor);
      colorMode.add(Messages.get().Gauge_FixedCustomColor);
      colorMode.add(Messages.get().Gauge_ActiveThresholdColor);
      colorMode.add("Data source color");
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
      customColor = WidgetHelper.createLabeledColorSelector(dialogArea, Messages.get().Gauge_CustomColor, gd); 
      customColor.setColorValue(ColorConverter.rgbFromInt(config.getCustomColor()));

      drillDownObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      drillDownObject.setLabel("Drill-down object");
      drillDownObject.setObjectClass(AbstractObject.class);
      drillDownObject.setClassFilter(ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
      drillDownObject.setObjectId(config.getDrillDownObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      drillDownObject.setLayoutData(gd);

		checkLegendInside = new Button(dialogArea, SWT.CHECK);
		checkLegendInside.setText(Messages.get().DialChart_LegendInside);
		checkLegendInside.setSelection(config.isLegendInside());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkLegendInside.setLayoutData(gd);

		checkVertical = new Button(dialogArea, SWT.CHECK);
		checkVertical.setText(Messages.get().Gauge_Vertical);
		checkVertical.setSelection(config.isVertical());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkVertical.setLayoutData(gd);

      checkElementBorders = new Button(dialogArea, SWT.CHECK);
      checkElementBorders.setText(Messages.get().Gauge_ShowBorder);
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
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
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
			MessageDialogHelper.openWarning(getShell(), Messages.get().DialChart_Warning, Messages.get().DialChart_WarningText);
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
