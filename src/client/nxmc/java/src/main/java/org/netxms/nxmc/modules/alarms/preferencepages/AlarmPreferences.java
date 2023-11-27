/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.preferencepages;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.editors.TimePeriodEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Alarms" preference page
 */
public class AlarmPreferences extends FieldEditorPreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmPreferences.class);

   public AlarmPreferences()
   {
      super(LocalizationHelper.getI18n(AlarmPreferences.class).tr("Alarms"), FieldEditorPreferencePage.FLAT);
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
	@Override
	protected void createFieldEditors()
	{
		addField(new BooleanFieldEditor("AlarmList.BlinkOutstandingAlarm", i18n.tr("Blinking outstanding alarms"), getFieldEditorParent()));
		addField(new BooleanFieldEditor("TrayIcon.ShowAlarmPopups", i18n.tr("Show tray pop-ups on new alarms"), getFieldEditorParent()));
		addField(new BooleanFieldEditor("AlarmNotifier.OutstandingAlarmsReminder", i18n.tr("Show pop-up reminder for outstanding alarms"), getFieldEditorParent()));
      if ((Registry.getSession()).isTimedAlarmAckEnabled())
      {
         addField(new TimePeriodEditor("Alarm.TimeEditor", i18n.tr("Predefined acknowledge timeouts"), getFieldEditorParent(), "AlarmList.AckMenuSize", "AlarmList.AckMenuEntry."));
      }
	}
}
