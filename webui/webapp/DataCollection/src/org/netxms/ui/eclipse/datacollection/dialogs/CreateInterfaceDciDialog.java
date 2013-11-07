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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
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
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.InterfaceDciInfo;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating DCIs for network interface
 */
public class CreateInterfaceDciDialog extends Dialog
{
	private static final String[] names = 
		{ 
			Messages.CreateInterfaceDciDialog_InBytes, 
			Messages.CreateInterfaceDciDialog_OutBytes,
			Messages.CreateInterfaceDciDialog_InPackets, 
			Messages.CreateInterfaceDciDialog_OutPackets,
			Messages.CreateInterfaceDciDialog_InErrors,
			Messages.CreateInterfaceDciDialog_OutErrors
		};
	private static final String[] descriptions = 
		{ 
			Messages.CreateInterfaceDciDialog_InBytesDescr, 
			Messages.CreateInterfaceDciDialog_OutBytesDescr,
			Messages.CreateInterfaceDciDialog_InPacketsDescr, 
			Messages.CreateInterfaceDciDialog_OutPacketsDescr,
			Messages.CreateInterfaceDciDialog_InErrorsDescr,
			Messages.CreateInterfaceDciDialog_OutErrorsDescr
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
		dataGroup.setText(Messages.CreateInterfaceDciDialog_Data);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataGroup.setLayoutData(gd);
		layout = new GridLayout();
		dataGroup.setLayout(layout);
		
		for(int i = 0; i < names.length; i++)
		{
			forms[i] = new InterfaceDciForm(dataGroup, names[i], 
					(object != null) ? descriptions[i].replaceAll("@@ifName@@", object.getObjectName()) : descriptions[i], defaultEnabled[i]); //$NON-NLS-1$
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			forms[i].setLayoutData(gd);
		}
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.CreateInterfaceDciDialog_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		optionsGroup.setLayout(layout);
		
		textInterval = new LabeledText(optionsGroup, SWT.NONE);
		textInterval.setLabel(Messages.CreateInterfaceDciDialog_PollingInterval);
		textInterval.setText("60"); //$NON-NLS-1$
		textInterval.getTextControl().setTextLimit(5);
		
		textRetention = new LabeledText(optionsGroup, SWT.NONE);
		textRetention.setLabel(Messages.CreateInterfaceDciDialog_RetentionTime);
		textRetention.setText("30"); //$NON-NLS-1$
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
			MessageDialogHelper.openError(getShell(), Messages.CreateInterfaceDciDialog_Error, Messages.CreateInterfaceDciDialog_BadPollingInterval);
		}
		
		try
		{
			retentionTime = Integer.parseInt(textRetention.getText());
			if ((retentionTime < 1) || (retentionTime > 10000))
				throw new NumberFormatException();
		}
		catch(NumberFormatException e)
		{
			MessageDialogHelper.openError(getShell(), Messages.CreateInterfaceDciDialog_Error, Messages.CreateInterfaceDciDialog_BadRetentionTime);
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
			checkDelta.setText(Messages.CreateInterfaceDciDialog_Delta);
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
			
			new Label(textRow, SWT.NONE).setText(Messages.CreateInterfaceDciDialog_Description);
			
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
