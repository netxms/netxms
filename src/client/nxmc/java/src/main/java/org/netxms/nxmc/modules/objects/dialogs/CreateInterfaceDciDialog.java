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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
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
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.helpers.InterfaceDciInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating DCIs for network interface
 */
public class CreateInterfaceDciDialog extends Dialog
{
   private static final boolean[] DEFAULT_ENABLED = { true, true, false, false, false, false, false, false };

   private final I18n i18n = LocalizationHelper.getI18n(CreateInterfaceDciDialog.class);
   private final String[] names =
		{ 
		   i18n.tr("Inbound traffic (bytes)"),
         i18n.tr("Outbound traffic (bytes)"),
         i18n.tr("Inbound traffic (bits)"),
         i18n.tr("Outbound traffic (bits)"),
         i18n.tr("Inbound traffic (packets)"),
         i18n.tr("Outbound traffic (packets)"),
         i18n.tr("Input errors"),
         i18n.tr("Output errors")
		};
   private final String[] descriptions = 
		{ 
			i18n.tr("Inbound traffic on @@ifName@@"),
         i18n.tr("Outbound traffic on @@ifName@@"),
         i18n.tr("Inbound traffic on @@ifName@@"),
         i18n.tr("Outbound traffic on @@ifName@@"),
         i18n.tr("Inbound traffic on @@ifName@@"),
         i18n.tr("Outbound traffic on @@ifName@@"),
         i18n.tr("Input error rate on @@ifName@@"),
         i18n.tr("Output error rate on @@ifName@@")
		};

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
      newShell.setText(i18n.tr("Create Interface DCI"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

      final PreferenceStore settings = PreferenceStore.getInstance();
		final boolean enabled[] = new boolean[DEFAULT_ENABLED.length];
      final boolean delta[] = new boolean[DEFAULT_ENABLED.length];
		for(int i = 0; i < enabled.length; i++)
		{
         enabled[i] = settings.getAsBoolean("CreateInterfaceDciDialog.enabled_" + i, DEFAULT_ENABLED[i]);
         delta[i] = settings.getAsBoolean("CreateInterfaceDciDialog.delta_" + i, true);
		}

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

		Group dataGroup = new Group(dialogArea, SWT.NONE);
      dataGroup.setText(i18n.tr("Data"));
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
               (object != null) ? descriptions[i].replaceAll("@@ifName@@", object.getObjectName()) : descriptions[i], enabled[i], delta[i]);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			forms[i].setLayoutData(gd);
		}

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		optionsGroup.setLayout(layout);

      pollingScheduleTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Polling interval"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollingScheduleTypeSelector.add(i18n.tr("Default"));
      pollingScheduleTypeSelector.add(i18n.tr("Custom"));
      pollingScheduleTypeSelector.select(settings.getAsInteger("CreateInterfaceDciDialog.pollingScheduleType", DataCollectionObject.POLLING_SCHEDULE_DEFAULT));
      pollingScheduleTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textInterval.setEnabled(pollingScheduleTypeSelector.getSelectionIndex() == 1);
         }
      });

      retentionTypeSelector = WidgetHelper.createLabeledCombo(optionsGroup, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Retention time"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      retentionTypeSelector.add(i18n.tr("Use default retention time"));
      retentionTypeSelector.add(i18n.tr("Use custom retention time"));
      retentionTypeSelector.add(i18n.tr("Do not save collected data to database"));
      retentionTypeSelector.select(settings.getAsInteger("CreateInterfaceDciDialog.retentionType", DataCollectionObject.RETENTION_DEFAULT));
      retentionTypeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textRetention.setEnabled(retentionTypeSelector.getSelectionIndex() == 1);
         }
      });

      textInterval = new LabeledText(optionsGroup, SWT.NONE);
      textInterval.setLabel(i18n.tr("Custom polling interval (seconds)"));
      textInterval.setText(Integer.toString(settings.getAsInteger("CreateInterfaceDciDialog.pollingInterval", 60)));
      textInterval.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textInterval.setLayoutData(gd);
      textInterval.setEnabled(false);

      textRetention = new LabeledText(optionsGroup, SWT.NONE);
      textRetention.setLabel(i18n.tr("Custom retention time (days)"));
      textRetention.setText(Integer.toString(settings.getAsInteger("CreateInterfaceDciDialog.retentionTime", 30)));
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
      final PreferenceStore settings = PreferenceStore.getInstance();

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
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter polling pollingInterval as integer in range 2 .. 10000"));
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
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter retention time as integer in range 1 .. 10000"));
         else
            retentionTime = 30;
      }

      settings.set("CreateInterfaceDciDialog.pollingScheduleType", pollingScheduleType);
      settings.set("CreateInterfaceDciDialog.pollingInterval", pollingInterval);
      settings.set("CreateInterfaceDciDialog.retentionType", retentionType);
      settings.set("CreateInterfaceDciDialog.retentionTime", retentionTime);

		dciInfo = new InterfaceDciInfo[forms.length];
		for(int i = 0; i < forms.length; i++)
		{
         dciInfo[i] = new InterfaceDciInfo(forms[i].isDciEnabled(), forms[i].isDelta(), forms[i].getDescription());
         settings.set("CreateInterfaceDciDialog.enabled_" + i, forms[i].isDciEnabled());
         settings.set("CreateInterfaceDciDialog.delta_" + i, forms[i].isDelta());
         settings.set("CreateInterfaceDciDialog.description_" + i, forms[i].getDescription());
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
         checkDelta.setText(i18n.tr("Delta value (average per second)"));
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

         new Label(textRow, SWT.NONE).setText(i18n.tr("Description:"));

			description = new Text(textRow, SWT.BORDER);
			description.setText(defaultDescription);
			description.setTextLimit(255);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			description.setLayoutData(gd);
			description.setEnabled(enabled);

         checkEnable.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					boolean enabled = checkEnable.getSelection();
					checkDelta.setEnabled(enabled);
					description.setEnabled(enabled);
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
