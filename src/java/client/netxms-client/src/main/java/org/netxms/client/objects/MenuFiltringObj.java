package org.netxms.client.objects;

import org.netxms.client.ObjectMenuFilter;

public interface MenuFiltringObj
{
   /**
    * Returns filter
    */
   public ObjectMenuFilter getFilter();
   

   /**
    * Sets filter
    */
   public void setFilter(ObjectMenuFilter filter);
   
   /**
    * Returns type of of object(for Object tools it is required for determining local commands)
    */
   public int getType();
}
