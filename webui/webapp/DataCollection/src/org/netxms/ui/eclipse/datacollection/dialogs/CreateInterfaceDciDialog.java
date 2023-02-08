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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.InterfaceDciInfo;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for creating DCIs for network interface
 */
public class CreateInterfaceDciDialog extends Dialog
{
	private final String[] names = 
		{ 
			Messages.get().CreateInterfaceDciDialog_InBytes,
			Messages.get().CreateInterfaceDciDialog_OutBytes,
			Messages.get().CreateInterfaceDciDialog_InBits,
			Messages.get().CreateInterfaceDciDialog_OutBits,
			Messages.get().CreateInterfaceDciDialog_InPackets, 
			Messages.get().CreateInterfaceDciDialog_OutPackets,
			Messages.get().CreateInterfaceDciDialog_InErrors,
			Messages.get().CreateInterfaceDciDialog_OutErrors
		};
	private final String[] descriptions = 
		{ 
			Messages.get().CreateInterfaceDciDialog_InBytesDescr,
			Messages.get().CreateInterfaceDciDialog_OutBytesDescr,
			Messages.get().CreateInterfaceDciDialog_InBitsDescr,
			Messages.get().CreateInterfaceDciDialog_OutBitsDescr,
			Messages.get().CreateInterfaceDciDialog_InPacketsDescr, 
			Messages.get().CreateInterfaceDciDialog_OutPacketsDescr,
			Messages.get().CreateInterfaceDciDialog_InErrorsDescr,
			Messages.get().CreateInterfaceDciDialog_OutErrorsDescr
		};
	private static final boolean[] DEFAULT_ENABLED = { true, true, false, false, false, false, false, false };

	private Interface object;
	private InterfaceDciForm[] forms = new InterfaceDciForm[names.length];
   private Combo pollingScheduleTypeSelector;
   private LabeledText textInterval;
   private Combo retentionTypeSelector;
   private LabeledText textRetention;

   private int pollingScheduleType;
	private int pollingInterval;
   private int retentionType;
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(Messages.get().CreateInterfaceDciDialog_Title);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		final boolean enabled[] = new boolean[DEFAULT_ENABLED.length];
      final boolean delta[] = new boolean[DEFAULT_ENABLED.length];
		for(int i = 0; i < enabled.length; i++)
		{
		   String v = settings.get("CreateInterfaceDciDialog.enabled_" + i); //$NON-NLS-1$
		   if (v != null)
		   {
		      enabled[i] = Boolean.parseBoolean(v);
		   }
		   else
		   {
		      enabled[i] = DEFAULT_ENABLED[i];
		   }

         v = settings.get("CreateInterfaceDciDialog.delta_" + i); //$NON-NLS-1$
         if (v != null)
         {
            delta[i] = Boolean.parseBoolean(v);
         }
         else
         {
            delta[i] = true;
         }
		}

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

		Group dataGroup = new Group(dialogArea, SWT.NONE);
		dataGroup.setText(Messages.get().CreateInterfaceDciDialog_Data);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataGroup.setLayoutData(gd);
		layout = new GridLayout();
		dataGroup.setLayout(layout);

		for(int i = 0; i < names.length; i++)
		{
         if (i > 0)
         {
            Label sep = new Label(dataGroup, SWT.SEPARATOR | SWT.HORIZONTAL);
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            sep.setLayoutData(gd);
         }

			forms[i] = new InterfaceDciForm(dataGroup, names[i], 
               (object != null) ? descriptions[i].replaceAll("@@ifName@@", object.getObjectName()) : descriptions[i], enabled[i], delta[i]); //$NON-NLS-1$
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			forms[i].setLayoutData(gd);
		}

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().CreateInterfaceDciDialog_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		optionsGroup.setLayout(layout);
		
		int savedPollingScheduleType;
		try
		{
         savedPollingScheduleType = settings.getInt("CreateInterfaceDciDialog.pollingScheduleType");
		}
      catch(NumberFormatException e)
      {
         savedPollingScheduleType = DataCollectionObject.POLLING_SCHEDULE_DEFAULT;
      }

      pollingScheduleTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, "Polling interval", WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollingScheduleTypeSelector.add("Default");
      pollingScheduleTypeSelector.add("Custom");
      pollingScheduleTypeSelector.select(savedPollingScheduleType);
      pollingScheduleTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textInterval.setEnabled(pollingScheduleTypeSelector.getSelectionIndex() == 1);
         }
      });

      int savedRetentionType;
      try
      {
         savedRetentionType = settings.getInt("CreateInterfaceDciDialog.retentionType");
      }
      catch(NumberFormatException e)
      {
         savedRetentionType = DataCollectionObject.RETENTION_DEFAULT;
      }

      retentionTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, "Retention time", WidgetHelper.DEFAULT_LAYOUT_DATA);
      retentionTypeSelector.add(Messages.get().General_UseDefaultRetention);
      retentionTypeSelector.add(Messages.get().General_UseCustomRetention);
      retentionTypeSelector.add(Messages.get().General_NoStorage);
      retentionTypeSelector.select(savedRetentionType);
      retentionTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textRetention.setEnabled(retentionTypeSelector.getSelectionIndex() == 1);
         }
      });

      textInterval = new LabeledText(optionsGroup, SWT.NONE);
      textInterval.setLabel("Custom polling interval (seconds)");
      String v = settings.get("CreateInterfaceDciDialog.pollingInterval"); //$NON-NLS-1$
      textInterval.setText((v != null) ? v : "60"); //$NON-NLS-1$
      textInterval.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textInterval.setLayoutData(gd);
      textInterval.setEnabled(false);

      textRetention = new LabeledText(optionsGroup, SWT.NONE);
      textRetention.setLabel("Custom retention time (days)");
      v = settings.get("CreateInterfaceDciDialog.retentionTime"); //$NON-NLS-1$
      textRetention.setText((v != null) ? v : "30"); //$NON-NLS-1$
      textRetention.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textRetention.setLayoutData(gd);
      textRetention.setEnabled(false);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();

      pollingScheduleType = pollingScheduleTypeSelector.getSelectionIndex();
      try
      {
         pollingInterval = Integer.parseInt(textInterval.getText());
         if ((pollingInterval < 2) || (pollingInterval > 10000))
            throw new NumberFormatException();
      }
      catch(NumberFormatException e)
      {
         if (pollingScheduleType == DataCollectionObject.POLLING_SCHEDULE_CUSTOM)
            MessageDialogHelper.openError(getShell(), Messages.get().CreateInterfaceDciDialog_Error, Messages.get().CreateInterfaceDciDialog_BadPollingInterval);
         else
            pollingInterval = 60;
      }

      retentionType = retentionTypeSelector.getSelectionIndex();
      try
      {
         retentionTime = Integer.parseInt(textRetention.getText());
         if ((retentionTime < 1) || (retentionTime > 10000))
            throw new NumberFormatException();
      }
      catch(NumberFormatException e)
      {
         if (retentionType == DataCollectionObject.RETENTION_CUSTOM)
            MessageDialogHelper.openError(getShell(), Messages.get().CreateInterfaceDciDialog_Error, Messages.get().CreateInterfaceDciDialog_BadRetentionTime);
         else
            retentionTime = 30;
      }

      settings.put("CreateInterfaceDciDialog.pollingScheduleType", pollingScheduleType); //$NON-NLS-1$
		settings.put("CreateInterfaceDciDialog.pollingInterval", pollingInterval); //$NON-NLS-1$
      settings.put("CreateInterfaceDciDialog.retentionType", retentionType); //$NON-NLS-1$
      settings.put("CreateInterfaceDciDialog.retentionTime", retentionTime); //$NON-NLS-1$

		dciInfo = new InterfaceDciInfo[forms.length];
		for(int i = 0; i < forms.length; i++)
		{
         dciInfo[i] = new InterfaceDciInfo(forms[i].isDciEnabled(), forms[i].isDelta(), forms[i].getDescription());
			settings.put("CreateInterfaceDciDialog.enabled_" + i, forms[i].isDciEnabled()); //$NON-NLS-1$
         settings.put("CreateInterfaceDciDialog.delta_" + i, forms[i].isDelta()); //$NON-NLS-1$
         settings.put("CreateInterfaceDciDialog.description_" + i, forms[i].getDescription()); //$NON-NLS-1$
		}

		super.okPressed();
	}

	/**
    * @return the pollingScheduleType
    */
   public int getPollingScheduleType()
   {
      return pollingScheduleType;
   }

   /**
    * @return the pollingInterval
    */
	public int getPollingInterval()
	{
		return pollingInterval;
	}

   /**
    * @return the retentionType
    */
   public int getRetentionType()
   {
      return retentionType;
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

      public InterfaceDciForm(Composite parent, String name, String defaultDescription, boolean enabled, boolean delta)
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
			checkDelta.setText(Messages.get().CreateInterfaceDciDialog_Delta);
         checkDelta.setSelection(delta);
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
			
			new Label(textRow, SWT.NONE).setText(Messages.get().CreateInterfaceDciDialog_Description);
			
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
