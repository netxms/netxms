/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ObjectQueryResult;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig.ObjectProperty;

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

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /* (non-Javadoc)
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
      catch(ArrayIndexOutOfBoundsException e)
      {
         return null;
      }
   }
}
