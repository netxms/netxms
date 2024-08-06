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
package org.netxms.nxmc.modules.datacollection.propertypages;

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
import org.netxms.nxmc.base.widgets.ExtendedColorSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.views.helpers.PerfViewGraphSettings;
import org.netxms.nxmc.modules.datacollection.widgets.YAxisRangeEditor;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI property page for Performance View settings
 */
public class PerformanceView extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(PerformanceView.class);
   
	private DataCollectionItem dci;
	private PerfViewGraphSettings settings;
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
    * Create property page.
    *
    * @param editor DCI editor object
    */
   public PerformanceView(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(PerformanceView.class).tr("Performance View"), editor);
   }

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
         settings = XMLTools.createFromXml(PerfViewGraphSettings.class, dci.getPerfTabSettings());
		}
		catch(Exception e)
		{
			settings = new PerfViewGraphSettings();	// Create default empty settings
		}

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      layout.numColumns = 3;
      dialogArea.setLayout(layout);

      checkShow = new Button(dialogArea, SWT.CHECK);
      checkShow.setText(i18n.tr("&Show in performance view"));
      checkShow.setSelection(settings.isEnabled());
      GridData gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      checkShow.setLayoutData(gd);

      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel(i18n.tr("Title"));
      title.setText(settings.getTitle());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      title.setLayoutData(gd);

      groupName = new LabeledText(dialogArea, SWT.NONE);
      groupName.setLabel(i18n.tr("Group"));
      groupName.setText(settings.getGroupName());
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      groupName.setLayoutData(gd);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name in legend"));
      name.setText(settings.getName());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      name.setLayoutData(gd);

      Group timeGroup = new Group(dialogArea, SWT.NONE);
      timeGroup.setText(i18n.tr("Time Period"));
      GridLayout timeGroupLayout = new GridLayout();
      timeGroupLayout.makeColumnsEqualWidth = true;
      timeGroupLayout.numColumns = 2;
      timeGroup.setLayout(timeGroupLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      timeGroup.setLayoutData(gd);

      timeRange = WidgetHelper.createLabeledSpinner(timeGroup, SWT.BORDER, i18n.tr("Time interval"), 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeRange.setSelection(settings.getTimeRange());

      timeUnits = WidgetHelper.createLabeledCombo(timeGroup, SWT.READ_ONLY, i18n.tr("Time units"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeUnits.add(i18n.tr("Minutes"));
      timeUnits.add(i18n.tr("Hours"));
      timeUnits.add(i18n.tr("Days"));
      timeUnits.select(settings.getTimeUnits());      

      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalSpan = 3;
      gd.verticalAlignment = SWT.FILL;
      optionsGroup.setLayoutData(gd);
      GridLayout optionsLayout = new GridLayout();
      optionsGroup.setLayout(optionsLayout);

      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(i18n.tr("&Show thresholds on graph"));
      checkShowThresholds.setSelection(settings.isShowThresholds());

      checkLogScale = new Button(optionsGroup, SWT.CHECK);
      checkLogScale.setText(i18n.tr("Logarithmic scale"));
      checkLogScale.setSelection(settings.isLogScaleEnabled());

      checkStacked = new Button(optionsGroup, SWT.CHECK);
      checkStacked.setText(i18n.tr("&Stacked"));
      checkStacked.setSelection(settings.isStacked());

      checkShowLegendAlways = new Button(optionsGroup, SWT.CHECK);
      checkShowLegendAlways.setText(i18n.tr("Always show &legend"));
      checkShowLegendAlways.setSelection(settings.isShowLegendAlways());

      checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
      checkExtendedLegend.setText(i18n.tr("&Extended legend"));
      checkExtendedLegend.setSelection(settings.isExtendedLegend());

      checkUseMultipliers = new Button(optionsGroup, SWT.CHECK);
      checkUseMultipliers.setText(i18n.tr("Use &multipliers"));
      checkUseMultipliers.setSelection(settings.isUseMultipliers());

      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText(i18n.tr("&Inverted values"));
      checkInvertValues.setSelection(settings.isInvertedValues());

      checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(i18n.tr("&Translucent"));
      checkTranslucent.setSelection(settings.isTranslucent());

      color = new ExtendedColorSelector(dialogArea);
      color.setLabels(i18n.tr("Color"), i18n.tr("Automatic"), i18n.tr("Custom"));
      color.setColorValue(settings.isAutomaticColor() ? null : ColorConverter.rgbFromInt(settings.getColorAsInt()));
      gd = new GridData();
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      color.setLayoutData(gd);

      type = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Type"), new GridData(SWT.FILL, SWT.TOP, false, false));
      type.add(i18n.tr("Line"));
      type.add(i18n.tr("Area"));
      type.select(settings.getType());

      orderNumber = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Order"), 0, 65535, new GridData(SWT.FILL, SWT.TOP, false, false));
      orderNumber.setSelection(settings.getOrder());

      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      yAxisRange.setLayoutData(gd);
      yAxisRange.setSelection(settings.isAutoScale(), settings.modifyYBase(), settings.getMinYScaleValue(), settings.getMaxYScaleValue(), settings.getYAxisLabel());

      return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if APPLY button was used
	 */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      if (!yAxisRange.validate(true))
         return false;
      
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
      settings.setYAxisLabel(yAxisRange.getLabel());

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
         dci.setPerfTabSettings(XMLTools.serialize(settings));
		}
		catch(Exception e)
		{
			dci.setPerfTabSettings(null);
		}

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
		PerfViewGraphSettings defaults = new PerfViewGraphSettings();
		checkShow.setSelection(defaults.isEnabled());
		title.setText(defaults.getTitle());
		color.setColorValue(ColorConverter.rgbFromInt(defaults.getColorAsInt()));
		groupName.setText("");
		orderNumber.setSelection(100);
	}
}
