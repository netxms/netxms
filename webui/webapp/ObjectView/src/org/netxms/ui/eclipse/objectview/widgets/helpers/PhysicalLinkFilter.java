package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.PhysicalLink;

public class PhysicalLinkFilter extends ViewerFilter
{
   private String filterString = null;
   private PhysicalLinkLabelProvider labelProvider;
   
   public PhysicalLinkFilter(PhysicalLinkLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }
   
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      PhysicalLink link = (PhysicalLink)element;
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (Long.toString(link.getId()).contains(filterString))
         return true;
      else if (link.getDescription().toLowerCase().contains(filterString))
         return true;
      else if (labelProvider.getObjectText(link, true).toLowerCase().contains(filterString))
         return true;
      else if (labelProvider.getPortText(link, true).toLowerCase().contains(filterString))
         return true;
      else if (labelProvider.getObjectText(link, false).toLowerCase().contains(filterString))
         return true;
      else if (labelProvider.getPortText(link, false).toLowerCase().contains(filterString))
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
