/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.InetAddressEx;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.objectbrowser.views.ObjectFinder;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for object search results
 */
public class ObjectSearchResultComparator extends ViewerComparator
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final AbstractObject object1 = (AbstractObject)e1;
      final AbstractObject object2 = (AbstractObject)e2;
      final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
      
      int result;
      switch(column)
      {
         case ObjectFinder.COL_CLASS:
            String c1 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object1, column);
            String c2 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object2, column);
            result = c1.compareToIgnoreCase(c2);
            break;
         case ObjectFinder.COL_ID:
            result = Long.signum(object1.getObjectId() - object2.getObjectId());
            break;
         case ObjectFinder.COL_IP_ADDRESS:
            InetAddressEx a1 = getIpAddress(object1);
            InetAddressEx a2 = getIpAddress(object2);
            result = ComparatorHelper.compareInetAddresses(a1.getAddress(), a2.getAddress());
            break;
         case ObjectFinder.COL_NAME:
            result = object1.getObjectName().compareToIgnoreCase(object2.getObjectName());
            break;
         case ObjectFinder.COL_ZONE:
            String z1 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object1, column);
            String z2 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object2, column);
            result = z1.compareToIgnoreCase(z2);
            break;
         default:
            result = 0;
            break;
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
   
   /**
    * @param object
    * @return
    */
   private static InetAddressEx getIpAddress(AbstractObject object)
   {
      InetAddressEx addr = null;
      if (object instanceof AbstractNode)
         addr = ((AbstractNode)object).getPrimaryIP();
      else if (object instanceof AccessPoint)
         addr = ((AccessPoint)object).getIpAddress();
      else if (object instanceof Interface)
         addr = ((Interface)object).getFirstUnicastAddressEx();
      return (addr != null) ? addr : new InetAddressEx();
   }
}
