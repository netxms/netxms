package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

public class SummaryTableFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;
   
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (checkId(element))
         return true;
      else if (checkPath(element))
         return true;
      else if (checkTitle(element))
         return true;
      return false;
   }
   
   public boolean checkId(Object element)
   {
      if (Integer.toString(((DciSummaryTableDescriptor)element).getId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean checkPath(Object element)
   {
      if (((DciSummaryTableDescriptor)element).getMenuPath().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean checkTitle(Object element)
   {
      if (((DciSummaryTableDescriptor)element).getTitle().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
