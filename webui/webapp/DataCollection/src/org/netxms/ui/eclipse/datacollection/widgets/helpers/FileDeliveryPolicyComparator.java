package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.ui.eclipse.datacollection.widgets.FileDeliveryPolicyEditor;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

public final class FileDeliveryPolicyComparator extends ViewerComparator
{
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int dir = ((SortableTreeViewer)viewer).getTree().getSortDirection();
      PathElement p1 = (PathElement)e1;
      PathElement p2 = (PathElement)e2;
      if (p1.isFile() && !p2.isFile())
         return (dir == SWT.UP) ? 1 : -1;
      if (!p1.isFile() && p2.isFile())
         return (dir == SWT.UP) ? -1 : 1;
      
      TreeColumn sortColumn = ((SortableTreeViewer)viewer).getTree().getSortColumn();
      if (sortColumn == null)
         return 0;
      
      int rc = 0;
      switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
      {
         case FileDeliveryPolicyEditor.COLUMN_NAME:
            rc = p1.getName().compareToIgnoreCase(p2.getName());
         case FileDeliveryPolicyEditor.COLUMN_GUID:
            rc = p1.getGuid() != null ? p1.getGuid().compareTo(p2.getGuid()) : 0;
         case FileDeliveryPolicyEditor.COLUMN_DATE:
            if (!p1.isFile())
               rc = 0;
            else
            {
               Date date1 = p1.getCreationTime();
               Date date2 = p2.getCreationTime();
               if (date1 == null)
                  if (date2 == null)
                     rc = 0;
                  else
                     rc = 1;
               else
                  if (date2 == null)
                     rc = -1;
                  else
                     rc = date1.compareTo(date2);
            }
      }
      return (dir == SWT.UP) ? rc : -rc;
   }
}