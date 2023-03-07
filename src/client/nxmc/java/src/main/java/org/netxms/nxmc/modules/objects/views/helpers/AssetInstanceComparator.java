package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.Map.Entry;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.AssetInstancesView;

/**
 * Asset instance comparator
 */
public class AssetInstanceComparator extends ViewerComparator
{
   AssetInstanceLabelProvider labelProvider;
   
   /**
    * Asset instance comparator
    * 
    * @param labelProvider asset instance comparator
    */
   public AssetInstanceComparator(AssetInstanceLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @SuppressWarnings("unchecked")
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final Entry<String,String> o1 = (Entry<String, String>)e1;
      final Entry<String,String> o2 = (Entry<String, String>)e2;

      int result = 0;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case AssetInstancesView.NAME:
            result = labelProvider.getName(o1.getKey()).compareToIgnoreCase(labelProvider.getName(o1.getKey()));
            break;
         case AssetInstancesView.VALUE:
            result = o1.getValue().compareToIgnoreCase(o2.getValue());
            break;
         case AssetInstancesView.IS_MANDATORY:
            result = labelProvider.isMandatory(o1.getKey()).compareToIgnoreCase(labelProvider.isMandatory(o2.getKey()));
            break;
         case AssetInstancesView.IS_UNIQUE:
            result = labelProvider.isUnique(o1.getKey()).compareToIgnoreCase(labelProvider.isUnique(o2.getKey()));
            break;
         case AssetInstancesView.SYSTEM_TYPE:
            result = labelProvider.getSystemType(o1.getKey()).compareToIgnoreCase(labelProvider.getSystemType(o2.getKey()));  
            break;      
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }   
}
