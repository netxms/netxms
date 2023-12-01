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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.List;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.Table;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.modules.dashboards.dialogs.TableColumnSelectionDialog;

/**
 * Selector for table column (either from table value or from summary table definition)
 */
public class TableColumnSelector extends AbstractSelector
{
   private String emptySelectionName = "";
   private String columnName = null;
   private DciSummaryTable sourceSummaryTable;
   private Table sourceTable;

   public TableColumnSelector(Composite parent, int style, String columnName, Table sourceTable, DciSummaryTable sourceSummaryTable)
   {
      super(parent, style);
      this.columnName = columnName;
      setText(columnName == null ? emptySelectionName : columnName);
      this.sourceTable = sourceTable;
      this.sourceSummaryTable = sourceSummaryTable;
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      String columnNames[] = null;
      if (sourceTable != null)
      {
         columnNames = sourceTable.getColumnDisplayNames();         
      }
      else if (sourceSummaryTable != null)
      {
         List<DciSummaryTableColumn> columns = sourceSummaryTable.getColumns();
         columnNames = new String[columns.size()];
         for(int i = 0; i < columns.size(); i++)
            columnNames[i] = columns.get(i).getName();
      }
      TableColumnSelectionDialog dlg = new TableColumnSelectionDialog(getShell(), columnNames);
      if (dlg.open() == Window.OK)
      {
         columnName = dlg.getColumnName();
         setText(columnName == null ? emptySelectionName : columnName);
         fireModifyListeners();
      }
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      columnName = null;
      setText(emptySelectionName);
      fireModifyListeners();
   }

   /**
    * @return the columnName
    */
   public String getColumnName()
   {
      return columnName;
   }

   /**
    * Set summary table to get information from.
    *
    * @param sourceSummaryTable new source summary table
    */
   public void setSummaryTable(DciSummaryTable sourceSummaryTable)
   {
      this.sourceSummaryTable = sourceSummaryTable;      
   }
}
