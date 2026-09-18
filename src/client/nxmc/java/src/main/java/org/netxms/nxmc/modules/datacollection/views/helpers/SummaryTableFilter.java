package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

public class SummaryTableFilter extends ViewerFilter implements AbstractViewerFilter
{
   private TokenizedFilter filter = new TokenizedFilter(null);
   
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      DciSummaryTableDescriptor d = (DciSummaryTableDescriptor)element;
      return filter.matches(Integer.toString(d.getId()), d.getMenuPath(), d.getTitle());
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }
}
