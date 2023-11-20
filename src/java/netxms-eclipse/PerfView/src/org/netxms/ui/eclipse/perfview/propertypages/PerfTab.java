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
package org.netxms.ui.eclipse.perfview.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.AbstractDCIPropertyPage;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.perfview.widgets.YAxisRangeEditor;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ExtendedColorSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * DCI property page for Performance Tab settings
 */
public class PerfTab extends AbstractDCIPropertyPage
{
	private DataCollectionItem dci;
	private PerfTabGraphSettings settings;
	private Button checkShow;
	private Button checkLogScale;
   private Button checkStacked;
   private Button checkShowLegendAlways;
   private Button checkExtendedLegend;
   private Button checkUseMultipliers;
   private Button checkInvertValues;
   private Button checkTranslucent;
	private LabeledText title;
	private LabeledText name;
   private ExtendedColorSelector color;
	private Combo type;
	private Spinner orderNumber;
	private Button checkShowThresholds;
	private LabeledText groupName;
   private Spinner timeRange;
   private Combo timeUnits;
	private YAxisRangeEditor yAxisRange;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();
		
		try
		{
         settings = XMLTools.createFromXml(PerfTabGraphSettings.class, dci.getPerfTabSettings());
		}
		catch(Exception e)
		{
			settings = new PerfTabGraphSettings();	// Create default empty settings
		}
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      layout.numColumns = 3;
      dialogArea.setLayout(layout);

      checkShow = new Button(dialogArea, SWT.CHECK);
      checkShow.setText(Messages.get().PerfTab_ShowOnPerfTab);
      checkShow.setSelection(settings.isEnabled());
      GridData gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      checkShow.setLayoutData(gd);
      
      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel(Messages.get().PerfTab_Title);
      title.setText(settings.getTitle());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      title.setLayoutData(gd);

      groupName = new LabeledText(dialogArea, SWT.NONE);
      groupName.setLabel("Group");
      groupName.setText(settings.getGroupName());
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      groupName.setLayoutData(gd);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(Messages.get().PerfTab_NameInLegend);
      name.setText(settings.getName());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      name.setLayoutData(gd);
      
