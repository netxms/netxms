/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
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
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.datacollection.widgets.TemplateDciSelector;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ExtendedColorSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Data source editing dialog
 */
public class DataSourceEditDlg extends Dialog
{
	private ChartDciConfig dci;
	private DciSelector dciSelector;
	private LabeledText name;
   private LabeledText displayFormat;
   private TemplateDciSelector dciName;
   private TemplateDciSelector dciDescription;
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
		newShell.setText(Messages.get().DataSourceEditDlg_ShellTitle);
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
         dciName.setLabel("Metric");
         dciName.setText(dci.dciName);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciName.setLayoutData(gd);       

         dciDescription = new TemplateDciSelector(dialogArea, SWT.NONE);
         dciDescription.setLabel("DCI display name");
         dciDescription.setText(dci.dciDescription);
         dciDescription.setSelectDescription(true);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciDescription.setLayoutData(gd);
         
         ModifyListener listener = new ModifyListener() {            
            @Override
            public void modifyText(ModifyEvent e)
            {
               if (dciName.isNoValueObject() || dciDescription.isNoValueObject())
               {
                  checkMultipeMatch.setSelection(true);
                  checkRegexpMatch.setSelection(true);
                  if (name.getText().isEmpty())
                     name.setText("\\1");
               }
            }
         };
         dciName.addModifyListener(listener);
         dciDescription.addModifyListener(listener);
      }
      else
      {
         dciSelector = new DciSelector(dialogArea, SWT.NONE);
   		dciSelector.setLabel(Messages.get().DataSourceEditDlg_DCI);
   		dciSelector.setDciId(dci.nodeId, dci.dciId);
   		dciSelector.setDcObjectType(dci.type);
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		gd.widthHint = 400;
   		gd.horizontalSpan = 2;
   		dciSelector.setLayoutData(gd);
      }

		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel("Label (Leave empty to use DCI display name)");
		name.setText(dci.name);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);

      displayFormat = new LabeledText(dialogArea, SWT.NONE);
      displayFormat.setLabel(Messages.get().DataSourceEditDlg_DisplayFormat);
      displayFormat.setText(dci.displayFormat);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      displayFormat.setLayoutData(gd);

		if (dci.type == ChartDciConfig.TABLE)
		{
			Group tableGroup = new Group(dialogArea, SWT.NONE);
			tableGroup.setText(Messages.get().DataSourceEditDlg_TableCell);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			tableGroup.setLayoutData(gd);
			
			layout = new GridLayout();
			tableGroup.setLayout(layout);
			
			dataColumn = new LabeledText(tableGroup, SWT.NONE);
			dataColumn.setLabel(Messages.get().DataSourceEditDlg_DataColumn);
			dataColumn.setText(dci.column);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataColumn.setLayoutData(gd);
			
			instance = new LabeledText(tableGroup, SWT.NONE);
			instance.setLabel(Messages.get().DataSourceEditDlg_Instance);
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
      displayType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.BORDER, "Display type", gd);
      displayType.add("Default");
      displayType.add("Line");
      displayType.add("Area");
      displayType.select(dci.getLineChartType() + 1);

      /*** Options group ***/
      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(Messages.get().DataSourceEditDlg_Options);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 2;
      optionsGroup.setLayoutData(gd);
      
      layout = new GridLayout();
      optionsGroup.setLayout(layout);

      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(Messages.get().DataSourceEditDlg_ShowThresholds);
      checkShowThresholds.setSelection(dci.showThresholds);

      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText(Messages.get().DataSourceEditDlg_InvertValues);
      checkInvertValues.setSelection(dci.invertValues);

      checkRawValues = new Button(optionsGroup, SWT.CHECK);
      checkRawValues.setText("Raw values");
      checkRawValues.setSelection(dci.useRawValues);

      if (isTemplate)
      {
         checkMultipeMatch = new Button(optionsGroup, SWT.CHECK);
         checkMultipeMatch.setText("Multiple match");
         checkMultipeMatch.setSelection(dci.multiMatch);
         
         checkRegexpMatch = new Button(optionsGroup, SWT.CHECK);
         checkRegexpMatch.setText("Use regular expression for metric and display name");
         checkRegexpMatch.setSelection(isNew ? false : dci.regexMatch);
      }

      /*** Color ***/
      colorSelector = new ExtendedColorSelector(dialogArea);
      colorSelector.setLabels(Messages.get().DataSourceEditDlg_Color, Messages.get().DataSourceEditDlg_AutoColor, Messages.get().DataSourceEditDlg_CustomColor);
      colorSelector.setColorValue(dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? null : ColorConverter.rgbFromInt(dci.getColorAsInt()));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      colorSelector.setLayoutData(gd);

		return dialogArea;
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
         dci.color = "0x" + Integer.toHexString(ColorConverter.rgbToInt(color)); //$NON-NLS-1$
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
