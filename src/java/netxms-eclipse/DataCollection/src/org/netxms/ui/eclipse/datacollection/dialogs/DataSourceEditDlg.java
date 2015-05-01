/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.charts.api.ChartDciConfig;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Data source editing dialog
 */
public class DataSourceEditDlg extends Dialog
{
	private ChartDciConfig dci;
	private DciSelector dciSelector;
	private LabeledText name;
	private Button colorAuto;
	private Button colorCustom;
	private ColorSelector colorSelector;
	private Button checkAreaChart;
	private Button checkShowThresholds;
	private Button checkInvertValues;
	private LabeledText instance;
	private LabeledText dataColumn;
	
	/**
	 * @param parentShell
	 * @param dci
	 */
	public DataSourceEditDlg(Shell parentShell, ChartDciConfig dci)
	{
		super(parentShell);
		this.dci = dci;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().DataSourceEditDlg_ShellTitle);
	}

	/* (non-Javadoc)
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
		
		dciSelector = new DciSelector(dialogArea, SWT.NONE, false);
		dciSelector.setLabel(Messages.get().DataSourceEditDlg_DCI);
		dciSelector.setDciId(dci.nodeId, dci.dciId);
		dciSelector.setDcObjectType(dci.type);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		gd.horizontalSpan = 2;
		dciSelector.setLayoutData(gd);
		
		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel(Messages.get().DataSourceEditDlg_DispName);
		name.setText(dci.name);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);
		
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
		
		Group colorGroup = new Group(dialogArea, SWT.NONE);
		colorGroup.setText(Messages.get().DataSourceEditDlg_Color);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		colorGroup.setLayoutData(gd);
		
		layout = new GridLayout();
		layout.numColumns = 2;
		colorGroup.setLayout(layout);
		
		colorAuto = new Button(colorGroup, SWT.RADIO);
		colorAuto.setText(Messages.get().DataSourceEditDlg_AutoColor);
		colorAuto.setSelection(dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		gd = new GridData();
		gd.horizontalSpan = 2;
		colorAuto.setLayoutData(gd);
		colorAuto.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				colorSelector.setEnabled(false);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		colorCustom = new Button(colorGroup, SWT.RADIO);
		colorCustom.setText(Messages.get().DataSourceEditDlg_CustomColor);
		colorCustom.setSelection(!dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		colorCustom.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				colorSelector.setEnabled(true);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		colorSelector = new ColorSelector(colorGroup);
		colorSelector.setColorValue(ColorConverter.rgbFromInt(dci.getColorAsInt()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		colorSelector.getButton().setLayoutData(gd);
		colorSelector.setEnabled(!dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().DataSourceEditDlg_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		optionsGroup.setLayoutData(gd);
		
		layout = new GridLayout();
		optionsGroup.setLayout(layout);
		
		checkAreaChart = new Button(optionsGroup, SWT.CHECK);
		checkAreaChart.setText(Messages.get().DataSourceEditDlg_AreaChart);
		checkAreaChart.setSelection(dci.area);
		
		checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
		checkShowThresholds.setText(Messages.get().DataSourceEditDlg_ShowThresholds);
		checkShowThresholds.setSelection(dci.showThresholds);
		
      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText("Invert values");
      checkInvertValues.setSelection(dci.invertValues);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		dci.nodeId = dciSelector.getNodeId();
		dci.dciId = dciSelector.getDciId();
		dci.name = name.getText();
		if (colorAuto.getSelection())
		{
			dci.color = ChartDciConfig.UNSET_COLOR;
		}
		else
		{
			dci.color = "0x" + Integer.toHexString(ColorConverter.rgbToInt(colorSelector.getColorValue())); //$NON-NLS-1$
		}
		dci.area = checkAreaChart.getSelection();
		dci.showThresholds = checkShowThresholds.getSelection();
		dci.invertValues = checkInvertValues.getSelection();
		if (dci.type == ChartDciConfig.TABLE)
		{
			dci.column = dataColumn.getText().trim();
			dci.instance = instance.getText();
		}
		super.okPressed();
	}
}
