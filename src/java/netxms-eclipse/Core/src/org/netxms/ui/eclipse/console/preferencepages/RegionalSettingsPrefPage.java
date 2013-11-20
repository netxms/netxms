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
package org.netxms.ui.eclipse.console.preferencepages;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.api.client.Session;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Regional settings page
 */
public class RegionalSettingsPrefPage extends PreferencePage implements IWorkbenchPreferencePage
{
	private Combo dateTimeFormat;
	private LabeledText dateFormatString;
	private LabeledText timeFormatString;
   private LabeledText shortTimeFormatString;
	private LabeledText dateTimeExample;
	private LabeledText shortTimeExample;
	private int format;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.numColumns = 3;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 3;
		dateTimeFormat = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().RegionalSettingsPrefPage_DateTimeFormat, gd); //$NON-NLS-1$
		dateTimeFormat.add(Messages.get().RegionalSettingsPrefPage_FmtServer); //$NON-NLS-1$
		dateTimeFormat.add(Messages.get().RegionalSettingsPrefPage_FmtJava); //$NON-NLS-1$
		dateTimeFormat.add(Messages.get().RegionalSettingsPrefPage_FmtCustom); //$NON-NLS-1$
		format = getPreferenceStore().getInt("DATETIME_FORMAT"); //$NON-NLS-1$
		dateTimeFormat.select(format);
		dateTimeFormat.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				format = dateTimeFormat.getSelectionIndex();
				dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
				timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
            shortTimeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
				updateExample(null);
				updateShortTimeExample(null);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		ModifyListener listener = new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				updateExample((Control)e.widget);
			}
		};
		
		dateFormatString = new LabeledText(dialogArea, SWT.NONE);
		dateFormatString.setLabel(Messages.get().RegionalSettingsPrefPage_DateFormatString); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dateFormatString.setLayoutData(gd);
		dateFormatString.setText(getPreferenceStore().getString("DATE_FORMAT_STRING")); //$NON-NLS-1$
		dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		dateFormatString.getTextControl().addModifyListener(listener);
		
		timeFormatString = new LabeledText(dialogArea, SWT.NONE);
		timeFormatString.setLabel(Messages.get().RegionalSettingsPrefPage_TimeFormatString); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		timeFormatString.setLayoutData(gd);
		timeFormatString.setText(getPreferenceStore().getString("TIME_FORMAT_STRING")); //$NON-NLS-1$
		timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		timeFormatString.getTextControl().addModifyListener(listener);

      shortTimeFormatString = new LabeledText(dialogArea, SWT.NONE);
      shortTimeFormatString.setLabel(Messages.get().RegionalSettingsPrefPage_ShortTimeFormatString);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      shortTimeFormatString.setLayoutData(gd);
      shortTimeFormatString.setText(getPreferenceStore().getString("SHORT_TIME_FORMAT_STRING")); //$NON-NLS-1$
      shortTimeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
      shortTimeFormatString.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateShortTimeExample((Control)e.widget);
         }
      });
		
		dateTimeExample = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
		dateTimeExample.setLabel(Messages.get().RegionalSettingsPrefPage_Example); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		dateTimeExample.setLayoutData(gd);

      shortTimeExample = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      shortTimeExample.setLabel(Messages.get().RegionalSettingsPrefPage_ShortTimeExample);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      shortTimeExample.setLayoutData(gd);
		
		updateExample(null);
      updateShortTimeExample(null);
		
		return dialogArea;
	}

	/**
	 * Update formatting example
	 */
	private void updateExample(Control updatedControl)
	{
		DateFormat df;
		switch(format)
		{
			case RegionalSettings.DATETIME_FORMAT_SERVER:
				Session session = ConsoleSharedData.getSession();
				df = new SimpleDateFormat(session.getDateFormat() + " " + session.getTimeFormat()); //$NON-NLS-1$
				break;
			case RegionalSettings.DATETIME_FORMAT_CUSTOM:
				try
				{
					df = new SimpleDateFormat(dateFormatString.getText() + " " + timeFormatString.getText()); //$NON-NLS-1$
				}
				catch(IllegalArgumentException e)
				{
					setErrorMessage(e.getLocalizedMessage());
					if (updatedControl != null)
						updatedControl.setBackground(SharedColors.getColor(SharedColors.ERROR_BACKGROUND, getShell().getDisplay()));
					return;
				}
				break;
			default:
				df = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM);
				break;
		}
		dateTimeExample.setText(df.format(new Date(System.currentTimeMillis())));
		
		setErrorMessage(null);
		dateFormatString.getTextControl().setBackground(null);
		timeFormatString.getTextControl().setBackground(null);
	}

   /**
    * Update formatting example
    */
   private void updateShortTimeExample(Control updatedControl)
   {
      DateFormat df;
      switch(format)
      {
         case RegionalSettings.DATETIME_FORMAT_SERVER:
            NXCSession session = (NXCSession)ConsoleSharedData.getSession();
            df = new SimpleDateFormat(session.getShortTimeFormat());
            break;
         case RegionalSettings.DATETIME_FORMAT_CUSTOM:
            try
            {
               df = new SimpleDateFormat(shortTimeFormatString.getText());
            }
            catch(IllegalArgumentException e)
            {
               setErrorMessage(e.getLocalizedMessage());
               if (updatedControl != null)
                  updatedControl.setBackground(SharedColors.getColor(SharedColors.ERROR_BACKGROUND, getShell().getDisplay()));
               return;
            }
            break;
         default:
            df = DateFormat.getTimeInstance(DateFormat.SHORT);
            break;
      }
      shortTimeExample.setText(df.format(new Date(System.currentTimeMillis())));
      
      setErrorMessage(null);
      shortTimeFormatString.getTextControl().setBackground(null);
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		format = getPreferenceStore().getInt("DATETIME_FORMAT"); //$NON-NLS-1$
		dateTimeFormat.select(format);
		dateFormatString.setText(getPreferenceStore().getString("DATE_FORMAT_STRING")); //$NON-NLS-1$
		timeFormatString.setText(getPreferenceStore().getString("TIME_FORMAT_STRING")); //$NON-NLS-1$
      shortTimeFormatString.setText(getPreferenceStore().getString("SHORT_TIME_FORMAT_STRING")); //$NON-NLS-1$
		dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
      shortTimeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		updateExample(null);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		final IPreferenceStore ps = getPreferenceStore();
		
		ps.setValue("DATETIME_FORMAT", format); //$NON-NLS-1$
		ps.setValue("DATE_FORMAT_STRING", dateFormatString.getText()); //$NON-NLS-1$
		ps.setValue("TIME_FORMAT_STRING", timeFormatString.getText()); //$NON-NLS-1$
      ps.setValue("SHORT_TIME_FORMAT_STRING", shortTimeFormatString.getText()); //$NON-NLS-1$
		
		RegionalSettings.updateFromPreferences();
		
		return true;
	}
}
