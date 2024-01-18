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
package org.netxms.nxmc.modules.reporting.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm state field editor
 */
public class AlarmStateFieldEditor extends ReportFieldEditor
{
   private I18n i18n;
   private Combo state;

   /**
    * @param parameter
    * @param toolkit
    * @param parent
    */
   public AlarmStateFieldEditor(ReportParameter parameter, Composite parent)
   {
      super(parameter, parent);
   }

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(AlarmStateFieldEditor.class);
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContent(Composite parent)
   {
      state = new Combo(parent, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY);
      state.add("");
      state.add(i18n.tr("Outstanding"));
      state.add(i18n.tr("Acknowledged"));
      state.add(i18n.tr("Resolved"));
      state.add(i18n.tr("Terminated"));
      state.select(0);
      return state;
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#getValue()
    */
   @Override
   public String getValue()
   {
      return Integer.toString(state.getSelectionIndex() - 1);
   }
}
