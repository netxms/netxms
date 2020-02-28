package org.netxms.ui.eclipse.usermanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.usermanager.Messages;

public final class UserFilter extends ViewerFilter
{
   private String filterString;
   private UserLabelProvider labelProvider;
   
   public UserFilter(UserLabelProvider labelProvider)
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
      
      final AbstractUserObject user = (AbstractUserObject)element;
      boolean found = false;
      if (element instanceof User)
      {
         String authMethod = "";
         try
         {
            authMethod = labelProvider.AUTH_METHOD[((User)user).getAuthMethod()];
         }
         catch(ArrayIndexOutOfBoundsException e)
         {
            authMethod = Messages.get().UserLabelProvider_Unknown;
         }
         
         if (authMethod.toLowerCase().contains(filterString) || 
               ((User)user).getFullName().toLowerCase().contains(filterString))
            return true;
      }
      
      String objClass = (element instanceof User) ? Messages.get().UserLabelProvider_User : Messages.get().UserLabelProvider_Group;
      String objType = ((user.getFlags() & AbstractUserObject.LDAP_USER) != 0) ? Messages.get().UserLabelProvider_LDAP : Messages.get().UserLabelProvider_Local;
      if (objClass.toLowerCase().contains(filterString) || 
            objType.toLowerCase().contains(filterString) ||
            user.getName().toLowerCase().contains(filterString) || 
            user.getDescription().toLowerCase().contains(filterString) ||
            user.getLdapDn().toLowerCase().contains(filterString))
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