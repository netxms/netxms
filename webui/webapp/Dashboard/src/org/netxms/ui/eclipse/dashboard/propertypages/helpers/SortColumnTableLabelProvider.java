package org.netxms.ui.eclipse.dashboard.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.dashboard.propertypages.DciSummaryTable;

public class SortColumnTableLabelProvider extends LabelProvider implements ITableLabelProvider
{

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      // TODO Auto-generated method stub
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      String column = (String)element;
      switch(columnIndex)
      {
         case DciSummaryTable.NAME:
            return column.substring(1);
         case DciSummaryTable.ORDER:
            return column.charAt(0) == '>' ? "Descending" : "Ascending" ;
      }
      return null;
   }

}
