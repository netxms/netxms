/**
 * 
 */
package org.netxms.webui.mobile.pages.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractObject;
import org.netxms.webui.mobile.Activator;

/**
 * Object list label provider
 */
public class ObjectListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private Map<Integer, Image> objectIcons = new HashMap<Integer, Image>();
   
   public ObjectListLabelProvider()
   {
      objectIcons.put(AbstractObject.OBJECT_CLUSTER, Activator.getImageDescriptor("icons/object_cluster.png").createImage());
      objectIcons.put(AbstractObject.OBJECT_CONTAINER, Activator.getImageDescriptor("icons/object_container.png").createImage());
      objectIcons.put(AbstractObject.OBJECT_NODE, Activator.getImageDescriptor("icons/object_node.png").createImage());
      objectIcons.put(AbstractObject.OBJECT_SUBNET, Activator.getImageDescriptor("icons/object_subnet.png").createImage());
      objectIcons.put(AbstractObject.OBJECT_ZONE, Activator.getImageDescriptor("icons/object_zone.png").createImage());
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return objectIcons.get(((AbstractObject)element).getObjectClass());
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case 0:
            return ((AbstractObject)element).getObjectName();
      }
      return null;
   }

   @Override
   public void dispose()
   {
      for(Image i : objectIcons.values())
         i.dispose();
      super.dispose();
   }

   
}
