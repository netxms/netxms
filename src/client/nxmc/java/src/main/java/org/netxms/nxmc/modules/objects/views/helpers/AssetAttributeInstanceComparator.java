package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.Map.Entry;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.AssetView;

/**
 * Asset attribute instance comparator
 */
public class AssetAttributeInstanceComparator extends ViewerComparator
{
   private AssetAttributeInstanceLabelProvider labelProvider;

   /**
    * Asset instance comparator
    * 
    * @param labelProvider asset instance comparator
    */
   public AssetAttributeInstanceComparator(AssetAttributeInstanceLabelProvider labelProvider)
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
         case AssetView.NAME:
            result = labelProvider.getName(o1.getKey()).compareToIgnoreCase(labelProvider.getName(o2.getKey()));
            break;
         case AssetView.VALUE:
            result = labelProvider.getValue(o1).compareToIgnoreCase(labelProvider.getValue(o2));
            break;
         case AssetView.IS_MANDATORY:
            result = labelProvider.isMandatory(o1.getKey()).compareToIgnoreCase(labelProvider.isMandatory(o2.getKey()));
            break;
         case AssetView.IS_UNIQUE:
            result = labelProvider.isUnique(o1.getKey()).compareToIgnoreCase(labelProvider.isUnique(o2.getKey()));
            break;
         case AssetView.SYSTEM_TYPE:
            result = labelProvider.getSystemType(o1.getKey()).compareToIgnoreCase(labelProvider.getSystemType(o2.getKey()));  
            break;      
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }   
}
