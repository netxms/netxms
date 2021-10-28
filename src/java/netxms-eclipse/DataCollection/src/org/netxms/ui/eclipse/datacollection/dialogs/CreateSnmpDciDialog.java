/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating DCI from SNMP walk result
 */
public class CreateSnmpDciDialog extends Dialog
{
   private LabeledText textDescription;
   private Combo pollingScheduleTypeSelector;
   private LabeledText textInterval;
   private Combo retentionTypeSelector;
   private LabeledText textRetention;
   private Combo deltaCalculationSelector;

   private String description;
   private int pollingScheduleType;
   private String pollingInterval;
   private int retentionType;
   private String retentionTime;
   private int deltaCalculation = DataCollectionItem.DELTA_NONE;

	/**
	 * @param parentShell
	 */
	public CreateSnmpDciDialog(Shell parentShell, String initialDescription)
	{
		super(parentShell);
		description = initialDescription;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().CreateSnmpDciDialog_ShellTitle);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

		textDescription = new LabeledText(dialogArea, SWT.NONE);
		textDescription.setLabel(Messages.get().CreateSnmpDciDialog_Description);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textDescription.setLayoutData(gd);
		textDescription.setText(description);

		Composite optionsGroup = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		layout = new GridLayout();
      layout.numColumns = 3;
		layout.makeColumnsEqualWidth = true;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		optionsGroup.setLayout(layout);

      pollingScheduleTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, "Polling interval", WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollingScheduleTypeSelector.add("Default");
      pollingScheduleTypeSelector.add("Custom");
      pollingScheduleTypeSelector.select(0);
      pollingScheduleTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textInterval.setEnabled(pollingScheduleTypeSelector.getSelectionIndex() == 1);
         }
      });

      retentionTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, "Retention time", WidgetHelper.DEFAULT_LAYOUT_DATA);
      retentionTypeSelector.add(Messages.get().General_UseDefaultRetention);
      retentionTypeSelector.add(Messages.get().General_UseCustomRetention);
      retentionTypeSelector.add(Messages.get().General_NoStorage);
      retentionTypeSelector.select(0);
      retentionTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textRetention.setEnabled(retentionTypeSelector.getSelectionIndex() == 1);
         }
      });

      deltaCalculationSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().CreateSnmpDciDialog_DeltaCalculation, WidgetHelper.DEFAULT_LAYOUT_DATA);
      deltaCalculationSelector.add(Messages.get().Transformation_DeltaNone);
      deltaCalculationSelector.add(Messages.get().Transformation_DeltaSimple);
      deltaCalculationSelector.add(Messages.get().Transformation_DeltaAvgPerSec);
      deltaCalculationSelector.add(Messages.get().Transformation_DeltaAvgPerMin);
      deltaCalculationSelector.select(deltaCalculation);

      textInterval = new LabeledText(optionsGroup, SWT.NONE);
      textInterval.setLabel("Custom polling interval (seconds)");
      textInterval.setText("60"); //$NON-NLS-1$
      textInterval.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textInterval.setLayoutData(gd);
      textInterval.setEnabled(false);

      textRetention = new LabeledText(optionsGroup, SWT.NONE);
      textRetention.setLabel("Custom retention time (days)");
      textRetention.setText("30"); //$NON-NLS-1$
      textRetention.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textRetention.setLayoutData(gd);
      textRetention.setEnabled(false);

		return dialogArea;
	}

	/**
	 * @return the description
	 */
	public final String getDescription()
	{
		return description;
	}

	/**
    * @return the pollingScheduleType
    */
   public int getPollingScheduleType()
   {
      return pollingScheduleType;
   }

   /**
    * @return the retentionType
    */
   public int getRetentionType()
   {
      return retentionType;
   }

   /**
    * @return the pollingInterval
    */
   public final String getPollingInterval()
	{
		return pollingInterval;
	}

	/**
	 * @return the retentionTime
	 */
   public final String getRetentionTime()
	{
		return retentionTime;
	}

	/**
	 * @return the deltaCalculation
	 */
	public final int getDeltaCalculation()
	{
		return deltaCalculation;
	}
	
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      pollingScheduleType = pollingScheduleTypeSelector.getSelectionIndex();
      pollingInterval = (pollingScheduleType == DataCollectionObject.POLLING_SCHEDULE_CUSTOM) ? textInterval.getText().trim() : null;
      retentionType = retentionTypeSelector.getSelectionIndex();
      retentionTime = (retentionType == DataCollectionObject.RETENTION_CUSTOM) ? textRetention.getText().trim() : null;
		description = textDescription.getText().trim();
		deltaCalculation = deltaCalculationSelector.getSelectionIndex();
		super.okPressed();
	}
}
