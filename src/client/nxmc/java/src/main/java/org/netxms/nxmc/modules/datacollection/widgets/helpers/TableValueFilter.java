package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.TableRow;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for table values
 */
public class TableValueFilter extends ViewerFilter implements AbstractViewerFilter
{
   private TokenizedFilter filter = new TokenizedFilter(null);

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {      
      if (filter.isEmpty())
         return true;
      else if (compareRow((TableRow)element))
         return true;
      return false;
   }
   
   private boolean compareRow(TableRow row)
   {
      String[] values = new String[row.size()];
      for(int i = 0; i < values.length; i++)
         values[i] = row.get(i).getValue();
      return filter.matches(values);
   }
   
   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filter.getFilterString();
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }
}
