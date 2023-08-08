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
package org.netxms.nxmc.modules.objects.preferencepages;

import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.editors.TimePeriodEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Object maintenance" preference page
 */
public class MaintenancePreferences extends FieldEditorPreferencePage
{
   private static final I18n i18n = LocalizationHelper.getI18n(MaintenancePreferences.class);

   public MaintenancePreferences()
   {
      super(i18n.tr("Object Maintenance"), FieldEditorPreferencePage.FLAT);
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
	@Override
	protected void createFieldEditors()
	{
      addField(new TimePeriodEditor("Maintenance.TimeEditor", i18n.tr("Predefined maintenance periods"), getFieldEditorParent(), "Maintenance.TimeMenuSize", "Maintenance.TimeMenuEntry."));
	}
}
