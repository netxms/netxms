/**
 * 
 */
package org.netxms.nxmc.modules.dashboards.widgets.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.modules.dashboards.config.ObjectDetailsConfig.ObjectProperty;

/**
 * Label provider for "object details" table
 */
public class ObjectDetailsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private List<ObjectProperty> properties;
   
   /**
    * @param properties
    */
   public ObjectDetailsLabelProvider(List<ObjectProperty> properties)
   {
      super();
      this.properties = properties;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ObjectQueryResult r = (ObjectQueryResult)element;
      try
      {
         return r.getPropertyValue(properties.get(columnIndex).name);
      }
      catch(IndexOutOfBoundsException e)
      {
         return null;
      }
   }
}
