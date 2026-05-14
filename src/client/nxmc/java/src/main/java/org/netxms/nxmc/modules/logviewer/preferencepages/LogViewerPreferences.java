/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logviewer.preferencepages;

import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.IntegerFieldEditor;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Log Viewer" preference page
 */
public class LogViewerPreferences extends FieldEditorPreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(LogViewerPreferences.class);

   public LogViewerPreferences()
   {
      super(LocalizationHelper.getI18n(LogViewerPreferences.class).tr("Log Viewer"), FieldEditorPreferencePage.FLAT);
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
   @Override
   protected void createFieldEditors()
   {
      IntegerFieldEditor pageSize = new IntegerFieldEditor("LogViewer.PageSize", i18n.tr("Page size (records per request)"), getFieldEditorParent(), 6);
      pageSize.setValidRange(50, 10000);
      addField(pageSize);
   }
}
