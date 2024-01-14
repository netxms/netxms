/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.preferencepage;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.IntegerFieldEditor;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * General preferences page for network maps
 *
 */
public class GeneralMapPreferences extends FieldEditorPreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(GeneralMapPreferences.class);

   public GeneralMapPreferences()
   {
      super(LocalizationHelper.getI18n(GeneralMapPreferences.class).tr("Network Maps"), FieldEditorPreferencePage.FLAT);
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
	@Override
	protected void createFieldEditors()
	{
      addField(new BooleanFieldEditor("NetMap.ShowStatusIcon", i18n.tr("Show status icon on objects"), getFieldEditorParent()));
      addField(new BooleanFieldEditor("NetMap.ShowStatusFrame", i18n.tr("Show status frame around objects"), getFieldEditorParent()));
      addField(new BooleanFieldEditor("NetMap.ShowStatusBackground", i18n.tr("Show status background under objects"), getFieldEditorParent()));
      addField(new BooleanFieldEditor("NetMap.TranslucentLabelBkgnd", i18n.tr("Translucent label background"), getFieldEditorParent()));
      addField(new IntegerFieldEditor("NetMap.DefaultLinkWidth", i18n.tr("Default link width"), getFieldEditorParent(), 3));
	}
}
