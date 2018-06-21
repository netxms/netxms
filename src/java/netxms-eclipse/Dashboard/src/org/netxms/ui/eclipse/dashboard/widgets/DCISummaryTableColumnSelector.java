package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.Table;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.ui.eclipse.dashboard.propertypages.TableColumnSelectionDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

public class DCISummaryTableColumnSelector extends AbstractSelector
{
   private String emptySelectionName = "";
   private String columnName = null;
   private DciSummaryTable sourceSummaryTable;
   private Table sourceTable;

   public DCISummaryTableColumnSelector(Composite parent, int style, int options, String columnName, Table sourceTable, DciSummaryTable sourceSummaryTable)
   {
      super(parent, style, options == SHOW_CLEAR_BUTTON ? options : 0);
      this.columnName = columnName;
      setText(columnName == null ? emptySelectionName : columnName);
      this.sourceTable = sourceTable;
      this.sourceSummaryTable = sourceSummaryTable;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      String valuesArray[] = null;
      if (sourceTable != null)
      {
         valuesArray = sourceTable.getColumnDisplayNames();         
      }
      if (sourceSummaryTable != null)
      {
         List<DciSummaryTableColumn> column = sourceSummaryTable.getColumns();
         List<String> tmp = new ArrayList<String>();
         for(int i = 0; i < column.size(); i++)
         {
            tmp.add(column.get(i).getName());
         }
         valuesArray = tmp.toArray(new String[tmp.size()]);           
      }
      TableColumnSelectionDialog dlg = new TableColumnSelectionDialog(getShell(), valuesArray);
      if (dlg.open() == Window.OK)
      {
         columnName = dlg.getSelectedName();
         setText(columnName == null ? emptySelectionName : columnName);
         fireModifyListeners();
      }
   }
   
   /* (non-Javadoc)
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

   public void setSummaryTbale(DciSummaryTable sourceSummaryTable)
   {
      this.sourceSummaryTable = sourceSummaryTable;      
   }
}
