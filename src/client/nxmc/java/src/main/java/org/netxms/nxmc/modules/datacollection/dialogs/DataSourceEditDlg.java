/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.base.widgets.ExtendedColorSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.datacollection.widgets.TemplateDciSelector;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Data source editing dialog
 */
public class DataSourceEditDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(DataSourceEditDlg.class);

	private ChartDciConfig dci;
	private DciSelector dciSelector;
	private LabeledText name;
   private LabeledText displayFormat;
   private TemplateDciSelector dciName;
   private TemplateDciSelector dciDescription;
   private TemplateDciSelector dciTag;
   private ExtendedColorSelector colorSelector;
	private Combo displayType;
	private Button checkShowThresholds;
	private Button checkInvertValues;
   private Button checkRawValues;
   private Button checkMultipeMatch;
   private Button checkRegexpMatch;
	private LabeledText instance;
	private LabeledText dataColumn;
	private boolean isTemplate;
   private boolean isNew;

	/**
	 * @param parentShell
	 * @param dci
	 */
	public DataSourceEditDlg(Shell parentShell, ChartDciConfig dci, boolean isNew, boolean isTemplate)
	{
		super(parentShell);
		this.dci = dci;
      this.isNew = isNew;
		this.isTemplate = isTemplate;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Data Source"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

      GridData gd = new GridData();

      if (isTemplate)
      {
         dciName = new TemplateDciSelector(dialogArea, SWT.NONE);
         dciName.setLabel(i18n.tr("Metric"));
         dciName.setText(dci.dciName);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciName.setLayoutData(gd);       
         addTemplateSelectorListener(dciName);

         dciDescription = new TemplateDciSelector(dialogArea, SWT.NONE);
         dciDescription.setLabel(i18n.tr("DCI display name"));
         dciDescription.setText(dci.dciDescription);
         dciDescription.setField(TemplateDciSelector.Field.DESCRIPTION);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciDescription.setLayoutData(gd);
         addTemplateSelectorListener(dciDescription);
         
         dciTag = new TemplateDciSelector(dialogArea, SWT.NONE);
         dciTag.setLabel(i18n.tr("DCI tag"));
         dciTag.setText(dci.dciTag);
         dciTag.setField(TemplateDciSelector.Field.TAG);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciTag.setLayoutData(gd);
         addTemplateSelectorListener(dciTag);
      }
      else
      {
         dciSelector = new DciSelector(dialogArea, SWT.NONE);
         dciSelector.setLabel(i18n.tr("Data collection item"));
   		dciSelector.setDciId(dci.nodeId, dci.dciId);
   		dciSelector.setDcObjectType(dci.type);
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		gd.widthHint = 400;
   		gd.horizontalSpan = 2;
   		dciSelector.setLayoutData(gd);
      }

		name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Label (Leave empty to use DCI display name)"));
		name.setText(dci.name);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);

      displayFormat = new LabeledText(dialogArea, SWT.NONE);
      displayFormat.setLabel(i18n.tr("Display format"));
      displayFormat.setText(dci.displayFormat);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      displayFormat.setLayoutData(gd);

		if (dci.type == ChartDciConfig.TABLE)
		{
			Group tableGroup = new Group(dialogArea, SWT.NONE);
         tableGroup.setText(i18n.tr("Table Cell"));
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			tableGroup.setLayoutData(gd);
			
			layout = new GridLayout();
			tableGroup.setLayout(layout);
			
			dataColumn = new LabeledText(tableGroup, SWT.NONE);
         dataColumn.setLabel(i18n.tr("Data column"));
			dataColumn.setText(dci.column);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataColumn.setLayoutData(gd);
			
			instance = new LabeledText(tableGroup, SWT.NONE);
         instance.setLabel(i18n.tr("Instance"));
			instance.setText(dci.instance);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			instance.setLayoutData(gd);
		}

      /*** Display type ***/
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      displayType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.BORDER, i18n.tr("Display type"), gd);
      displayType.add("Default");
      displayType.add("Line");
      displayType.add("Area");
      displayType.select(dci.getLineChartType() + 1);

      /*** Options group ***/
      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 2;
      optionsGroup.setLayoutData(gd);
      
      layout = new GridLayout();
      optionsGroup.setLayout(layout);

      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(i18n.tr("Show &thresholds"));
      checkShowThresholds.setSelection(dci.showThresholds);

      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText(i18n.tr("&Invert values"));
      checkInvertValues.setSelection(dci.invertValues);

      checkRawValues = new Button(optionsGroup, SWT.CHECK);
      checkRawValues.setText(i18n.tr("&Raw values"));
      checkRawValues.setSelection(dci.useRawValues);

      if (isTemplate)
      {
         checkMultipeMatch = new Button(optionsGroup, SWT.CHECK);
         checkMultipeMatch.setText(i18n.tr("&Multiple match"));
         checkMultipeMatch.setSelection(dci.multiMatch);
         
         checkRegexpMatch = new Button(optionsGroup, SWT.CHECK);
         checkRegexpMatch.setText(i18n.tr("Use regular &expressions for DCI matching"));
         checkRegexpMatch.setSelection(isNew ? false : dci.regexMatch);
      }

      /*** Color ***/
      colorSelector = new ExtendedColorSelector(dialogArea);
      colorSelector.setLabels(i18n.tr("Color"), i18n.tr("&Automatic color"), i18n.tr("&Custom color:"));
      colorSelector.setColorValue(dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? null : ColorConverter.rgbFromInt(dci.getColorAsInt()));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      colorSelector.setLayoutData(gd);

		return dialogArea;
	}

   /**
    * Add modification listener to template selector.
    *
    * @param selector template selector
    */
   private void addTemplateSelectorListener(final TemplateDciSelector selector)
   {
      selector.addModifyListener((e) -> {
         if (selector.isNoValueObject())
         {
            checkMultipeMatch.setSelection(true);
            checkRegexpMatch.setSelection(true);
            if (name.getText().isEmpty())
               name.setText("\\1");
         }
      });
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      if (isTemplate)
      {
         dci.dciName = dciName.getText();
         dci.dciDescription = dciDescription.getText();
         dci.dciTag = dciTag.getText();
         dci.multiMatch = checkMultipeMatch.getSelection();
         dci.regexMatch = checkRegexpMatch.getSelection();
      }
      else
      {
   		dci.nodeId = dciSelector.getNodeId();
   		dci.dciId = dciSelector.getDciId();
      }
		dci.name = name.getText();
		dci.displayFormat = displayFormat.getText();
      RGB color = colorSelector.getColorValue();
      if (color == null)
		{
			dci.color = ChartDciConfig.UNSET_COLOR;
		}
		else
		{
         dci.color = "0x" + Integer.toHexString(ColorConverter.rgbToInt(color));
		}
      dci.lineChartType = displayType.getSelectionIndex() - 1;
		dci.showThresholds = checkShowThresholds.getSelection();
		dci.invertValues = checkInvertValues.getSelection();
      dci.useRawValues = checkRawValues.getSelection();
		if (dci.type == ChartDciConfig.TABLE)
		{
			dci.column = dataColumn.getText().trim();
			dci.instance = instance.getText();
		}
		super.okPressed();
	}
}
