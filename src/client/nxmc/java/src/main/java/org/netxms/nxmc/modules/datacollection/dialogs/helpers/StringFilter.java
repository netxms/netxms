/**
 * 
 */
package org.netxms.nxmc.modules.datacollection.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;

/**
 * Filter view with strings
 */
public class StringFilter extends ViewerFilter 
{
   private String filter = ""; 
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      String name = (String)element;
      return (filter == null) || filter.isEmpty() || name.toLowerCase().contains(filter);
   }

   /**
    * @return the filter
    */
   public String getFilter()
   {
      return filter;
   }

   /**
    * @param filter the filter to set
    */
   public void setFilter(String filter)
   {
      this.filter = filter.toLowerCase();
   }
}
