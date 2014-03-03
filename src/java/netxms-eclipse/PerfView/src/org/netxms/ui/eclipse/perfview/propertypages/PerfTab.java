/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.perfview.widgets.YAxisRangeEditor;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * DCI property page for Performance Tab settings
 */
public class PerfTab extends PropertyPage
{
	private DataCollectionObjectEditor editor;
	private DataCollectionItem dci;
	private PerfTabGraphSettings settings;
	private Button checkShow;
	private Button checkLogScale;
	private LabeledText title;
	private LabeledText name;
	private ColorSelector color;
	private Combo type;
	private Spinner orderNumber;
	private Button checkShowThresholds;
	private DciSelector parentDci;
   private Spinner timeRange;
   private Combo timeUnits;
	private YAxisRangeEditor yAxisRange;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (DataCollectionObjectEditor)getElement().getAdapter(DataCollectionObjectEditor.class);
		dci = editor.getObjectAsItem();
		try
		{
			settings = PerfTabGraphSettings.createFromXml(dci.getPerfTabSettings());
		}
		catch(Exception e)
		{
			settings = new PerfTabGraphSettings();	// Create default empty settings
		}
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 4;
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
      title.setLayoutData(gd);
      
      Composite colors = new Composite(dialogArea, SWT.NONE);
      colors.setLayout(new RowLayout(SWT.VERTICAL));
      new Label(colors, SWT.NONE).setText(Messages.get().PerfTab_Color);
      color = new ColorSelector(colors);
      color.setColorValue(ColorConverter.rgbFromInt(settings.getColorAsInt()));
      
      type = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().PerfTab_Type, new GridData(SWT.LEFT, SWT.CENTER, false, false));
      type.add(Messages.get().PerfTab_Line);
      type.add(Messages.get().PerfTab_Area);
      type.select(settings.getType());
      
      orderNumber = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().PerfTab_Order, 0, 65535, new GridData(SWT.LEFT, SWT.CENTER, false, false));
      orderNumber.setSelection(settings.getOrder());

      parentDci = new DciSelector(dialogArea, SWT.NONE, false);
      parentDci.setDciId(dci.getNodeId(), settings.getParentDciId());
      parentDci.setFixedNode(true);
      parentDci.setLabel(Messages.get().PerfTab_Attach);
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      parentDci.setLayoutData(gd);

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
      timeGroupLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      timeGroupLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      timeGroupLayout.horizontalSpacing = 16;
      timeGroupLayout.makeColumnsEqualWidth = true;
      timeGroupLayout.numColumns = 1;
      timeGroup.setLayout(timeGroupLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = timeGroupLayout.numColumns;
      timeGroup.setLayoutData(gd);
      
      Composite timeRangeArea = new Composite(timeGroup, SWT.NONE);
      timeGroupLayout = new GridLayout();
      timeGroupLayout.numColumns = 2;
      timeGroupLayout.marginWidth = 0;
      timeGroupLayout.marginHeight = 0;
      timeGroupLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      timeRangeArea.setLayout(timeGroupLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeRangeArea.setLayoutData(gd);
      
      timeRange = WidgetHelper.createLabeledSpinner(timeRangeArea, SWT.BORDER, Messages.get().PerfTab_TimeInterval, 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeRange.setSelection(settings.getTimeRange());
      
      timeUnits = WidgetHelper.createLabeledCombo(timeRangeArea, SWT.READ_ONLY, Messages.get().PerfTab_TimeUnits, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeUnits.add(Messages.get().PerfTab_Minutes);
      timeUnits.add(Messages.get().PerfTab_Hours);
      timeUnits.add(Messages.get().PerfTab_Days);
      timeUnits.select(settings.getTimeUnits());      
      
      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(Messages.get().PerfTab_Options);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      gd.verticalSpan = 2;
      optionsGroup.setLayoutData(gd);
      GridLayout optionsLayout = new GridLayout();
      optionsGroup.setLayout(optionsLayout);
      
      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(Messages.get().PerfTab_ShowThresholds);
      checkShowThresholds.setSelection(settings.isShowThresholds());
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      checkShowThresholds.setLayoutData(gd);
      
      checkLogScale = new Button(optionsGroup, SWT.CHECK);
      checkLogScale.setText(Messages.get().PerfTab_LogarithmicScale);
      checkLogScale.setSelection(settings.isLogScaleEnabled());
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      checkLogScale.setLayoutData(gd);
      
      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      yAxisRange.setLayoutData(gd);
      yAxisRange.setSelection(settings.isAutoScale(), settings.getMinYScaleValue(), settings.getMaxYScaleValue());
      
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
		settings.setColor(ColorConverter.rgbToInt(color.getColorValue()));
		settings.setType(type.getSelectionIndex());
		settings.setOrder(orderNumber.getSelection());
		settings.setShowThresholds(checkShowThresholds.getSelection());
		
		settings.setAutoScale(yAxisRange.isAuto());
		settings.setMinYScaleValue(yAxisRange.getMinY());
		settings.setMaxYScaleValue(yAxisRange.getMaxY());

		settings.setParentDciId(parentDci.getDciId());
		
		settings.setTimeRange(timeRange.getSelection());
		settings.setTimeUnits(timeUnits.getSelectionIndex());
		
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
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
		parentDci.setDciId(0, 0);
	}
}