      Group timeGroup = new Group(dialogArea, SWT.NONE);
      timeGroup.setText(Messages.get().PerfTab_TeimePeriod);
      GridLayout timeGroupLayout = new GridLayout();
      timeGroupLayout.makeColumnsEqualWidth = true;
      timeGroupLayout.numColumns = 2;
      timeGroup.setLayout(timeGroupLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      timeGroup.setLayoutData(gd);
      
      timeRange = WidgetHelper.createLabeledSpinner(timeGroup, SWT.BORDER, Messages.get().PerfTab_TimeInterval, 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeRange.setSelection(settings.getTimeRange());

      timeUnits = WidgetHelper.createLabeledCombo(timeGroup, SWT.READ_ONLY, Messages.get().PerfTab_TimeUnits, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeUnits.add(Messages.get().PerfTab_Minutes);
      timeUnits.add(Messages.get().PerfTab_Hours);
      timeUnits.add(Messages.get().PerfTab_Days);
      timeUnits.select(settings.getTimeUnits());      

      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(Messages.get().PerfTab_Options);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalSpan = 3;
      gd.verticalAlignment = SWT.FILL;
      optionsGroup.setLayoutData(gd);
      GridLayout optionsLayout = new GridLayout();
      optionsGroup.setLayout(optionsLayout);

      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(Messages.get().PerfTab_ShowThresholds);
      checkShowThresholds.setSelection(settings.isShowThresholds());
      
      checkLogScale = new Button(optionsGroup, SWT.CHECK);
      checkLogScale.setText(Messages.get().PerfTab_LogarithmicScale);
      checkLogScale.setSelection(settings.isLogScaleEnabled());
      
      checkStacked = new Button(optionsGroup, SWT.CHECK);
      checkStacked.setText("&Stacked");
      checkStacked.setSelection(settings.isStacked());

      checkShowLegendAlways = new Button(optionsGroup, SWT.CHECK);
      checkShowLegendAlways.setText("Always show &legend");
      checkShowLegendAlways.setSelection(settings.isShowLegendAlways());

      checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
      checkExtendedLegend.setText("&Extended legend");
      checkExtendedLegend.setSelection(settings.isExtendedLegend());

      checkUseMultipliers = new Button(optionsGroup, SWT.CHECK);
      checkUseMultipliers.setText("Use &multipliers");
      checkUseMultipliers.setSelection(settings.isUseMultipliers());

      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText("&Inverted values");
      checkInvertValues.setSelection(settings.isInvertedValues());
      
      checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText("&Translucent");
      checkTranslucent.setSelection(settings.isTranslucent());

      color = new ExtendedColorSelector(dialogArea);
      color.setLabels(Messages.get().PerfTab_Color, "Automatic", "Custom");
      color.setColorValue(settings.isAutomaticColor() ? null : ColorConverter.rgbFromInt(settings.getColorAsInt()));
      gd = new GridData();
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      color.setLayoutData(gd);

      type = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().PerfTab_Type, new GridData(SWT.FILL, SWT.TOP, false, false));
      type.add(Messages.get().PerfTab_Line);
      type.add(Messages.get().PerfTab_Area);
      type.select(settings.getType());

      orderNumber = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().PerfTab_Order, 0, 65535, new GridData(SWT.FILL, SWT.TOP, false, false));
      orderNumber.setSelection(settings.getOrder());

      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      yAxisRange.setLayoutData(gd);
      yAxisRange.setSelection(settings.isAutoScale(), settings.modifyYBase(),
                              settings.getMinYScaleValue(), settings.getMaxYScaleValue());
      
      return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if APPLY button was used
	 */
	private void applyChanges(final boolean isApply)
	{
		settings.setEnabled(checkShow.getSelection());
		settings.setLogScaleEnabled(checkLogScale.getSelection());
		settings.setTitle(title.getText());
		settings.setName(name.getText());
		settings.setType(type.getSelectionIndex());
		settings.setOrder(orderNumber.getSelection());
      settings.setGroupName(groupName.getText().trim());
		settings.setShowThresholds(checkShowThresholds.getSelection());
		settings.setStacked(checkStacked.getSelection());
      settings.setShowLegendAlways(checkShowLegendAlways.getSelection());
      settings.setExtendedLegend(checkExtendedLegend.getSelection());
      settings.setUseMultipliers(checkUseMultipliers.getSelection());
      settings.setInvertedValues(checkInvertValues.getSelection());
      settings.setTranslucent(checkTranslucent.getSelection());

		settings.setAutoScale(yAxisRange.isAuto());
		settings.setMinYScaleValue(yAxisRange.getMinY());
		settings.setMaxYScaleValue(yAxisRange.getMaxY());
		settings.setModifyYBase(yAxisRange.modifyYBase());

		settings.setTimeRange(timeRange.getSelection());
		settings.setTimeUnits(timeUnits.getSelectionIndex());

		RGB rgb = color.getColorValue();
		if (rgb != null)
		{
         settings.setAutomaticColor(false);
         settings.setColor(ColorConverter.rgbToInt(rgb));
		}
		else
		{
		   settings.setAutomaticColor(true);
		}
		
		try
		{
			dci.setPerfTabSettings(settings.createXml());
		}
		catch(Exception e)
		{
			dci.setPerfTabSettings(null);
		}

		editor.modify();
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
      if (yAxisRange.validate(true))
         applyChanges(true);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
      if (!yAxisRange.validate(true))
         return false;
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		PerfTabGraphSettings defaults = new PerfTabGraphSettings();
		checkShow.setSelection(defaults.isEnabled());
		title.setText(defaults.getTitle());
		color.setColorValue(ColorConverter.rgbFromInt(defaults.getColorAsInt()));
		groupName.setText("");
		orderNumber.setSelection(100);
	}
}
