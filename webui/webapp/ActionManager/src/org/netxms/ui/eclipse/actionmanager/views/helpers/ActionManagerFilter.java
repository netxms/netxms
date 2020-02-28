package org.netxms.ui.eclipse.actionmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.ServerAction;

public final class ActionManagerFilter extends ViewerFilter
{
   private String filterString;
   private ActionLabelProvider labelProvider;
   
   public ActionManagerFilter(ActionLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      final ServerAction action = (ServerAction)element;
      boolean found = false;
      if(action.getName().toLowerCase().contains(filterString) ||
            labelProvider.ACTION_TYPE[action.getType()].toLowerCase().contains(filterString) ||
            action.getRecipientAddress().toLowerCase().contains(filterString) ||
            action.getEmailSubject().toLowerCase().contains(filterString) ||
            action.getData().toLowerCase().contains(filterString) ||
            action.getChannelName().toLowerCase().contains(filterString))
      {
         found = true;
      }
      
      return found;
   }

   public void setFilterString(String text)
   {
      filterString = text.toLowerCase();
   }

   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filterString;
   }
}
