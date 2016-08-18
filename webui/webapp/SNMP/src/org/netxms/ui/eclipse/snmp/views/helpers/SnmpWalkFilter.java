package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.snmp.SnmpValue;

public class SnmpWalkFilter extends ViewerFilter
{
   private String filterString = null;
   
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      // TODO Auto-generated method stub
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      final SnmpValue vl = (SnmpValue)element;
      if (containsValue(vl)) {
         return true;
      }
      else if (containsOid(vl)) {
         return true;
      }
      return false;
   }
   
   public boolean containsValue(SnmpValue vl)
   {
      if (vl.getValue().toLowerCase().contains(filterString.toLowerCase()))
         return true;
      return false;
   }
   
   public boolean containsOid(SnmpValue vl)
   {
      if (vl.getObjectId().toString().toLowerCase().contains(filterString.toLowerCase()))
         return true;
      return false;
   }
   
   public String getFilterString()
   {
      return filterString;
   }
   
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
