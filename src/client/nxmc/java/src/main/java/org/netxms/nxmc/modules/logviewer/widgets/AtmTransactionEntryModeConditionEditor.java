/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.views.helpers.LogLabelProvider;
import org.xnap.commons.i18n.I18n;

/**
 * Condition editor for ATM transaction entry mode column
 */
public class AtmTransactionEntryModeConditionEditor extends ConditionEditor
{
   private final I18n i18n = LocalizationHelper.getI18n(AtmTransactionEntryModeConditionEditor.class);
   private final String[] OPERATIONS = { i18n.tr("IS"), i18n.tr("IS NOT") };

   private Combo entryModeCombo;

   /**
    * @param parent
    */
   public AtmTransactionEntryModeConditionEditor(Composite parent)
   {
      super(parent);
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ConditionEditor#getOperations()
    */
   @Override
   protected String[] getOperations()
   {
      return OPERATIONS;
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ConditionEditor#createContent(org.netxms.client.log.ColumnFilter)
    */
   @Override
   protected void createContent(ColumnFilter initialFilter)
   {
      entryModeCombo = new Combo(this, SWT.READ_ONLY | SWT.BORDER);
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      entryModeCombo.setLayoutData(gd);

      final String[] entryModes = LogLabelProvider.getAtmTransactionEntryModeTexts(i18n);
      for(String entryMode : entryModes)
         entryModeCombo.add(entryMode);
      entryModeCombo.select(0);

      if ((initialFilter != null) && (initialFilter.getType() == ColumnFilterType.LIKE))
      {
         setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
         for(int i = 0; i < LogLabelProvider.ATM_TXN_ENTRY_MODE_CODES.length; i++)
         {
            if (LogLabelProvider.ATM_TXN_ENTRY_MODE_CODES[i].equals(initialFilter.getLike()))
            {
               entryModeCombo.select(i);
               break;
            }
         }
      }
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ConditionEditor#createFilter()
    */
   @Override
   public ColumnFilter createFilter()
   {
      int op = getSelectedOperation();
      // Entry mode is stored as letter code, so exact match is done with LIKE without wildcards
      ColumnFilter filter = new ColumnFilter(LogLabelProvider.ATM_TXN_ENTRY_MODE_CODES[entryModeCombo.getSelectionIndex()]);
      filter.setNegated(op == 1);
      return filter;
   }
}
