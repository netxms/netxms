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
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;
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
	private LabeledText dateTimeExample;
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
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		dateTimeFormat = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.getString("RegionalSettingsPrefPage.DateTimeFormat"), gd); //$NON-NLS-1$
		dateTimeFormat.add(Messages.getString("RegionalSettingsPrefPage.FmtServer")); //$NON-NLS-1$
		dateTimeFormat.add(Messages.getString("RegionalSettingsPrefPage.FmtJava")); //$NON-NLS-1$
		dateTimeFormat.add(Messages.getString("RegionalSettingsPrefPage.FmtCustom")); //$NON-NLS-1$
		format = getPreferenceStore().getInt("DATETIME_FORMAT"); //$NON-NLS-1$
		dateTimeFormat.select(format);
		dateTimeFormat.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				format = dateTimeFormat.getSelectionIndex();
				dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
				timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
				updateExample(null);
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
		dateFormatString.setLabel(Messages.getString("RegionalSettingsPrefPage.DateFormatString")); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dateFormatString.setLayoutData(gd);
		dateFormatString.setText(getPreferenceStore().getString("DATE_FORMAT_STRING")); //$NON-NLS-1$
		dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		dateFormatString.getTextControl().addModifyListener(listener);
		
		timeFormatString = new LabeledText(dialogArea, SWT.NONE);
		timeFormatString.setLabel(Messages.getString("RegionalSettingsPrefPage.TimeFormatString")); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		timeFormatString.setLayoutData(gd);
		timeFormatString.setText(getPreferenceStore().getString("TIME_FORMAT_STRING")); //$NON-NLS-1$
		timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		timeFormatString.getTextControl().addModifyListener(listener);
		
		dateTimeExample = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
		dateTimeExample.setLabel(Messages.getString("RegionalSettingsPrefPage.Example")); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		dateTimeExample.setLayoutData(gd);
		updateExample(null);
		
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
						updatedControl.setBackground(SharedColors.RED);
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
		dateFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
		timeFormatString.setEnabled(format == RegionalSettings.DATETIME_FORMAT_CUSTOM);
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
		
		RegionalSettings.updateFromPreferences();
		
		return true;
	}
}
