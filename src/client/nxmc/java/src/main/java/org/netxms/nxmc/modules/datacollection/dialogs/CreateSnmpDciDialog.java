/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating DCI from SNMP walk result
 */
public class CreateSnmpDciDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSnmpDciDialog.class);
   
	private LabeledText textDescription;
	private LabeledText textInterval;
	private LabeledText textRetention;
	private Combo deltaCalculationSelector;

	private String description;
	private int pollingInterval;
	private int retentionTime;
	private int deltaCalculation = DataCollectionItem.DELTA_NONE;
	
	/**
	 * @param parentShell
	 */
	public CreateSnmpDciDialog(Shell parentShell, String initialDescription)
	{
		super(parentShell);
		description = initialDescription;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("Create SNMP DCI"));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

		textDescription = new LabeledText(dialogArea, SWT.NONE);
		textDescription.setLabel(i18n.tr("Description"));
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
		
		textInterval = new LabeledText(optionsGroup, SWT.NONE);
		textInterval.setLabel(i18n.tr("Polling pollingInterval (seconds)"));
		textInterval.setText("60"); //$NON-NLS-1$
		textInterval.getTextControl().setTextLimit(5);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textInterval.setLayoutData(gd);
		
		textRetention = new LabeledText(optionsGroup, SWT.NONE);
		textRetention.setLabel(i18n.tr("Retention time (days)"));
		textRetention.setText("30"); //$NON-NLS-1$
		textRetention.getTextControl().setTextLimit(5);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textRetention.setLayoutData(gd);

      deltaCalculationSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Delta calculation"),
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      deltaCalculationSelector.add(i18n.tr("None (keep original value)"));
      deltaCalculationSelector.add(i18n.tr("Simple delta"));
      deltaCalculationSelector.add(i18n.tr("Average delta per second"));
      deltaCalculationSelector.add(i18n.tr("Average delta per minute"));
      deltaCalculationSelector.select(deltaCalculation);
		
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
	 * @return the pollingInterval
	 */
	public final int getPollingInterval()
	{
		return pollingInterval;
	}

	/**
	 * @return the retentionTime
	 */
	public final int getRetentionTime()
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
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			pollingInterval = Integer.parseInt(textInterval.getText());
			if ((pollingInterval < 2) || (pollingInterval > 10000))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter polling pollingInterval as integer in range 2 .. 10000"));
		}
		
		try
		{
			retentionTime = Integer.parseInt(textRetention.getText());
			if ((retentionTime < 1) || (retentionTime > 10000))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter retention time as integer in range 1 .. 10000"));
		}

		description = textDescription.getText().trim();
		deltaCalculation = deltaCalculationSelector.getSelectionIndex();
		
		super.okPressed();
	}
}
