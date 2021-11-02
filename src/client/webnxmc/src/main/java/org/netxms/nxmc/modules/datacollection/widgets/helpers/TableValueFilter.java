package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.TableRow;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for table values
 */
public class TableValueFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (compareRow((TableRow)element))
         return true;
      return false;
   }
   
   private boolean compareRow(TableRow row)
   {
      for(int i = 0; i < row.size(); i++)
      {
         if (row.get(i).getValue().toLowerCase().contains(filterString))
            return true;
      }
      return false;
   }
   
   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
