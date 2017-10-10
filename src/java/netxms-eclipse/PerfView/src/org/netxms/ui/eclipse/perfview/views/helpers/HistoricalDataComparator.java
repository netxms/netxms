package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.ui.eclipse.perfview.views.HistoricalDataView;
import org.netxms.ui.eclipse.tools.NaturalOrderComparator;

public class HistoricalDataComparator extends ViewerComparator
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;
      
      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case HistoricalDataView.COLUMN_TIME:
            rc = Long.signum(((DciDataRow)e1).getTimestamp().getTime() - ((DciDataRow)e2).getTimestamp().getTime());
            break;
         case HistoricalDataView.COLUMN_DATA:
            rc = NaturalOrderComparator.compare(((DciDataRow)e1).getValueAsString(), ((DciDataRow)e2).getValueAsString());
            break;
         default:
            rc = 0;
            break;
      }
      int dir = (((TableViewer)viewer).getTable().getSortDirection());
      return (dir == SWT.UP) ? rc : -rc;
   }
   
}
