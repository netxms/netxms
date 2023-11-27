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
package org.netxms.nxmc.base.preferencepages;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Regional settings page
 */
public class RegionalSettingsPage extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RegionalSettingsPage.class);

	private Combo dateTimeFormat;
	private LabeledText dateFormatString;
	private LabeledText timeFormatString;
   private LabeledText shortTimeFormatString;
	private LabeledText dateTimeExample;
	private LabeledText shortTimeExample;
	private Button checkServerTimeZone;
	private int format;

   public RegionalSettingsPage()
   {
      super(LocalizationHelper.getI18n(RegionalSettingsPage.class).tr("Regional Settings"));
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
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
		dateTimeFormat = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Date and time format"), gd); 
		dateTimeFormat.add(i18n.tr("Defined by server")); 
		dateTimeFormat.add(i18n.tr("JVM default locale")); 
		dateTimeFormat.add(i18n.tr("Custom")); 
		format = getPreferenceStore().getInt("DateFormatFactory.Format.DateTime"); 
		dateTimeFormat.select(format);
		dateTimeFormat.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				format = dateTimeFormat.getSelectionIndex();
				dateFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
				timeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
            shortTimeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
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
		dateFormatString.setLabel(i18n.tr("Date format string")); 
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dateFormatString.setLayoutData(gd);
		dateFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.Date")); 
		dateFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
		dateFormatString.getTextControl().addModifyListener(listener);
		
		timeFormatString = new LabeledText(dialogArea, SWT.NONE);
		timeFormatString.setLabel(i18n.tr("Time format string")); 
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		timeFormatString.setLayoutData(gd);
		timeFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.Time")); 
		timeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
		timeFormatString.getTextControl().addModifyListener(listener);

      shortTimeFormatString = new LabeledText(dialogArea, SWT.NONE);
      shortTimeFormatString.setLabel(i18n.tr("Short time format string"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      shortTimeFormatString.setLayoutData(gd);
      shortTimeFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.ShortTime")); 
      shortTimeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
      shortTimeFormatString.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateShortTimeExample((Control)e.widget);
         }
      });
		
		dateTimeExample = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
		dateTimeExample.setLabel(i18n.tr("Date and time formatting example")); 
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		dateTimeExample.setLayoutData(gd);

      shortTimeExample = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      shortTimeExample.setLabel(i18n.tr("Short time example"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      shortTimeExample.setLayoutData(gd);
      
      checkServerTimeZone = new Button(dialogArea, SWT.CHECK);
      checkServerTimeZone.setText(i18n.tr("Use server time &zone"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      checkServerTimeZone.setLayoutData(gd);
      checkServerTimeZone.setSelection(getPreferenceStore().getBoolean("DateFormatFactory.UseServerTimeZone")); 
		
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
			case DateFormatFactory.DATETIME_FORMAT_SERVER:
				NXCSession session = Registry.getSession();
				df = new SimpleDateFormat(session.getDateFormat() + " " + session.getTimeFormat()); 
				break;
			case DateFormatFactory.DATETIME_FORMAT_CUSTOM:
				try
				{
					df = new SimpleDateFormat(dateFormatString.getText() + " " + timeFormatString.getText()); 
				}
				catch(IllegalArgumentException e)
				{
					setErrorMessage(e.getLocalizedMessage());
					if (updatedControl != null)
                  updatedControl.setBackground(ThemeEngine.getBackgroundColor("TextInput.Error"));
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
         case DateFormatFactory.DATETIME_FORMAT_SERVER:
            NXCSession session = Registry.getSession();
            df = new SimpleDateFormat(session.getShortTimeFormat());
            break;
         case DateFormatFactory.DATETIME_FORMAT_CUSTOM:
            try
            {
               df = new SimpleDateFormat(shortTimeFormatString.getText());
            }
            catch(IllegalArgumentException e)
            {
               setErrorMessage(e.getLocalizedMessage());
               if (updatedControl != null)
                  updatedControl.setBackground(ThemeEngine.getBackgroundColor("TextInput.Error"));
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		format = getPreferenceStore().getInt("DateFormatFactory.Format.DateTime"); 
		dateTimeFormat.select(format);
		dateFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.Date")); 
		timeFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.Time")); 
      shortTimeFormatString.setText(getPreferenceStore().getString("DateFormatFactory.Format.ShortTime")); 
		dateFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
		timeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
      shortTimeFormatString.setEnabled(format == DateFormatFactory.DATETIME_FORMAT_CUSTOM);
      checkServerTimeZone.setSelection(false);
		updateExample(null);
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		final IPreferenceStore ps = getPreferenceStore();

		ps.setValue("DateFormatFactory.Format.DateTime", format); 
		ps.setValue("DateFormatFactory.Format.Date", dateFormatString.getText()); 
		ps.setValue("DateFormatFactory.Format.Time", timeFormatString.getText()); 
      ps.setValue("DateFormatFactory.Format.ShortTime", shortTimeFormatString.getText()); 
      ps.setValue("DateFormatFactory.UseServerTimeZone", checkServerTimeZone.getSelection()); 

		DateFormatFactory.updateFromPreferences();

		return true;
	}
}
