/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.InterfaceDciInfo;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating DCIs for network interface
 */
public class CreateInterfaceDciDialog extends Dialog
{
	private static final String[] names = 
		{ 
			"Inbound traffic (bytes)", 
			"Outbound traffic (bytes)",
			"Inbound traffic (packets)", 
			"Outbound traffic (packets)",
			"Input errors",
			"Output errors"
		};
	private static final String[] descriptions = 
		{ 
			"Inbound traffic on @@ifName@@ (bytes/sec)", 
			"Outbound traffic on @@ifName@@ (bytes/sec)",
			"Inbound traffic on @@ifName@@ (packets/sec)", 
			"Outbound traffic on @@ifName@@ (packets/sec)",
			"Inbound error rate on @@ifName@@ (errors/sec)",
			"Outbound error rate on @@ifName@@ (errors/sec)"
		};
	private static final boolean[] defaultEnabled = { true, true, false, false, false, false };
	
	private Interface object;
	private InterfaceDciForm[] forms = new InterfaceDciForm[names.length];
	private LabeledText textInterval;
	private LabeledText textRetention;
	
	private int pollingInterval;
	private int retentionTime;
	private InterfaceDciInfo[] dciInfo;
	
	/**
	 * @param parentShell
	 */
	public CreateInterfaceDciDialog(Shell parentShell, Interface object)
	{
		super(parentShell);
		this.object = object;
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
		
		Group dataGroup = new Group(dialogArea, SWT.NONE);
		dataGroup.setText("Data");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataGroup.setLayoutData(gd);
		layout = new GridLayout();
		dataGroup.setLayout(layout);
		
		for(int i = 0; i < names.length; i++)
		{
			forms[i] = new InterfaceDciForm(dataGroup, names[i], 
					(object != null) ? descriptions[i].replaceAll("@@ifName@@", object.getObjectName()) : descriptions[i], defaultEnabled[i]);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			forms[i].setLayoutData(gd);
		}
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		optionsGroup.setLayout(layout);
		
		textInterval = new LabeledText(optionsGroup, SWT.NONE);
		textInterval.setLabel("Polling pollingInterval (seconds)");
		textInterval.setText("60");
		textInterval.getTextControl().setTextLimit(5);
		
		textRetention = new LabeledText(optionsGroup, SWT.NONE);
		textRetention.setLabel("Retention time (days)");
		textRetention.setText("30");
		textRetention.getTextControl().setTextLimit(5);
		
		return dialogArea;
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
			MessageDialog.openError(getShell(), "Error", "Please enter polling pollingInterval as integer in range 2 .. 10000");
		}
		
		try
		{
			retentionTime = Integer.parseInt(textRetention.getText());
			if ((retentionTime < 1) || (retentionTime > 10000))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openError(getShell(), "Error", "Please enter retention time as integer in range 1 .. 10000");
		}
		
		dciInfo = new InterfaceDciInfo[forms.length];
		for(int i = 0; i < forms.length; i++)
		{
			dciInfo[i] = new InterfaceDciInfo(forms[i].isDciEnabled(), forms[i].isDelta(), forms[i].getDescription());
		}
		
		super.okPressed();
	}

	/**
	 * @return the pollingInterval
	 */
	public int getPollingInterval()
	{
		return pollingInterval;
	}

	/**
	 * @return the retentionTime
	 */
	public int getRetentionTime()
	{
		return retentionTime;
	}

	/**
	 * @return the dciInfo
	 */
	public InterfaceDciInfo[] getDciInfo()
	{
		return dciInfo;
	}

	/**
	 * Composite for single interface DCI
	 */
	private class InterfaceDciForm extends Composite
	{
		private Button checkEnable;
		private Button checkDelta;
		private Text description;
		
		public InterfaceDciForm(Composite parent, String name, String defaultDescription, boolean enabled)
		{
			super(parent, SWT.NONE);
			GridLayout layout = new GridLayout();
			layout.marginHeight = 0;
			layout.marginWidth = 0;
			setLayout(layout);
			
			final Composite buttonRow = new Composite(this, SWT.NONE);
			layout = new GridLayout();
			layout.numColumns = 2;
			layout.marginHeight = 0;
			layout.marginWidth = 0;
			buttonRow.setLayout(layout);
			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			buttonRow.setLayoutData(gd);
			
			checkEnable = new Button(buttonRow, SWT.CHECK);
			checkEnable.setText(name);
			checkEnable.setSelection(enabled);
			
			checkDelta = new Button(buttonRow, SWT.CHECK);
			checkDelta.setText("Delta value (average per second)");
			checkDelta.setSelection(true);
			checkDelta.setEnabled(enabled);
			gd = new GridData();
			gd.horizontalAlignment = SWT.RIGHT;
			checkDelta.setLayoutData(gd);
			
			final Composite textRow = new Composite(this, SWT.NONE);
			layout = new GridLayout();
			layout.numColumns = 2;
			layout.marginHeight = 0;
			layout.marginWidth = 0;
			textRow.setLayout(layout);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			textRow.setLayoutData(gd);
			
			new Label(textRow, SWT.NONE).setText("Description:");
			
			description = new Text(textRow, SWT.BORDER);
			description.setText(defaultDescription);
			description.setTextLimit(255);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			description.setLayoutData(gd);
			description.setEnabled(enabled);
			
			checkEnable.addSelectionListener(new SelectionListener() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					boolean enabled = checkEnable.getSelection();
					checkDelta.setEnabled(enabled);
					description.setEnabled(enabled);
				}
				
				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
		/**
		 * @return
		 */
		public boolean isDciEnabled()
		{
			return checkEnable.getSelection();
		}
		
		/**
		 * @return
		 */
		public boolean isDelta()
		{
			return checkDelta.getSelection();
		}
		
		/**
		 * @return
		 */
		public String getDescription()
		{
			return description.getText();
		}
	}
}
