/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating DCI from SNMP walk result
 */
public class CreateSnmpDciDialog extends Dialog
{
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
		newShell.setText("Create SNMP DCI");
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
		textDescription.setLabel("Description");
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
		textInterval.setLabel(Messages.CreateInterfaceDciDialog_PollingInterval);
		textInterval.setText("60"); //$NON-NLS-1$
		textInterval.getTextControl().setTextLimit(5);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textInterval.setLayoutData(gd);
		
		textRetention = new LabeledText(optionsGroup, SWT.NONE);
		textRetention.setLabel(Messages.CreateInterfaceDciDialog_RetentionTime);
		textRetention.setText("30"); //$NON-NLS-1$
		textRetention.getTextControl().setTextLimit(5);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textRetention.setLayoutData(gd);

      deltaCalculationSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, "Delta calculation",
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      deltaCalculationSelector.add(Messages.Transformation_DeltaNone);
      deltaCalculationSelector.add(Messages.Transformation_DeltaSimple);
      deltaCalculationSelector.add(Messages.Transformation_DeltaAvgPerSec);
      deltaCalculationSelector.add(Messages.Transformation_DeltaAvgPerMin);
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
			MessageDialog.openError(getShell(), Messages.CreateInterfaceDciDialog_Error, Messages.CreateInterfaceDciDialog_BadPollingInterval);
		}
		
		try
		{
			retentionTime = Integer.parseInt(textRetention.getText());
			if ((retentionTime < 1) || (retentionTime > 10000))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openError(getShell(), Messages.CreateInterfaceDciDialog_Error, Messages.CreateInterfaceDciDialog_BadRetentionTime);
		}

		description = textDescription.getText().trim();
		deltaCalculation = deltaCalculationSelector.getSelectionIndex();
		
		super.okPressed();
	}
}
